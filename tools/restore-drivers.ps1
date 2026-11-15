# 还原 Drivers/ 目录（第三方库不入库，用本脚本从本机 Keil 包里拷回来）
# 用法：在 PowerShell 里执行  powershell -ExecutionPolicy Bypass -File tools\restore-drivers.ps1
# 前提：Keil CMSIS 包已安装（本例 D:\Keil5\Pack\ARM\CMSIS\5.9.0），
#       HAL 部分请先用 CubeMX 打开 ..\VibrationMonitor.ioc 点 GENERATE CODE 生成。

$ErrorActionPreference = 'Stop'
$root  = Split-Path -Parent $PSScriptRoot          # 工程根目录（本脚本在 tools 下）
$cmsis = 'D:\Keil5\Pack\ARM\CMSIS\5.9.0\CMSIS\DSP'  # 按自己的路径修改

if (-not (Test-Path -LiteralPath $cmsis)) { throw "找不到 CMSIS DSP 包：$cmsis" }

$dstDsp = Join-Path $root 'Drivers\CMSIS\DSP'
New-Item -ItemType Directory -Force -Path (Join-Path $dstDsp 'Source\TransformFunctions') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $dstDsp 'Source\CommonTables')      | Out-Null

# 1) 头文件（整个 Include 目录）
Copy-Item -LiteralPath (Join-Path $cmsis 'Include') -Destination $dstDsp -Recurse -Force

# 2) 只需要 8 个源文件，不要拷整个 Source（几百 MB）
$files = @(
  'Source\TransformFunctions\arm_rfft_fast_f32.c',
  'Source\TransformFunctions\arm_rfft_fast_init_f32.c',
  'Source\TransformFunctions\arm_cfft_f32.c',
  'Source\TransformFunctions\arm_cfft_init_f32.c',
  'Source\TransformFunctions\arm_cfft_radix8_f32.c',
  'Source\TransformFunctions\arm_bitreversal2.c',   # 必须 .c，不要 .S
  'Source\CommonTables\arm_common_tables.c',
  'Source\CommonTables\arm_const_structs.c'
)
foreach ($f in $files) {
  $src = Join-Path $cmsis $f
  $dst = Join-Path $dstDsp $f
  if (-not (Test-Path -LiteralPath $src)) { throw "缺少文件：$src" }
  Copy-Item -LiteralPath $src -Destination $dst -Force
  Write-Host "OK  $f"
}

Write-Host "`nCMSIS-DSP 还原完成：$dstDsp"
Write-Host "HAL 驱动请用 CubeMX 打开 VibrationMonitor.ioc 点 GENERATE CODE 生成。"
Write-Host "然后在 Keil 里核对 Define=ARM_MATH_CM4 与 IncludePath=../Drivers/CMSIS/DSP/Include。"
