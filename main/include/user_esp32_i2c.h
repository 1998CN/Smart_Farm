/**
 *****************************************************************************
 * @file    : user_esp32_i2c.h
 * @brief   : ESP32 I2C Application
 * @author  : Cao Jin
 * @date    : 20-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#ifndef USER_ESP32_I2C_H
#define USER_ESP32_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/i2c.h"
#include "driver/gpio.h"

/** @brief Default esp32 i2c configuration.  */
#define DEFAULT_ESP32_I2C_NUM           I2C_NUM_0
#define DEFAULT_ESP32_I2C_SDA           GPIO_NUM_22           
#define DEFAULT_ESP32_I2C_SCL           GPIO_NUM_23
#define DEFAULT_ESP32_I2C_FREQ_HZ       (400000U)
#define DEFAULT_ESP32_I2C_TIMEOUT_MS    (1000U)

esp_err_t user_esp32_i2c_init(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_ESP32_I2C_H */
/******************************** End of File *********************************/