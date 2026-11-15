# STM32F407 + MPU6050 振动频谱仪

《信息获取与检测技术》的期末项目。用 MPU6050 采振动，在 F407 上做 FFT，OLED 上直接看时域波形和频谱，串口打印主频。

代码写完编译通过了（0 error 0 warning），硬件还在路上，所以实物照片和实测数据暂缺，等板子到了再补，空表已经放在 `docs/04-实测记录.md`。

## 现在能做到什么

- TIM3 定时 1kHz 采样，1024 点一块，频率分辨率约 0.98Hz
- 每块先做 4 点滑动平均、去直流、加汉宁窗，再走 CMSIS-DSP 的实数 FFT
- OLED 上半屏画时域波形，下半屏画幅值谱柱状图（目前画到 61Hz 左右）
- 串口 115200 打印 `PEAK=xx.x Hz bin=xx`
- 静止时是一条平线；敲一下板子，时域出现振荡，频谱在低频段冒出一根柱子

```
MPU6050 ──I2C(PB6/PB7, 400kHz)──┐
                                ├── STM32F407VGT6 ──USART1(PA9)── 串口
SSD1306 OLED ──同一条 I2C───────┘        168MHz + FPU
```

## 接线

两块模块挂在同一条 I2C 上，供电统一用 3.3V：

| 模块引脚 | 接到 |
|---|---|
| MPU6050 VCC / GND | 3.3V / GND |
| MPU6050 SCL / SDA | PB6 / PB7 |
| OLED VCC / GND | 3.3V / GND |
| OLED SCL / SDA | PB6 / PB7 |
| ST-Link SWDIO / SWCLK | PA13 / PA14 |
| ST-Link GND / 3.3V | GND / 3.3V |

MPU6050 的另外四根（XDA、XCL、AD0、INT）空着不接，AD0 悬空就是默认地址 0x68，和 OLED 的 0x3C 不冲突。

两个注意：MPU6050 别接 5V；ST-Link 已经在给板子供 3.3V 了，就别再插板载 USB，双路供电容易把稳压片子搞坏。

## 几个参数的来由

**采样率 1kHz**：MPU6050 内部的数字低通我设成了 44Hz（`CONFIG=0x03`），1kHz 远远超过它的两倍，满足奈奎斯特。实际关心的是 0~44Hz 这一段，不会混叠。

**分辨率 0.98Hz**：就是 1000 / 1024 算出来的。想更细只能加点数或者降采样率，两个都意味着刷新变慢，1024 点大约 1 秒出一屏，演示刚好。

**量程 ±4g**：`ACCEL_CONFIG=0x08`。±2g 灵敏度更高但敲一下就满量程削顶，±4g 留点余量。

**找峰只看 2~120 号谱线**：第 0 根是直流，去掉重力偏置后仍有残余；第 1 根太靠近 0Hz，容易误判。120 号大约 117Hz，已经远超传感器带宽了。

**为什么用汉宁窗**：1024 点截断相当于乘了矩形窗，旁瓣高，强信号会把旁边的弱信号盖住。汉宁窗旁瓣低，代价是主瓣变宽、幅度要乘 2 修正。这里主要看主频位置，不在乎绝对幅值。

## 代码放在哪

```
Src/App/
├── mpu6050.c     WHO_AM_I 自检、上电配置、6 字节突发读三轴
├── sampler.c     TIM3 中断里读一次传感器，采满 1024 点置标志
├── fft_analyzer.c 滑动平均 / 去直流 / 汉宁窗 / FFT / 找峰
└── ssd1306.c     帧缓冲，画点、画线、填充矩形、5×7 字库
```

`main.c` 只动了 CubeMX 的 USER CODE 段：放缓冲区、写串口打印（没用 printf，自己把整数转成字符）、主循环里采集→处理→绘图。

采样放中断里是有意的：之前用 SysTick 的 `HAL_GetTick()` 软节拍，抖动能到零点几毫秒，做频谱不均匀。TIM3 硬件定时稳得多。

## 编译和烧录

Keil MDK（AC5 v5.06 update 7），器件包 `Keil.STM32F4xx_DFP.2.17.1`，打开 `MDK-ARM/VibrationMonitor.uvprojx` 直接编。

当前编译结果：

```
Code=16316  RO-data=121380  RW-data=48  ZI-data=25568
0 Error(s), 0 Warning(s)
```

RO 这么大主要是 CMSIS-DSP 的 FFT 系数表，RAM 只用了 25KB。

烧录：Keil 里 Debug 选 ST-Link，Port 选 SW，Flash Download 勾上 Reset and Run，加 STM32F4xx 1MB Flash 算法，F8 下载。

`Drivers/` 没传上来（45MB 第三方库，HAL 驱动 + CMSIS 器件头 + 手拷的 DSP 源码）。重新拉下来要编的话，步骤在 `docs/03-编译与环境.md`，还配了个 `tools/restore-drivers.ps1` 帮忙拷 DSP 那 8 个文件。

## 踩过的坑

1. **`arm_bitreversal2.S` 编不过**：那是 GNU 语法的汇编，AC5 一片 `A1163E`/`A1137E`，换成同目录的 `.c` 版本就好了。
2. **换完 `.c` 链接报 `arm_radix8_butterfly_f32`、`arm_cfft_init_f32` 未定义**：少拷了 `arm_cfft_radix8_f32.c` 和 `arm_cfft_init_f32.c`。补上后再编一次还报同样的错，把编译重跑一遍才过——增量编译看着 `.o` 在，链接还是用的旧状态。
3. **`TIM_HandleTypeDef` 未定义**：`stm32f4xx_hal_conf.h` 里 `HAL_TIM_MODULE_ENABLED` 被注释着，`hal_tim.c` 实际是空文件。取消注释就好了。
4. **CubeMX 里 TIM3 的参数改不动**：Clock Source 能选（下拉框点错了会冒出一个 TIM3_ETR 引脚，重选 Internal Clock 才消失），但 Prescaler、Counter Period 那两个格子怎么点都进不去编辑状态。最后走命令行：`set ip parameters TIM3 Prescaler 83` / `set ip parameters TIM3 Period 999`，写进 `.ioc` 再生成，代码里就有了。
5. **OLED 刷新慢**：100kHz 下刷一屏约 90ms，肉眼能看到闪。CubeMX 命令行把 I2C 设成 400kHz 不生效（生成出来还是 100000），于是在 `MX_I2C1_Init` 的 USER CODE 段里覆盖成 400000 并重新 `HAL_I2C_Init` 一次。
6. **外部晶振那条路走不通**：想用板上 8MHz 晶振跑 168MHz，命令行怎么配，生成的代码都是 `HSEState = RCC_HSE_OFF`。放弃，改成内部 HSI + PLL（M=16, N=336, P=2），同样 168MHz，还不挑板子有没有晶振。
7. **GitHub 传代码时好时坏**：`git push` 报 `Connection was reset`，但 DNS 解析正常、443 也能连，是 TLS 握手被打断。开 Watt Toolkit 的 GitHub 加速后 git 又报证书不认，`git config --global http.sslBackend schannel` 换成系统证书库才通。

## 还没做的

- 硬件到货后的实测（敲击频率和波形截图要填进实测记录）
- 面包板敲狠了会接触不良，可能得焊一块洞洞板把四根线固定住
- 频谱柱只画到 61Hz，想看更宽的频段要改柱子数量和 bin 范围
- 阈值报警没加：主频幅值超过设定值点亮 LED 或者串口喊一声

## License

MIT，见 `LICENSE`。
