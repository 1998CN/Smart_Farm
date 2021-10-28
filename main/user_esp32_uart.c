/**
 *****************************************************************************
 * @file    : user_esp32_uart.c
 * @brief   : ESP32 UART Application
 * @author  : Cao Jin
 * @date    : 28-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

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

/** @brief log output label. */
static const char *TAG = "Wi-Fi Application";

typedef struct 
{
    uart_port_t port;      
    gpio_num_t tx_io_num;  /* UART TX pin GPIO number. */
    gpio_num_t rx_io_num;  /* UART RX pin GPIO number. */
    gpio_num_t rts_io_num; /* UART RTS pin GPIO number. */
    gpio_num_t cts_io_num; /* UART CTS pin GPIO number. */
    uart_config_t config;  /* UART configuration parameters for uart_param_config function. */
    int rx_buffer_size;    /* UART RX ring buffer size. */
    int tx_buffer_size;    /* UART TX ring buffer size. */
}user_uart_param_t;


// static esp_err_t user_esp32_rs485_transmit_start();
// static esp_err_t user_esp32_rs485_transmit_stop();

/**
 * @brief Initialization UART driver.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_esp32_uart_tarnsmit(user_uart_param_t uart_param, uint8_t *buf, uint32_t len)
{
    /* Get UART mutex semaphore. */

    /* UART callback function before sending. */

    /* UART send data. */

    /* UART callback function after sending. */

    /* Release UART mutex semaphore. */

    return ESP_OK;
}

/**
 * @brief Initialization UART driver.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_esp32_uart_init(user_uart_param_t param)
{
    esp_err_t ret = ESP_OK;
    
    /* Set UART configuration parameters. */
    ret = uart_param_config(param.port, &param.config);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_num_%d set param configuration failed. Error Code: (%s).", param.port, esp_err_to_name(ret));
        return ret;
    }
    
    /* Set UART pin number. */
    ret = uart_set_pin(param.port, param.tx_io_num, param.rx_io_num, param.rts_io_num, param.cts_io_num);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "uart_num_%d set pin failed. Error Code: (%s).", param.port, esp_err_to_name(ret));
        return ret;
    }

    /* Install UART driver and set the UART to the default configuration. */
    ret = uart_driver_install(param.port, param.rx_buffer_size, param.tx_buffer_size, 0, NULL, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Driver install failed. Error Code: (%s).", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}
/**
 * @brief Deinitialization UART driver.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_esp32_uart_deinit(void)
{
    return ESP_OK;
}




