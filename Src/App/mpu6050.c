#include "mpu6050.h"

static uint8_t MPU6050_WriteReg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t value)
{
    return (HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                              &value, 1, 100) == HAL_OK) ? 0 : 1;
}

uint8_t MPU6050_ReadID(I2C_HandleTypeDef *hi2c, uint8_t *id)
{
    return (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_WHO_AM_I,
                             I2C_MEMADD_SIZE_8BIT, id, 1, 100) == HAL_OK) ? 0 : 1;
}

/*
 * Init sequence for vibration sampling:
 *  - wake up, clock source = X axis gyro PLL (more stable than internal RC)
 *  - sample rate divider = 0  -> 1kHz output data rate
 *  - digital low pass filter = 44Hz bandwidth (removes high frequency noise)
 *  - accelerometer full scale = +-4g (good for hand tapping / small motors)
 */
uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t id = 0;

    if (MPU6050_ReadID(hi2c, &id) != 0)
        return 1;

    /* WHO_AM_I of MPU6050 is 0x68, of MPU6500/9250 is 0x70/0x71, accept all of them */
    if ((id != 0x68) && (id != 0x70) && (id != 0x71) && (id != 0x98))
        return 2;

    if (MPU6050_WriteReg(hi2c, MPU6050_REG_PWR_MGMT_1, 0x01) != 0) return 3;
    HAL_Delay(10);
    if (MPU6050_WriteReg(hi2c, MPU6050_REG_SMPLRT_DIV, 0x00) != 0) return 4;
    if (MPU6050_WriteReg(hi2c, MPU6050_REG_CONFIG, 0x03) != 0) return 5;
    if (MPU6050_WriteReg(hi2c, MPU6050_REG_GYRO_CONFIG, 0x00) != 0) return 6;
    if (MPU6050_WriteReg(hi2c, MPU6050_REG_ACCEL_CONFIG, 0x08) != 0) return 7;

    return 0;
}

uint8_t MPU6050_ReadAccel(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];

    /* 5ms timeout: a 6 byte burst at 400kHz needs about 0.2ms, so this is
       generous, and it keeps the TIM3 interrupt short if the sensor hangs */
    if (HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, MPU6050_REG_ACCEL_XOUT_H,
                         I2C_MEMADD_SIZE_8BIT, buf, 6, 5) != HAL_OK)
        return 1;

    *ax = (int16_t)((buf[0] << 8) | buf[1]);
    *ay = (int16_t)((buf[2] << 8) | buf[3]);
    *az = (int16_t)((buf[4] << 8) | buf[5]);

    return 0;
}
