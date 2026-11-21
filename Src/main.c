/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mpu6050.h"
#include "ssd1306.h"
#include "fft_analyzer.h"
#include "sampler.h"
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
/* sample block: filled at exactly 1kHz from the MPU6050 X axis */
static float    sample_buf[FFT_N];
static float    fft_re[FFT_N];
static float    mag_buf[FFT_HALF + 1];

static uint32_t peak_bin = 0;      /* index of the strongest spectral line */
static float    peak_hz  = 0.0f;   /* that line converted to Hz            */

/* ---- idle / active detection thresholds ---------------------------------
 * amplitude unit: FFT magnitude in accelerometer LSB (+-4g, 8192 LSB/g).
 * PEAK_RATIO : the peak line must exceed the noise-floor mean by this factor
 * PEAK_FLOOR : absolute peak magnitude floor (LSB), rejects near-zero blocks
 * RMS_FLOOR  : time-domain RMS floor (LSB), rejects tiny idle motion
 * If a block fails either test it is reported as idle (PEAK = 0.0 Hz).
 * Tune these three constants together if idle still shows a fake peak,
 * or a light tap fails to trigger.                                          */
#define PEAK_RATIO    3.5f
#define PEAK_FLOOR    6.0f
#define RMS_FLOOR     8.0f
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
static void UART1_Print(const char *text);
static void UART1_PrintPeak(uint32_t bin, float hz);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* send a plain string over USART1 (PA9) at 115200 8N1 */
static void UART1_Print(const char *text)
{
  uint16_t len = 0;
  while ((text[len] != 0) && (len < 128))
    len++;
  HAL_UART_Transmit(&huart1, (uint8_t *)text, len, 100);
}

/* report the strongest spectral line, integer Hz is enough for the demo */
static void UART1_PrintPeak(uint32_t bin, float hz)
{
  char buf[48];
  uint16_t i = 0;
  char tmp[8];
  int8_t k = 0;
  uint32_t ip = (uint32_t)(hz + 0.5f);

  buf[i++] = 'P'; buf[i++] = 'E'; buf[i++] = 'A'; buf[i++] = 'K';
  buf[i++] = '=';
  if (ip == 0)
    tmp[k++] = '0';
  while ((ip > 0) && (k < 8))
  {
    tmp[k++] = (char)('0' + (ip % 10));
    ip /= 10;
  }
  while (k > 0)
    buf[i++] = tmp[--k];

  buf[i++] = 'H'; buf[i++] = 'z'; buf[i++] = ' ';
  buf[i++] = 'b'; buf[i++] = 'i'; buf[i++] = 'n'; buf[i++] = '=';
  k = 0;
  ip = bin;
  if (ip == 0)
    tmp[k++] = '0';
  while ((ip > 0) && (k < 8))
  {
    tmp[k++] = (char)('0' + (ip % 10));
    ip /= 10;
  }
  while (k > 0)
    buf[i++] = tmp[--k];

  buf[i++] = '\r'; buf[i++] = '\n'; buf[i] = 0;
  UART1_Print(buf);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  /* ---- OLED first, so every later step can report on screen ---- */
  OLED_Init(&hi2c1);
  OLED_Clear();
  OLED_String(0, 12, "STM32 VIB MONITOR");
  OLED_String(0, 28, "MPU6050 + FFT");
  OLED_Refresh();
  HAL_Delay(300);

  /* ---- accelerometer: WHO_AM_I check, 1kHz ODR, +-4g ---- */
  if (MPU6050_Init(&hi2c1) != 0)
  {
    OLED_Clear();
    OLED_String(0, 12, "MPU6050 FAIL");
    OLED_String(0, 28, "CHECK I2C WIRING");
    OLED_Refresh();
    UART1_Print("MPU6050 init failed\r\n");
    while (1)
    {
      /* stop here: nothing to analyse without the sensor */
    }
  }

  UART1_Print("MPU6050 ready: Fs=1000Hz, N=1024\r\n");

  /* ---- TIM3: 1kHz hardware sampler (update interrupt reads the sensor) ---- */
  Sampler_Init(&hi2c1);
  UART1_Print("TIM3 sampler started: 1kHz\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t n;
    float absmax, mmax, v;
    float mag_max, mag_sum, mag_mean, rms;
    uint32_t k, bin_max, n_bins;
    uint8_t active;
    float *sampled;
    int16_t x, y, yprev, bar_x;
    uint16_t bar_h;

    /* ============ 1) acquire one block: TIM3 fires every 1ms ============ */
    Sampler_Start();
    while (Sampler_IsDone() == 0)
    {
      /* the whole block is filled inside the TIM3 update interrupt,
         the main loop only waits, so no I2C bus conflict with the OLED */
    }
    sampled = Sampler_Buffer();
    for (n = 0; n < FFT_N; n++)
    {
      sample_buf[n] = sampled[n];
    }

    /* ============ 2) signal processing ============ */
    FFT_MovingAverage(sample_buf, FFT_N, 4);            /* 4 point moving average   */
    FFT_RemoveDC(sample_buf, FFT_N);                    /* drop the gravity offset */
    for (n = 0; n < FFT_N; n++)
    {
      fft_re[n] = sample_buf[n];
    }
    FFT_HannWindow(fft_re, FFT_N);                      /* reduce leakage */
    FFT_Spectrum(fft_re, mag_buf, FFT_N);               /* CMSIS-DSP real FFT */

    /* ============ 2.5) idle / active detection ==========================
     * A real vibration shows up both as a time-domain energy rise and as a
     * single spectral line standing out above the noise floor.  Pure sensor
     * noise has no outstanding line, so we reject it as idle instead of
     * reporting the biggest random noise bin as a frequency.               */
    rms = 0.0f;
    for (n = 0; n < FFT_N; n++)
    {
        rms += sample_buf[n] * sample_buf[n];
    }
    rms = sqrtf(rms / (float)FFT_N);

    mag_max = 0.0f;
    mag_sum = 0.0f;
    bin_max = 2;
    n_bins  = 0;
    for (k = 2; k <= 120; k++)                          /* 2 .. 117 Hz */
    {
        if (mag_buf[k] > mag_max)
        {
            mag_max = mag_buf[k];
            bin_max = k;
        }
        mag_sum += mag_buf[k];
        n_bins++;
    }
    mag_mean = mag_sum / (float)n_bins;

    active = (mag_max >= PEAK_RATIO * mag_mean) &&
             (mag_max >= PEAK_FLOOR) &&
             (rms >= RMS_FLOOR);

    if (active)
    {
        peak_bin = bin_max;
        peak_hz  = (float)peak_bin * (FFT_FS / (float)FFT_N);
    }
    else
    {
        peak_bin = 0;
        peak_hz  = 0.0f;
    }

    /* ============ 3) draw on the OLED ============ */
    OLED_Clear();

    /* line 1: main frequency */
    OLED_String(0, 0, "PEAK");
    OLED_StringNum(28, 0, "=", peak_hz, 1);
    OLED_String(74, 0, "Hz");

    /* line 2: parameters */
    OLED_StringNum(0, 8, "Fs=", FFT_FS, 0);
    OLED_String(48, 8, "N=1024");

    /* time domain waveform, area y=16..36 */
    absmax = 30.0f;   /* fixed floor: idle noise is not blown up full screen */
    for (n = 0; n < FFT_N; n += 8)
    {
      v = fabsf(sample_buf[n]);
      if (v > absmax)
        absmax = v;
    }
    yprev = 26;
    for (x = 0; x < 128; x++)
    {
      v = sample_buf[x * 8];
      y = (int16_t)(26 - (int16_t)(v / absmax * 10.0f));
      if (y < 16) y = 16;
      if (y > 36) y = 36;
      if (x == 0)
        OLED_Pixel(x, y);
      else
        OLED_Line((int16_t)(x - 1), yprev, x, y);
      yprev = y;
    }

    /* spectrum bars, area y=40..62, bin 1..63 -> 1Hz..61Hz */
    mmax = 3.0f;      /* floor so idle noise bars stay short */
    for (n = 1; n <= 63; n++)
    {
      if (mag_buf[n] > mmax)
        mmax = mag_buf[n];
    }
    for (n = 1; n <= 63; n++)
    {
      bar_h = (uint16_t)(mag_buf[n] / mmax * 22.0f);
      if (bar_h > 0)
      {
        bar_x = (int16_t)((n - 1) * 2);
        OLED_FillRect(bar_x, (int16_t)(62 - bar_h), 1, (int16_t)bar_h);
      }
    }
    OLED_HLine(0, 63, 128);

    OLED_Refresh();
    if (active)
        UART1_PrintPeak(peak_bin, peak_hz);
    else
        UART1_Print("IDLE\r\n");
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */
  /* fast mode 400kHz: needed for the OLED refresh, otherwise one full
     screen update takes ~90ms at 100kHz and the display flickers */
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
