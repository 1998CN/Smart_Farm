/**
 *****************************************************************************
 * @file    : user_esp32_wifi.c
 * @brief   : ESP32 Wi-Fi Application
 * @author  : Cao Jin
 * @date    : 27-Oct-2021
 * @version : 1.0.0 
 *****************************************************************************
 */

#ifndef USER_ESP32_WIFI_H
#define USER_ESP32_WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Wi-Fi connection status. */
typedef enum
{
    ESP32_WIFI_NOT_INIT,
    ESP32_WIFI_CONNECTED,
    ESP32_WIFI_DISCONNECTED
} esp_wifi_status_t;

esp_err_t user_esp32_wifi_init(void);
esp_wifi_status_t user_esp32_wifi_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_ESP32_WIFI_H */
/******************************** End of File *********************************/