/**
 *****************************************************************************
 * @file   : user_esp32_wifi.h
 * @brief  : ESP32 Wi-Fi Atapplication
 * @author : Cao Jin
 * @date   : 02-Oct-2021
 *****************************************************************************
 */

#ifndef __USER_ESP32_WIFI_H
#define __USER_ESP32_WIFI_H
/* Includes -----------------------------------------------------------------*/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"

#include "esp_log.h"
/* C PlusPlus Define --------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/* Exported define ----------------------------------------------------------*/
#define DEFAULT_ESP_WIFI_SSID           "QianKun_Board_WiFi"    /* Wi-Fi Station Mode Default Connection SSID. */
#define DEFAULT_ESP_WIFI_PWSD           "12345678"              /* Wi-Fi Station Mode Default Connection PWSD. */
#define DEFAULT_ESP_WIFI_RETRY          (3U)                    /* Wi-Fi Station Mode Default Maximum Retry. */

#define DEFAULT_ESP_WIFI_RETRY_STIME    (1U)                    /*  */
#define DEFAULT_ESP_WIFI_RETRY_LTIME    (10U)                   /*  */
/* Exported macro -----------------------------------------------------------*/
/* Exported typedef ---------------------------------------------------------*/
/* Exported variables -------------------------------------------------------*/
/* Exported function prototypes ---------------------------------------------*/
/* C PlusPlus Define --------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif /* __USER_ESP32_WIFI_H */
/******************************** End of File *********************************/