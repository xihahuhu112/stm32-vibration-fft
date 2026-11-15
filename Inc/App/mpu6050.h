#ifndef __MPU6050_H
#define __MPU6050_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* AD0 pin low -> address 0x68, HAL needs 8bit address */
#define MPU6050_ADDR        (0x68 << 1)

/* register map */
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75

uint8_t MPU6050_Init(I2C_HandleTypeDef *hi2c);
uint8_t MPU6050_ReadAccel(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az);
uint8_t MPU6050_ReadID(I2C_HandleTypeDef *hi2c, uint8_t *id);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H */
