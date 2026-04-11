/**
 * @file    wifi_log.c
 * @brief   WiFi SoftAP + TCP 日志服务器实现
 */

#include "wifi_log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

static const char *TAG = "WIFI_LOG";

/* ── 客户端 socket 槽位 ── */
static int              s_client_socks[WIFI_LOG_MAX_CLIENTS];
static SemaphoreHandle_t s_clients_mutex = NULL;

/* ── 防重入标志（每任务独立：用 task-local bool 即可，此处用全局原子标志简化） ── */
static volatile int s_in_log = 0;

/* ══════════════════════════════════════════════════════
 * 自定义 vprintf：同时输出到 UART 和所有 TCP 客户端
 * ════════════════════════════════════════════════════*/
static int wifi_log_vprintf(const char *fmt, va_list args)
{
    char buf[512];
    int len = vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    if (len <= 0) return 0;
    if (len >= (int)sizeof(buf)) len = (int)sizeof(buf) - 1;
    buf[len] = '\0';

    /* 始终输出到 UART（保持原有行为） */
    fwrite(buf, 1, len, stdout);

    /* 防止日志函数本身触发的 send() 产生递归调用 */
    if (s_in_log) return len;
    if (s_clients_mutex == NULL) return len;

    s_in_log = 1;
    if (xSemaphoreTake(s_clients_mutex, 0) == pdTRUE) {
        for (int i = 0; i < WIFI_LOG_MAX_CLIENTS; i++) {
            if (s_client_socks[i] < 0) continue;
            int ret = send(s_client_socks[i], buf, len, MSG_DONTWAIT);
            if (ret < 0) {
                /* 客户端断开，释放槽位 */
                close(s_client_socks[i]);
                s_client_socks[i] = -1;
            }
        }
        xSemaphoreGive(s_clients_mutex);
    }
    s_in_log = 0;

    return len;
}

/* ══════════════════════════════════════════════════════
 * TCP 日志服务器任务
 * ════════════════════════════════════════════════════*/
static void tcp_log_server_task(void *arg)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0) {
        ESP_LOGE(TAG, "socket() failed: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(WIFI_LOG_PORT),
    };

    if (bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "bind() failed: errno %d", errno);
        close(server_sock);
        vTaskDelete(NULL);
        return;
    }

    if (listen(server_sock, WIFI_LOG_MAX_CLIENTS) < 0) {
        ESP_LOGE(TAG, "listen() failed: errno %d", errno);
        close(server_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "TCP log server ready  ->  192.168.4.1:%d", WIFI_LOG_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* 找空槽 */
        xSemaphoreTake(s_clients_mutex, portMAX_DELAY);
        int slot = -1;
        for (int i = 0; i < WIFI_LOG_MAX_CLIENTS; i++) {
            if (s_client_socks[i] < 0) { slot = i; break; }
        }
        if (slot >= 0) {
            s_client_socks[slot] = client_sock;
        }
        xSemaphoreGive(s_clients_mutex);

        if (slot < 0) {
            /* 已满 */
            const char *msg = "=== Max clients reached, bye ===\r\n";
            send(client_sock, msg, strlen(msg), 0);
            close(client_sock);
            ESP_LOGW(TAG, "Client rejected (no free slot)");
        } else {
            char welcome[128];
            snprintf(welcome, sizeof(welcome),
                     "\r\n=== ESP32-S3 OSC Panel Log (slot %d) ===\r\n"
                     "=== Disconnect: close telnet/nc        ===\r\n\r\n",
                     slot);
            send(client_sock, welcome, strlen(welcome), 0);
            ESP_LOGI(TAG, "Log client connected from %s, slot=%d",
                     inet_ntoa(client_addr.sin_addr), slot);
        }
    }
}

/* ══════════════════════════════════════════════════════
 * WiFi 事件处理
 * ════════════════════════════════════════════════════*/
static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t *e = data;
            ESP_LOGI(TAG, "STA connected   MAC=" MACSTR " AID=%d",
                     MAC2STR(e->mac), e->aid);
        } else if (id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t *e = data;
            ESP_LOGI(TAG, "STA disconnected MAC=" MACSTR " AID=%d",
                     MAC2STR(e->mac), e->aid);
        }
    }
}

/* ══════════════════════════════════════════════════════
 * 公开初始化接口
 * ════════════════════════════════════════════════════*/
esp_err_t wifi_log_init(void)
{
    esp_err_t ret;

    /* 1. 初始化客户端槽位 */
    s_clients_mutex = xSemaphoreCreateMutex();
    for (int i = 0; i < WIFI_LOG_MAX_CLIENTS; i++) {
        s_client_socks[i] = -1;
    }

    /* 2. NVS（WiFi 驱动依赖） */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* 3. TCP/IP 协议栈 & 事件循环 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 4. 创建 AP 网络接口 */
    esp_netif_create_default_wifi_ap();

    /* 5. 初始化 WiFi 驱动 */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 6. 注册事件 */
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    /* 7. 配置 AP */
    wifi_config_t wifi_cfg = {
        .ap = {
            .ssid         = WIFI_LOG_SSID,
            .ssid_len     = (uint8_t)strlen(WIFI_LOG_SSID),
            .channel      = WIFI_LOG_CHANNEL,
            .password     = WIFI_LOG_PASSWORD,
            .max_connection = WIFI_LOG_MAX_CLIENTS,
            .authmode     = (strlen(WIFI_LOG_PASSWORD) >= 8)
                                ? WIFI_AUTH_WPA2_PSK
                                : WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started  SSID=\"%s\"  IP=192.168.4.1", WIFI_LOG_SSID);
    ESP_LOGI(TAG, "Connect then:  telnet 192.168.4.1 %d", WIFI_LOG_PORT);

    /* 8. 启动 TCP 日志服务任务 */
    xTaskCreate(tcp_log_server_task, "tcp_log", 4096, NULL, 4, NULL);

    /* 9. 重定向 ESP 日志输出到自定义 vprintf（UART + TCP） */
    esp_log_set_vprintf(wifi_log_vprintf);

    return ESP_OK;
}
