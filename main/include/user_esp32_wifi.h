/**
 *****************************************************************************
 * @file    : user_esp32_wifi.c
 * @brief   : ESP32 Wi-Fi Application
 * @author  : Cao Jin
 * @date    : 15-Oct-2021
 * @version : 1.0.0 
 *****************************************************************************
 */

#ifndef USER_ESP32_WIFI_H
#define USER_ESP32_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_WIFI_STA_SSID "QianKun_Board_Wi-Fi" /* Default Wi-Fi Station Mode Account. */
#define DEFAULT_WIFI_STA_PWSD "12345678"            /* Default Wi-Fi Station Mode Password. */
#define DEFAULT_WIFI_STA_MAXIMUM_NUMBER (0U)        /* Default Maximum Number of Wi-Fi Station Mode reconnections. */
#define DEFAULT_WIFI_STA_RETRY_SHORT_TIME (5U)      /* Default Time interval for reconnecting when Wi-Fi Station Mode Connection is Disconnected. */
#define DEFAULT_WIFI_STA_RETRY_LONG_TIME (10U)      /* Default Time interval for reconnecting when Wi-Fi Station Mode Connection is Disconnected.*/

#define DEFAULT_WIFI_AP_SSID "QianKun_Board_Wi-Fi"  /* Default Wi-Fi Soft-AP Mode Account. */
#define DEFAULT_WIFI_AP_PWSD "12345678"             /* Default Wi-Fi Soft-AP Mode Password. */
#define DEFAULT_WIFI_AP_MAXIMUM_CONNECT (5U)        /* Default Maximum Number of Wi-Fi Soft-AP Mode connected devices. */

/**
 * @brief  Initialize the Wi-Fi station mode.
 *
 * @return
 *     - ESP_OK: succeed
 *     - others: failure
 */
esp_err_t user_esp32_wifi_init(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_ESP32_WIFI_H */
/******************************** End of File *********************************/