/**
 *****************************************************************************
 * @file    : user_esp32_uart.h
 * @brief   : ESP32 UART Application
 * @author  : Cao Jin
 * @date    : 28-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#ifndef USER_ESP32_UART_H
#define USER_ESP32_UART_H

#ifdef __cplusplus
extern "C" {
#endif

// typedef struct
// {
//     uart_port_t port;             /* UART port number, the max port number is (UART_NUM_MAX -1). */
//     QueueHandle_t queue;          /* UART queue handle,  */
//     SemaphoreHandle_t semaphore;  /* UART queue handle, */
//     void (*transmit_start)(void); /* UART Callback function before sending data. */
//     void (*transmit_stop)(void);  /* UART Callback function after data is sent. */
// } user_uart_param_t;

// esp_err_t user_esp32_uart_init(user_uart_param_t param);

#ifdef __cplusplus
}
#endif

#endif /* USER_ESP32_UART_H */
/******************************** End of File *********************************/