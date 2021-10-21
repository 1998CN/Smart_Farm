/**
 *****************************************************************************
 * @file    : user_esp32_mqtt.h
 * @brief   : ESP32 MQTT Application
 * @author  : Cao Jin
 * @date    : 19-Oct-2021
 * @version : 1.0.0 
 *****************************************************************************
 */

#ifndef USER_ESP32_MQTT_H
#define USER_ESP32_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Create MQTT client
 * 
 * @return - ESP_OK: succeed
 *         - others: failure
 */
esp_err_t user_create_mqtt_client(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_ESP32_MQTT_H */
/******************************** End of File *********************************/