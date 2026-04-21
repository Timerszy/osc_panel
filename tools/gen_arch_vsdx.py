# -*- coding: utf-8 -*-
"""
生成软件架构分层示意图 (Visio .vsdx 格式)

输出: d:/Projects/ESP/osc_panel/软件架构图.vsdx
"""
import os
import zipfile
from pathlib import Path

OUT = Path(__file__).resolve().parent.parent / "软件架构图.vsdx"

# ========== 静态 XML 零件 ==========

CONTENT_TYPES = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
<Default Extension="xml" ContentType="application/xml"/>
<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
<Override PartName="/visio/document.xml" ContentType="application/vnd.ms-visio.drawing.main+xml"/>
<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
<Override PartName="/visio/pages/pages.xml" ContentType="application/vnd.ms-visio.pages+xml"/>
<Override PartName="/visio/pages/page1.xml" ContentType="application/vnd.ms-visio.page+xml"/>
<Override PartName="/visio/windows.xml" ContentType="application/vnd.ms-visio.windows+xml"/>
</Types>'''

ROOT_RELS = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.microsoft.com/visio/2010/relationships/document" Target="visio/document.xml"/>
<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
</Relationships>'''

APP_XML = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties">
<Application>Microsoft Visio</Application>
<AppVersion>15.0000</AppVersion>
</Properties>'''

CORE_XML = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" xmlns:dc="http://purl.org/dc/elements/1.1/">
<dc:title>示波器面板控制系统 - 软件架构图</dc:title>
<dc:creator>Song Zhenyu</dc:creator>
</cp:coreProperties>'''

DOCUMENT_XML = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<VisioDocument xmlns="http://schemas.microsoft.com/office/visio/2012/main" xml:space="preserve">
<DocumentSettings TopPage="0" DefaultTextStyle="0" DefaultLineStyle="0" DefaultFillStyle="0" DefaultGuideStyle="0">
<GlueSettings>9</GlueSettings>
<SnapSettings>65847</SnapSettings>
<SnapExtensions>34</SnapExtensions>
<SnapAngles/>
<DynamicGridEnabled>1</DynamicGridEnabled>
<ProtectStyles>0</ProtectStyles>
<ProtectShapes>0</ProtectShapes>
<ProtectMasters>0</ProtectMasters>
<ProtectBkgnds>0</ProtectBkgnds>
</DocumentSettings>
<Colors>
<ColorEntry IX="0" RGB="#000000"/>
<ColorEntry IX="1" RGB="#FFFFFF"/>
</Colors>
<FaceNames>
<FaceName NameU="Calibri" UnicodeRanges="0 0 0 0" CharSets="0 0" Panos="2 15 5 2 2 2 4 3 2 4" Flags="325"/>
<FaceName NameU="Microsoft YaHei" UnicodeRanges="0 268435456 22 0" CharSets="134 0" Panos="2 11 5 3 2 2 4 2 2 4" Flags="327"/>
</FaceNames>
<StyleSheets>
<StyleSheet ID="0" NameU="No Style" Name="No Style"/>
</StyleSheets>
<DocumentSheet NameU="TheDoc" LineStyle="0" FillStyle="0" TextStyle="0"/>
</VisioDocument>'''

DOC_RELS = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.microsoft.com/visio/2010/relationships/pages" Target="pages/pages.xml"/>
<Relationship Id="rId2" Type="http://schemas.microsoft.com/visio/2010/relationships/windows" Target="windows.xml"/>
</Relationships>'''

WINDOWS_XML = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Windows xmlns="http://schemas.microsoft.com/office/visio/2012/main" ClientWidth="1440" ClientHeight="900">
<Window ID="0" WindowType="Drawing" WindowState="1073741824" WindowLeft="0" WindowTop="0" WindowWidth="1440" WindowHeight="900" ContainerType="Page" Page="0" ViewScale="-1" ViewCenterX="7" ViewCenterY="5"/>
</Windows>'''

PAGES_XML = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Pages xmlns="http://schemas.microsoft.com/office/visio/2012/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" xml:space="preserve">
<Page ID="0" NameU="Page-1" Name="Page-1" ViewScale="-1" ViewCenterX="7" ViewCenterY="5">
<PageSheet LineStyle="0" FillStyle="0" TextStyle="0">
<Cell N="PageWidth" V="14"/>
<Cell N="PageHeight" V="10"/>
<Cell N="ShdwOffsetX" V="0.125"/>
<Cell N="ShdwOffsetY" V="-0.125"/>
<Cell N="PageScale" V="1"/>
<Cell N="DrawingScale" V="1"/>
<Cell N="DrawingSizeType" V="3"/>
<Cell N="DrawingScaleType" V="0"/>
<Cell N="InhibitSnap" V="0"/>
<Cell N="PageLockReplace" V="0"/>
<Cell N="PageLockDuplicate" V="0"/>
<Cell N="UIVisibility" V="0"/>
<Cell N="ShdwType" V="0"/>
<Cell N="ShdwObliqueAngle" V="0"/>
<Cell N="ShdwScaleFactor" V="1"/>
<Cell N="DrawingResizeType" V="1"/>
<Cell N="PageShapeSplit" V="1"/>
</PageSheet>
<Rel r:id="rId1"/>
</Page>
</Pages>'''

PAGES_RELS = '''<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.microsoft.com/visio/2010/relationships/page" Target="page1.xml"/>
</Relationships>'''


# ========== 形状构造 ==========

def rect_shape(sid, x, y, w, h, text, fill="#D9E2F3", line="#1F3864",
               font_size=0.14, bold=False):
    """生成一个矩形 Shape. (x, y) = 中心坐标."""
    bold_attr = ""
    if bold:
        bold_attr = '<Section N="Character"><Row IX="0"><Cell N="Style" V="1"/></Row></Section>'
    return f'''<Shape ID="{sid}" Type="Shape" LineStyle="0" FillStyle="0" TextStyle="0">
<Cell N="PinX" V="{x}"/>
<Cell N="PinY" V="{y}"/>
<Cell N="Width" V="{w}"/>
<Cell N="Height" V="{h}"/>
<Cell N="LocPinX" V="{w/2}" F="Width*0.5"/>
<Cell N="LocPinY" V="{h/2}" F="Height*0.5"/>
<Cell N="Angle" V="0"/>
<Cell N="FlipX" V="0"/>
<Cell N="FlipY" V="0"/>
<Cell N="ResizeMode" V="0"/>
<Cell N="LineWeight" V="0.015"/>
<Cell N="LineColor" V="{line}"/>
<Cell N="LinePattern" V="1"/>
<Cell N="FillForegnd" V="{fill}"/>
<Cell N="FillBkgnd" V="#FFFFFF"/>
<Cell N="FillPattern" V="1"/>
<Cell N="Char.Font" V="4"/>
<Cell N="Char.Color" V="#000000"/>
<Cell N="Char.Size" V="{font_size}"/>
<Cell N="Char.Style" V="{1 if bold else 0}"/>
<Cell N="VerticalAlign" V="1"/>
<Cell N="TxtPinX" V="{w/2}" F="Width*0.5"/>
<Cell N="TxtPinY" V="{h/2}" F="Height*0.5"/>
<Cell N="TxtWidth" V="{w-0.1}" F="Width*1-0.1"/>
<Cell N="TxtHeight" V="{h-0.05}" F="Height*1-0.05"/>
<Section N="Geometry" IX="0">
<Cell N="NoFill" V="0"/>
<Cell N="NoLine" V="0"/>
<Cell N="NoShow" V="0"/>
<Cell N="NoSnap" V="0"/>
<Row T="MoveTo" IX="1"><Cell N="X" V="0"/><Cell N="Y" V="0"/></Row>
<Row T="LineTo" IX="2"><Cell N="X" V="{w}"/><Cell N="Y" V="0"/></Row>
<Row T="LineTo" IX="3"><Cell N="X" V="{w}"/><Cell N="Y" V="{h}"/></Row>
<Row T="LineTo" IX="4"><Cell N="X" V="0"/><Cell N="Y" V="{h}"/></Row>
<Row T="LineTo" IX="5"><Cell N="X" V="0"/><Cell N="Y" V="0"/></Row>
</Section>
<Text>{text}</Text>
</Shape>'''


def arrow_shape(sid, x1, y1, x2, y2, color="#404040"):
    """上下贯穿的纵向箭头(表示数据流/调用方向). (x1,y1) -> (x2,y2) ."""
    dx, dy = x2 - x1, y2 - y1
    return f'''<Shape ID="{sid}" Type="Shape" LineStyle="0" FillStyle="0" TextStyle="0">
<Cell N="PinX" V="{x1}"/>
<Cell N="PinY" V="{y1}"/>
<Cell N="Width" V="{max(abs(dx), 0.01)}"/>
<Cell N="Height" V="{max(abs(dy), 0.01)}"/>
<Cell N="LocPinX" V="0"/>
<Cell N="LocPinY" V="0"/>
<Cell N="BeginX" V="{x1}"/>
<Cell N="BeginY" V="{y1}"/>
<Cell N="EndX" V="{x2}"/>
<Cell N="EndY" V="{y2}"/>
<Cell N="LineWeight" V="0.02"/>
<Cell N="LineColor" V="{color}"/>
<Cell N="LinePattern" V="1"/>
<Cell N="BeginArrow" V="13"/>
<Cell N="EndArrow" V="13"/>
<Cell N="BeginArrowSize" V="2"/>
<Cell N="EndArrowSize" V="2"/>
<Section N="Geometry" IX="0">
<Cell N="NoFill" V="1"/>
<Cell N="NoLine" V="0"/>
<Cell N="NoShow" V="0"/>
<Cell N="NoSnap" V="0"/>
<Row T="MoveTo" IX="1"><Cell N="X" V="0"/><Cell N="Y" V="0"/></Row>
<Row T="LineTo" IX="2"><Cell N="X" V="{dx}"/><Cell N="Y" V="{dy}"/></Row>
</Section>
</Shape>'''


# ========== 布局数据 ==========
# 页面: 14" x 10"  (横向)

shapes_xml = []
next_id = 1

def add(s):
    global next_id
    shapes_xml.append(s)
    next_id += 1
    return next_id - 1

# 标题
add(rect_shape(next_id, 7, 9.55, 13, 0.5,
               "&#31034;&#27874;&#22120;&#38754;&#26495;&#26174;&#31034;&#35843;&#33410;&#25511;&#21046;&#31995;&#32479; &#8212; &#36719;&#20214;&#20998;&#23618;&#26550;&#26500;",
               fill="#2F5496", line="#1F3864", font_size=0.22, bold=True))
# 第一行文本颜色白色 - 需要修改 rect_shape 支持，这里用备注方式简化

# 四层背景底板 (淡色)
LAYER_Y = [7.8, 5.8, 3.8, 1.8]      # 各层中心 y
LAYER_H = 1.6
LAYER_W = 13
LAYER_X = 7

layer_labels = [
    ("&#24212;&#29992;&#36923;&#36753;&#23618;  Application Logic Layer", "#FFF2CC", "#BF8F00"),
    ("FreeRTOS &#20219;&#21153;&#23618;  Task Layer",                     "#E2EFDA", "#375623"),
    ("&#22806;&#35774;&#39537;&#21160;&#23618;  Hardware Driver Layer",   "#DDEBF7", "#1F4E79"),
    ("&#30828;&#20214;&#23618;  Hardware Layer",                           "#FBE5D6", "#C65911"),
]

for i, (lbl, fg, ln) in enumerate(layer_labels):
    # 背景大矩形 (放层名在左侧)
    add(rect_shape(next_id, LAYER_X, LAYER_Y[i], LAYER_W, LAYER_H,
                   "", fill=fg, line=ln, font_size=0.12))

# --- 层 1: 应用逻辑层 (4 个子模块) ---
L1_Y = LAYER_Y[0]
l1_boxes = [
    ("panel_state.h\n&#20840;&#23616;&#29366;&#24577;\n(Mutex &#20445;&#25252;)",      "#FFF2CC"),
    ("event_queue\n32 &#20107;&#20214;&#38431;&#21015;",                               "#FFF2CC"),
    ("waveform.c\n&#27874;&#24418;&#28210;&#26579;\n(PSRAM &#24125;&#32531;&#20914;)",  "#FFF2CC"),
    ("&#24180;&#39564;/&#29366;&#24577;&#26426;\n(RUN/SCOPE/TDR/IDLE)",                 "#FFF2CC"),
]
for i, (t, fg) in enumerate(l1_boxes):
    x = 1.5 + i * 3.15
    add(rect_shape(next_id, x, L1_Y, 2.8, 1.05, t, fill=fg, line="#BF8F00"))

# 左侧层标签
add(rect_shape(next_id, 0.7, L1_Y, 0.9, 1.3, layer_labels[0][0], fill="#FFE699", line="#BF8F00", font_size=0.11, bold=True))

# --- 层 2: 任务层 ---
L2_Y = LAYER_Y[1]
l2_boxes = [
    ("event_task\nprio=10\n&#25353;&#38190;/&#32534;&#30721;&#22120;&#20998;&#21457;",      "#E2EFDA"),
    ("display_task\nprio=5\n100ms &#21047;&#27874;&#24418;",                                "#E2EFDA"),
    ("adc_task\nprio=6\n~20Hz &#37319;&#26679;",                                            "#E2EFDA"),
    ("heartbeat_task\nprio=3\n1s &#24515;&#36339;+&#29366;&#24577;",                        "#E2EFDA"),
]
for i, (t, fg) in enumerate(l2_boxes):
    x = 1.5 + i * 3.15
    add(rect_shape(next_id, x, L2_Y, 2.8, 1.05, t, fill=fg, line="#375623"))

add(rect_shape(next_id, 0.7, L2_Y, 0.9, 1.3, layer_labels[1][0], fill="#A9D08E", line="#375623", font_size=0.11, bold=True))

# --- 层 3: 驱动层 (7 个小盒) ---
L3_Y = LAYER_Y[2]
l3_boxes = [
    "encoder.c\nEC11\n&#27491;&#20132;&#35299;&#30721;",
    "button.c\nGPIO\n&#28040;&#25238;",
    "led.c\nGPIO\n&#25351;&#31034;&#28783;",
    "lcd_st7789.c\nSPI2 + DMA\n&#24125;&#32531;&#20914; 112KB",
    "adc_sampler.c\nADC oneshot\n12-bit",
    "uart_comm.c\nUART1\n&#24086;&#21327;&#35758;",
    "wifi_log.c\nSoftAP+TCP\n&#26085;&#24535;&#37325;&#23450;&#21521;",
]
n = len(l3_boxes)
for i, t in enumerate(l3_boxes):
    x = 1.85 + i * 1.76
    add(rect_shape(next_id, x, L3_Y, 1.55, 1.1, t, fill="#DDEBF7", line="#1F4E79", font_size=0.105))

add(rect_shape(next_id, 0.7, L3_Y, 0.9, 1.3, layer_labels[2][0], fill="#BDD7EE", line="#1F4E79", font_size=0.11, bold=True))

# --- 层 4: 硬件层 ---
L4_Y = LAYER_Y[3]
l4_boxes = [
    "EC11 x 4\nGPIO 1-8",
    "BTN x 4\nGPIO 9-12",
    "LED x 3\nGPIO 13-15\n+ WS2812 off",
    "ST7789 LCD\n240x240\nSPI 40MHz",
    "ADC1\nGPIO 4/5",
    "UART1\nGPIO 17/18",
    "WiFi SoftAP\nSSID: timer",
]
for i, t in enumerate(l4_boxes):
    x = 1.85 + i * 1.76
    add(rect_shape(next_id, x, L4_Y, 1.55, 1.1, t, fill="#FBE5D6", line="#C65911", font_size=0.105))

add(rect_shape(next_id, 0.7, L4_Y, 0.9, 1.3, layer_labels[3][0], fill="#F4B183", line="#C65911", font_size=0.11, bold=True))

# --- 纵向连接箭头 (层间数据流方向) ---
# 层 2 <-> 层 1 : 双向 (任务读写全局状态)
for x in [3, 7, 11]:
    add(arrow_shape(next_id, x, L1_Y - LAYER_H/2 - 0.02, x, L2_Y + LAYER_H/2 + 0.02, color="#7F7F7F"))

# 层 3 <-> 层 2 : 双向 (驱动回调/任务调用)
for x in [2.5, 5.5, 8.5, 11.5]:
    add(arrow_shape(next_id, x, L2_Y - LAYER_H/2 - 0.02, x, L3_Y + LAYER_H/2 + 0.02, color="#7F7F7F"))

# 层 4 <-> 层 3
for x in [2.5, 5.5, 8.5, 11.5]:
    add(arrow_shape(next_id, x, L3_Y - LAYER_H/2 - 0.02, x, L4_Y + LAYER_H/2 + 0.02, color="#7F7F7F"))


# ========== 组装 page1.xml ==========
PAGE1_XML = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>\n' \
            '<PageContents xmlns="http://schemas.microsoft.com/office/visio/2012/main" xml:space="preserve">\n' \
            '<Shapes>\n' + "\n".join(shapes_xml) + '\n</Shapes>\n' \
            '</PageContents>'


# ========== 写入 .vsdx ZIP ==========
OUT.parent.mkdir(parents=True, exist_ok=True)

with zipfile.ZipFile(OUT, "w", zipfile.ZIP_DEFLATED) as z:
    z.writestr("[Content_Types].xml", CONTENT_TYPES)
    z.writestr("_rels/.rels", ROOT_RELS)
    z.writestr("docProps/app.xml", APP_XML)
    z.writestr("docProps/core.xml", CORE_XML)
    z.writestr("visio/document.xml", DOCUMENT_XML)
    z.writestr("visio/_rels/document.xml.rels", DOC_RELS)
    z.writestr("visio/windows.xml", WINDOWS_XML)
    z.writestr("visio/pages/pages.xml", PAGES_XML)
    z.writestr("visio/pages/_rels/pages.xml.rels", PAGES_RELS)
    z.writestr("visio/pages/page1.xml", PAGE1_XML)

print(f"Generated: {OUT}")
print(f"Size: {OUT.stat().st_size} bytes")
