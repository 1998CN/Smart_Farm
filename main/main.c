#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "nvs_flash.h"

/*  
 * Name   : log_chip_info
 * Brief  : Print chip information
 * param  : None.
 * retval : None.
 * Date   : 2021-07-05
 */
// static void log_chip_info(void)
// {
//     esp_chip_info_t chip_info;
//     esp_chip_info(&chip_info);
//     ESP_LOGI(CHIP_INFO_TAG, "This is %s chip with %d CPU core(s), WiFi%s%s, silicon revision %d, %dMB %s flash",
//              CONFIG_IDF_TARGET,
//              chip_info.cores,
//              (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
//              (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
//              chip_info.revision,
//              spi_flash_get_chip_size() / (1024 * 1024),
//              (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
//     ESP_LOGI(CHIP_INFO_TAG, "Minimum free heap size: %d bytes", esp_get_minimum_free_heap_size());
// }

void app_main(void)
{
    /* Initialize NVS */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* print chip information */
    // log_chip_info();

    /* Initialize motor control pwm */
    user_mcpwm_init();
    /* Initialize RMT Rx Application */
    user_rmt_rx_init();
    /* Initialize RMT Tx Application */
    user_rmt_tx_init();
    /* Initialize Thread */

    rmt_item32_t *items = NULL;
    size_t length = 0;
    uint32_t addr = 0;
    uint32_t cmd = 0;
    bool repeat = false;

    while (1)
    {
        items = (rmt_item32_t *)xRingbufferReceive(g_rmt_rxrb_handle, &length, portMAX_DELAY);
        if (items)
        {
            length /= 4;
            if (g_ir_parser->input(g_ir_parser, items, length) == ESP_OK)
            {
                if (g_ir_parser->get_scan_code(g_ir_parser, &addr, &cmd, &repeat) == ESP_OK)
                {
                    if (addr == REMOTE_CONTROL_ADDRESS)
                    {
                        switch (cmd)
                        {
                        case REMOTE_CONTROL_A:
                            user_ws2812b_close();
                            break;
                        case REMOTE_CONTROL_B:                        
                            break;
                        case REMOTE_CONTROL_C:
                            user_ws2812b_open();
                            break;
                        case REMOTE_CONTROL_D:
                            break;
                        case REMOTE_CONTROL_E:
                            break;
                        case REMOTE_CONTROL_F:
                            break;

                        case REMOTE_CONTROL_0:
                            break;
                        case REMOTE_CONTROL_1:
                            break;
                        case REMOTE_CONTROL_2:
                            break;
                        case REMOTE_CONTROL_3:
                            break;
                        case REMOTE_CONTROL_4:
                            break;
                        case REMOTE_CONTROL_5:
                            break;
                        case REMOTE_CONTROL_6:
                            break;
                        case REMOTE_CONTROL_7:
                            break;
                        case REMOTE_CONTROL_8:
                            break;
                        case REMOTE_CONTROL_9:
                            break;

                        case REMOTE_CONTROL_UP:
                            user_mcpwm_set_forward();
                            break;
                        case REMOTE_CONTROL_DOWN:
                            user_mcpwm_set_backward();
                            break;
                        case REMOTE_CONTROL_LEFT:
                            user_mcpwm_set_leftward();
                            break;
                        case REMOTE_CONTROL_RIGHT:
                            user_mcpwm_set_rightward();
                            break;
                        case REMOTE_CONTROL_STOP:
                            user_mcpwm_set_stop();                            
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
            //after parsing the data, return spaces to ringbuffer.
            vRingbufferReturnItem(g_rmt_rxrb_handle, (void *)items);
        }
    }
}