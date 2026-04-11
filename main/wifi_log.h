/**
 * @file    wifi_log.h
 * @brief   WiFi SoftAP + TCP 日志服务器
 *
 * 功能:
 *   - 开启 WiFi SoftAP，SSID = "timer"，无密码（开放网络）
 *   - 启动 TCP 服务器监听 4444 端口
 *   - 接管 esp_log 输出，同时写 UART 和所有已连接的 TCP 客户端
 *
 * 使用方法:
 *   1. 在 app_main 最开始调用 wifi_log_init()
 *   2. 连接到 WiFi "timer"，用 telnet/nc 连接 192.168.4.1:4444 即可查看日志
 */
#pragma once

#include "esp_err.h"

#define WIFI_LOG_SSID         "timer"      /* WiFi 热点名称 */
#define WIFI_LOG_PASSWORD     ""           /* 留空 = 开放网络；填写则需 ≥8 位 */
#define WIFI_LOG_CHANNEL      1
#define WIFI_LOG_PORT         4444         /* TCP 日志端口 */
#define WIFI_LOG_MAX_CLIENTS  4            /* 最多同时连接的客户端数 */

/**
 * @brief 初始化 WiFi SoftAP 并启动 TCP 日志服务器
 *
 * 该函数会:
 *   - 初始化 NVS、esp_netif、esp_event_loop
 *   - 启动 WiFi AP（SSID: WIFI_LOG_SSID）
 *   - 启动 TCP 日志服务任务
 *   - 调用 esp_log_set_vprintf() 重定向日志输出
 *
 * @note 必须在其他外设初始化之前调用（日志会从此刻起同步到 WiFi）
 * @return ESP_OK 成功，其他值见 esp_err.h
 */
esp_err_t wifi_log_init(void);
