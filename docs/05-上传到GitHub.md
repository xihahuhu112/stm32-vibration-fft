# 05 · 上传到 GitHub

## 1. 本仓库提交什么、不提交什么

| 提交 | 不提交（已在 `.gitignore` 里排除） |
|---|---|
| `README.md`、`LICENSE`、`.gitignore`、`docs/`、`assets/`、`tools/` | `Drivers/`（约 45 MB 第三方库，还原方法见 03） |
| `Src/`、`Inc/`、`VibrationMonitor.ioc`、`.mxproject` | `MDK-ARM/*/Objects/`、`Listings/`（编译中间产物，约 49 MB） |
| `MDK-ARM/*.uvprojx`、`*.uvoptx`、`startup_stm32f407xx.s` | `*.hex`、`*.axf`、`*.map`（可执行文件，避免误传固件） |

提交量约 **182 KB / 29 个文件**，推送很快；如果把 `Drivers` 一起传会变成 46 MB，没必要。

## 2. 装 Git（本机还没装）

```powershell
winget install --id Git.Git -e --source winget
```

装完**关掉所有终端重开**，验证：

```powershell
git --version
```

## 3. 本地建仓并首次提交

在工程目录里执行（工程目录 = `VibrationMonitor`，Git Bash 或 PowerShell 都可）：

```powershell
cd 'C:\Users\hs\Doubao\chats\2026-11-13\new-chat-2\VibrationMonitor'
git init
git config user.name  "你的GitHub用户名"
git config user.email "你的邮箱"
git add .
git status                 # 检查列表里没有 Drivers / Objects / hex
git commit -m "feat: STM32F407+MPU6050 振动频谱分析仪初版（TIM3 1kHz 采样、CMSIS-DSP FFT、OLED 波形与频谱）"
git branch -M main
```

## 4. 建远程仓库并推送

1. 打开 GitHub → New repository → 仓库名建议 `stm32-vibration-fft`，**不要**勾选 "Add a README"，保持空仓库。
2. 复制它给出的地址，然后：

```powershell
git remote add origin https://github.com/<你的用户名>/stm32-vibration-fft.git
git push -u origin main
```

首次推送会弹浏览器登录，按提示授权即可。

## 5. 后续的提交习惯（这也是"别人怎么记录项目"的核心）

真实项目不是一次传完，而是每个可验证的进展一条提交：

| 时机 | 提交信息示例 |
|---|---|
| 硬件跑通、能出波形 | `feat: 硬件联调通过，OLED 显示时域波形` |
| 频谱出峰正确 | `feat: FFT 频谱显示与峰值频率输出（0.98Hz 分辨率）` |
| 修掉某个 bug | `fix: I2C 偶发读失败导致数据块卡死，改为连续 20 次失败即结束本块` |
| 调好显示比例 | `style: 调整波形纵向缩放，避免敲击时越界` |
| 写完实测记录 | `docs: 补充实测记录与波形截图` |

配套约定：

- 每个功能一条提交，信息写"做了什么 + 为什么"，不要 `update`、`修改一下`。
- 截图/波形图放进 `assets/`，并在 `docs/04-实测记录.md` 引用。
- 固件（hex/axf）不要进仓库，需要发固件就用 Release 附件。

## 6. 网络慢的备选方案

- 仓库只有 182 KB，正常几分钟就能推完；如果 GitHub 卡，改用 Gitee（码云）建私有仓库，推送方式完全一样，只是域名换成 `gitee.com`。
- 简历里贴链接时，Gitee 和 GitHub 等价，但要保证别人能打开（设为公开）。

## 7. 简历上怎么写这个仓库

> **基于 STM32F407 与 MPU6050 的振动信号频谱分析仪**（[GitHub 链接]）
> TIM3 硬件定时 1kHz 采样 1024 点，实现滑动平均滤波、去直流、汉宁窗加窗与 CMSIS-DSP 实数 FFT，频率分辨率 0.98 Hz；OLED 实时显示时域波形与幅值谱并输出主频，可用于电机轴承类低频振动的简易监测。
