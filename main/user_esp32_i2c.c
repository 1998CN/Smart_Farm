/**
 *****************************************************************************
 * @file    : user_esp32_i2c.c
 * @brief   : ESP32 I2C Application
 * @author  : Cao Jin
 * @date    : 20-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#include "user_esp32_i2c.h"

/** @brief Default esp32 i2c configuration.  */
#define DEFAULT_ESP32_I2C_NUM           I2C_NUM_0
#define DEFAULT_ESP32_I2C_SDA           GPIO_NUM_22           
#define DEFAULT_ESP32_I2C_SCL           GPIO_NUM_23
#define DEFAULT_ESP32_I2C_FREQ_HZ       (400000U)
#define DEFAULT_ESP32_I2C_TIMEOUT_MS    (1000U)

esp_err_t user_esp32_i2c_init(void)
{
    i2c_port_t i2c_port = DEFAULT_ESP32_I2C_NUM;
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = DEFAULT_ESP32_I2C_SDA,
        .scl_io_num = DEFAULT_ESP32_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = DEFAULT_ESP32_I2C_FREQ_HZ,
    };

    i2c_param_config(i2c_port, &i2c_config);

    return i2c_driver_install(i2c_port, i2c_config.mode, 0, 0, 0);
}