/**
 *****************************************************************************
 * @file    : user_esp32_uart.c
 * @brief   : ESP32 UART Application
 * @author  : Cao Jin
 * @date    : 20-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include "esp_err.h"
#include "esp_log.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "user_esp32_uart.h"

/** @brief   */
#define DEFAULT_UART_NUM    ()
#define DEFAULT_UART_BOUND  ()
#define DEFAULT_UART_TX_PIN (GPIO_NUM_22)
#define DEFAULT_UART_RX_PIN (GPIO_NUM_22)
#define DEFAULT_UART_CTS_PIN    ()
#define DEFAULT_UART_RTS_PIN    ()

esp_err_t user_esp32_uart_init(void)
{
    




    return ESP_OK;
}

esp_err_t user_esp32_uart_deinit(void)
{



    return ESP_OK;
}



