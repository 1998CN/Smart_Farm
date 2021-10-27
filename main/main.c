/**
 *****************************************************************************
 * @file    : main.c
 * @brief   : Smart farm Application
 * @author  : Cao Jin
 * @date    : 15-Oct-2021
 * @version : 1.0.0 
 *****************************************************************************
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "user_esp32_wifi.h"
#include "user_esp32_mqtt.h"
#include "user_esp32_ota.h"
#include "user_esp32_pwm.h"
#include "user_esp32_rmt.h"
#include "user_esp32_uart.h"
#include "user_esp32_modbus.h"
#include "user_esp32_i2c.h"
#include "user_esp32_hardware.h"
/*  
 * Name   : log_chip_info
 * Brief  : Print chip information
 * param  : None.
 * retval : None.
 * Date   : 2021-07-05
 */
static void user_esp32_print_chip_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    ESP_LOGI("Chip information", "This is %s chip with %d CPU core(s), WiFi%s%s, silicon revision %d, %dMB %s flash",
             CONFIG_IDF_TARGET,
             chip_info.cores,
             (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
             chip_info.revision,
             spi_flash_get_chip_size() / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI("Chip information", "Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());
}

void app_main(void)
{
    /* OTA data verification. */
    user_esp32_ota_data_verification();

    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Print chip information */
    user_esp32_print_chip_info();
    /* Initialize Wi-Fi */
    user_esp32_wifi_init();
    /* Initialize I2C. */
    user_esp32_i2c_init();
    /* Initialize UART. */
    user_esp32_uart_init();
    /* Initialize PWM. */
    user_esp32_pwm_init();
    /* Initialize RMT. */
    user_esp32_rmt_init();
    /* Initialize hardware. */
    user_esp32_hardware_init();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}