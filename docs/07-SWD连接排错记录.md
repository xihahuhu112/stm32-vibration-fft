# 07 · SWD 连接排错记录（2026-11-21 晚）

## 1. 现象

- Keil：Debug 弹窗 `No target connected`，ID CODE 为空；Download 报 `Flash Download failed - Target DLL has been cancelled`。
- pyOCD 0.46.1（独立于 Keil 的另一套驱动栈）：`STLink error (9): Get IDCODE error` / `Target not detected`。
- 两套软件、两种连接方式（Normal / under Reset）、两种时钟（100 kHz / 1 MHz）结果相同。

## 2. 已逐项排除（均实测通过）

| # | 检查项 | 结果 |
|---|---|---|
| 1 | 下载器 USB 枚举 | 正常：`VID_0483&PID_3748`，Serial `000000000001`，HW V2，FW V2J48M7，驱动 Status OK（WinUSB） |
| 2 | 板子 MCU 是否存活 | 存活：`VID_0483&PID_5720` 枚举出两个 USB 存储设备（stmdev SPI Flash 2MB、mcudev SD Flash），跑的是卖家自带 USB MSC 演示固件 |
| 3 | 三根线端到端通断 | 重新插紧后蜂鸣档三根稳定响；单独验线线芯本身都通 |
| 4 | 空闲电平 | 上电后板子 DIO / CLK 对 GND 约 3.2 V，未被拉低、未短路 |
| 5 | 线序（按丝印，不按颜色） | 下载器右列：2 SWCLK←红线、4 SWDIO←棕线、6 GND←黑线；板子：GND(3)、DIO(4)、CLK(5)。DIO/CLK 对调重扫仍失败，已换回 |
| 6 | 供电方式 | 板子只走 Type-C 独立供电；下载器 3.3V 不并电（避免两路 3.3V 并联环流） |
| 7 | Keil 工程/器件包配置 | pyOCD 用通用 `cortex_m` 目标（不依赖 F407 pack）同样读不到 IDCODE，排除 Keil 侧配置问题 |
| 8 | 探针目标电压检测 | pyOCD 读到探针自测 VDD = 3.295 V，不是"检测不到目标电压"导致拒绝连接 |
| 9 | 失败层级 | 原始命令实测：`JTAG_ENTER2(SWD)` 探针固件直接回状态 `0x09`（Get IDCODE error）。即探针一进 SWD 模式读 IDCODE 就失败，发生在 SWD DP 物理层：探针发出时钟/数据后，目标没有 ACK |

## 3. 剩余嫌疑（本地无第二块板子/第二个下载器，无法继续分流）

- **A. 克隆下载器 SWD 收发通道损坏**：USB 栈正常（能枚举、能回版本），但 SWCLK/SWDIO 的驱动电路坏了，只能看到上拉电平、发不出有效波形。
- **B. 板子侧问题**：排针到 PA13(引脚71)/PA14(引脚73) 的走线或焊点虚焊；或芯片调试口被禁用 / RDP 读保护。

## 4. 下一步（按顺序做）

1. **交叉验证下载器**：借同学一块已知能正常烧录的 STM32 板，用本下载器连一次。能连 → 下载器 SWD 通道坏；连不上 → 大概率本下载器问题。
2. **交叉验证板子**：借同学一个正常的 ST-Link 接本板。能连 → 本下载器坏；仍 `No target connected` → 板子侧（虚焊或调试口被禁）。
3. **联系商家客服**：说明"下载器 USB 正常枚举、板子 USB MSC 正常，但 Keil 和 pyOCD 都读不到 IDCODE，Enter SWD 即回 0x09"，问芯片是否开了读保护 / 禁用了 SWD，能否换下载器或换板。把设备 PID（下载器 `3748`、板子 `5720`）和报错一并发过去。
4. **不要**用 `ST-LinkUpgrade.exe` 给克隆下载器升级固件，有变砖风险；该工具只看状态，不点升级。
5. 若之后拿到官方 STM32CubeProgrammer，可以看 RDP 等级；但 pyOCD 已证明 DP 层无应答，官方工具大概率同样连不上，期望值不要放太高。

## 5. 接线现状（断电后再改线）

- 板子 P1 丝印左→右：`BT0(1) 3V3(2) GND(3) DIO(4) CLK(5)`。
- 下载器右列：`2 SWCLK / 4 SWDIO / 6 GND / 8 3.3V / 10 5.0V`；左列是 STM8 SWIM 口，不用。
- 当前：红=SWCLK→CLK(5)，棕=SWDIO→DIO(4)，黑=GND→GND(3)；白线悬空；**只接 3.3V 体系，绝不接 5V**。
- NRST 未引出到扩展排针（只接板载复位键 S2 和对地电容 C5），有线复位飞线不可行，抢连只能按板载复位键。

## 6. 待 SWD 打通后还要回头核对的一件事

Keil Debug 设置里 CPU DLL 显示的是 `SARMCM3.DLL`，F407 是 Cortex-M4，正常应是 M4 系 DLL。这个不影响 DP 层读 IDCODE（IDCODE 在 ST-Link DLL 层），但下载前要核对 Options → Device 是否选对 `STM32F407VGTx`、F4 器件包是否完整。

---

## 7. 最终根因与解决（2026-11-21 晚，已打通并烧录成功）

### 7.1 根因：三根信号线误插到左列 STM8 SWIM 口

这块红色克隆 ST-Link V2 是 **STM8 & STM32 二合一**，2×5 双排针外观相同、丝印小，两列功能完全不同：

| 列 | 针脚 | 功能 | 本次是否使用 |
|---|---|---|---|
| 左列（奇数） | ① RST / ③ SWIM / ⑤ GND / ⑦ 3.3V / ⑨ 5.0V | **STM8 SWIM 口**，不用 | ❌ 最初误插在这一列 |
| 右列（偶数） | ② SWCLK / ④ SWDIO / ⑥ GND / ⑧ 3.3V / ⑩ 5.0V | **STM32 SWD 口** | ✅ 改接到这一列 |

整排 2×5 杜邦母座最初插在了左列 STM8 口：其中只有 ⑤ 脚碰巧也是 GND，所以万用表只有 GND 响；②④ 的 SWCLK/SWDIO 根本没接到调试脚上，探针发时钟/数据后目标无 ACK，DP 层回 `0x09`——这就是前面所有 `Get IDCODE error / No target connected` 的真正原因。前文第 3 节"下载器 SWD 通道损坏 / 板子调试口被禁"两条嫌疑 **均被推翻**，第 5 节"红/棕/黑已在右列"的早期列别判断也有误，以本节丝印针脚为准。

### 7.2 次要问题：USB 隔离器枚举失败

串在下载器和电脑之间的 USB 隔离器枚举出 `VID_0000&PID_0002`，Code 43「设备描述符请求失败」，导致下载器描述符都读不到。**绕过隔离器、ST-Link 直插主板 USB 口**后正常枚举。烧录阶段不需要隔离器。

### 7.3 解决后的接线（认丝印针脚，不认线色）

- 下载器 ② SWCLK → 板子 CLK(5)
- 下载器 ④ SWDIO → 板子 DIO(4)
- 下载器 ⑥ GND → 板子 GND(3)
- ⑧ 3.3V 悬空，板子走 Type-C 独立供电；只接 3.3V 体系，绝不接 5V。
- 手头线色（棕/红/橙）与商家标准色（黄/绿/蓝/紫）不一致，两头都对丝印核对。

### 7.4 实测结果

- pyOCD 100 kHz 读 CPUID `0xE000ED00 = 0x410FC241`（ARM Cortex-M4），UID `0x1FFF7A10` 非全 0/全 F。
- Keil → Debug → Settings：Adapter ST-LINK/V2，Serial `000000000001`，FW V2J48S7，Port **SW**，**IDCODE `0x2BA01477`，ARM CoreSight SW-DP**。
- Flash Download：`Erase Done. / Programming Done. / Verify OK. / Flash Load finished at 13:48:28`，固件烧录并校验通过。

### 7.5 Keil 工程配置复核（打通后回头核对第 6 节）

- Device = `STM32F407VGTx`，Cortex-M4，FPU2，IROM `0x08000000–0x080FFFFF`（1MB），IRAM1/IRAM2 分区正确。
- `SARMCM3.DLL` 是 Keil 通用 Cortex-M 调试 DLL，配合 Dialog DLL 参数 `-pCM4` 指定 M4，**属于正常配置，无需改成所谓 M4 专属 DLL**。
- Flash 算法 `STM32F4xx 1MB Flash`（08000000–080FFFFF）正确。
