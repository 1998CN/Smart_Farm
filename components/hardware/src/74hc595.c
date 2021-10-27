/**
 *****************************************************************************
 * @file    : 74hc595.c
 * @brief   : hardware esp32 74hc595 driver application
 * @author  : Cao Jin
 * @date    : 15-Oct-2021
 * @version : 1.0.0 
 *****************************************************************************
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "sys/unistd.h"

#include "74hc595.h"

#define SET_SN74HC595_SDA_H() gpio_set_level(SN74HC595_SDA_PIN, 1)
#define SET_SN74HC595_SDA_L() gpio_set_level(SN74HC595_SDA_PIN, 0)

#define SET_SN74HC595_RCK_H() gpio_set_level(SN74HC595_RCK_PIN, 1)
#define SET_SN74HC595_RCK_L() gpio_set_level(SN74HC595_RCK_PIN, 0)

#define SET_SN74HC595_SCK_H() gpio_set_level(SN74HC595_SCK_PIN, 1)
#define SET_SN74HC595_SCK_L() gpio_set_level(SN74HC595_SCK_PIN, 0)

esp_err_t sn74hc595_init(void)
{
    esp_err_t ret = ESP_OK;
    gpio_config_t io_config;

    io_config.pin_bit_mask = (1ULL << SN74HC595_SDA_PIN) | (1ULL << SN74HC595_RCK_PIN) | (1ULL << SN74HC595_SCK_PIN);
    io_config.mode = GPIO_MODE_OUTPUT;
    io_config.pull_down_en =  GPIO_PULLDOWN_ENABLE;
    io_config.pull_up_en = GPIO_PULLUP_DISABLE;
    io_config.intr_type = GPIO_PIN_INTR_DISABLE;

    ret = gpio_config(&io_config);

    SET_SN74HC595_SDA_L();
    SET_SN74HC595_RCK_L();
    SET_SN74HC595_SCK_L();

    return ret;
}

esp_err_t sn74hc595_send_data(void *_data)
{
    uint8_t *send_data = NULL;
    uint8_t send_length = 0;

    send_data = (uint8_t *)_data;
    send_length = (sizeof(send_data) / 8);

    if (send_length == 0)
    {
        return false;
    }

    for (uint8_t i = 0; i < send_length; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            SET_SN74HC595_SCK_L();

            if (((send_data[i] << j) & 0x8000) != 0)
            {
                SET_SN74HC595_SDA_H();
            }
            else
            {
                SET_SN74HC595_SDA_L();
            }

            SET_SN74HC595_SCK_H();
        }
    }
    SET_SN74HC595_RCK_L();
    usleep(10);
    SET_SN74HC595_RCK_H();

    return true;
}
/******************************** End of file *********************************/