/**
 *****************************************************************************
 * @file    : 74hc595.c
 * @brief   : Hardware 74hc595 driver
 * @author  : Cao Jin
 * @date    : 15-Oct-2021
 * @version : 1.0.0 
 *****************************************************************************
 */

#ifndef HARDWARE_74HC595_H
#define HARDWARE_74HC595_H

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief SN74HC595 GPIO Configuration.
  */
#define SN74HC595_SDA_PIN GPIO_NUM_32 /* ESP32_GPIO_NUM_X -> SN74HC595_SDA */
#define SN74HC595_RCK_PIN GPIO_NUM_18 /* ESP32_GPIO_NUM_X -> SN74HC595_RCK */
#define SN74HC595_SCK_PIN GPIO_NUM_19 /* ESP32_GPIO_NUM_X -> SN74HC595_SCK */

/**
  * @brief Initialize sn74hc595 chip.
  * 
  * @return -ESP_OK succeed.
  *         -other  failed.
  */
esp_err_t sn74hc595_init(void);

/**
  * @brief  Set the output value of the sn74hc595 device.
  * 
  * @param[IN]
  *     - _data output value 8/16/24/32 ... Bits.
  * 
  * @return
  *     - ESP_OK:   succeed.
  *     - ESP_FAIL: failed.
  *
  */
esp_err_t sn74hc595_send_data(void *_data);

#ifdef __cplusplus
}
#endif

#endif /* HARDWARE_74HC595_H */
/******************************** End of file *********************************/