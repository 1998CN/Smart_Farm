/**
 *****************************************************************************
 * @file    : user_esp32_ota.h
 * @brief   : ESP32 OAT service Application
 * @author  : Cao Jin
 * @date    : 27-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#ifndef USER_ESP32_OTA_H
#define USER_ESP32_OTA_H

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t user_esp32_ota_data_verification(void);
esp_err_t user_create_ota_service(void);
esp_err_t user_start_ota_service(void);
esp_err_t user_delete_ota_service(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_ESP32_OTA_H */
/******************************** End of File *********************************/