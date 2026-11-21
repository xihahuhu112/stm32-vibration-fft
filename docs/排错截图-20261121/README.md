# 排错截图清单（2026-11-21 晚）

发给商家客服 / 网上求助时按下面顺序配图，文字描述见会话里的两段话术。

## 已截好（软件侧，5 张）

| 文件 | 内容 | 对应话术 |
|---|---|---|
| `01-设备枚举.png` | 下载器 `VID_0483&PID_3748`、板子 `VID_0483&PID_5720`，状态均 OK；盘符 F:/G: | 话术第 1、2 条 |
| `02-pyOCD报错.png` | pyOCD 0.46.1 通用 cortex_m 目标：`STLink error (9): Get IDCODE error` / `Target not detected`；探针自测 VDD 3.295V | 话术第 5 条 |
| `03-Keil-Settings弹窗.png` | ST-LINK/V2、Serial 000000000001、FW V2J48S7、Port SW；SW Device `No target connected`，ID CODE 为空 | 话术第 4 条 |
| `04-Keil下载报错输出.png` | Build Output：`Internal command error`、`Flash Download failed - Target DLL has been cancelled` | 话术第 4 条 |
| `05-此电脑盘符FG.png` | 此电脑里两个可移动磁盘 U 盘 (F:)、U 盘 (G:) | 话术第 2 条 |

原始文本留档：`设备枚举.txt`、`pyOCD报错.txt`（完整 traceback，网友要看细节时再贴）。

## 待补拍（实物侧，明天白天拍照）

1. **接线全景照**：板子 SWD 排针 + 下载器排针 + 三根杜邦线，丝印 `GND/DIO/CLK` 和 `SWCLK/SWDIO/GND` 要能看清。
2. **万用表读数照**：上电后表笔量 DIO 对 GND、CLK 对 GND，表盘显示 3.2V 左右的读数。

## 发帖注意

- 截图里没有个人账号信息；订单号单独发客服，不贴公开帖。
- 不要在帖子里写"下载器坏了""板子虚焊"这类结论，只陈述上面截图和实测现象。
