/**
 *****************************************************************************
 * @file    : user_esp32_ota.c
 * @brief   : ESP32 OAT service Application
 * @author  : Cao Jin
 * @date    : 20-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include "esp_err.h"
#include "esp_log.h"

#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "user_esp32_ota.h"


#define DEFAULT_HTTP_OTA_BROKER_URL "https://192.168.2.106:8070/hello-world.bin"

#define DEFAULT_HTTP_OTA_REV_TIMEOUT (100U)
#define DEFAULT_HTTP_REQUEST_SIZE       (100U)


/** @brief log output label. */
static const char *TAG = "OTA Application";

extern const uint8_t server_cert_pem_start[] asm("_binary_ota_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ota_ca_cert_pem_end");

/**
 * @brief Verify the received new firmware.
 */
void user_esp32_ota_data_verification(void)
{

}

esp_err_t user_ota_validate_image_header(esp_app_desc_t *new_app_info)
{
    return ESP_OK;
}

static esp_err_t  _http_client_init_cb(esp_http_client_handle_t http_client)
{
    esp_err_t ret = ESP_OK;
    /* Uncomment to add custom headers to HTTP request */
    // ret = esp_http_client_set_header(http_client, "Custom-Header", "Value");
    return ret;
}

void user_create_ota_service(void)
{
    esp_err_t ret = ESP_OK;

    /* Information. */
    ESP_LOGI(TAG, "Create OTA service.");

    /* Configuration HTTP client information. */
    esp_http_client_config_t http_client_config = {
        .url = DEFAULT_HTTP_OTA_BROKER_URL,
        .cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = DEFAULT_HTTP_OTA_REV_TIMEOUT,
        .keep_alive_enable = true,
    };

    /* Configuration HTTPS OTA information. */
    esp_https_ota_config_t ota_config = {
        .http_config = &http_client_config,
        .http_client_init_cb = _http_client_init_cb,
        .partial_http_download = true,
        .max_http_request_size = DEFAULT_HTTP_REQUEST_SIZE,
    };

    esp_https_ota_handle_t https_ota_handle = NULL;
    ret = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (ret != ESP_OK)
    {
        /* Information. */
        ESP_LOGE(TAG, "esp_https_ota_begin failed.");
    }

    esp_app_desc_t app_desc;
    ret = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (ret != ESP_OK)
    {
        /* Information. */
        ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed.");
    }
    ret = user_ota_validate_image_header(&app_desc);
    if (ret != ESP_OK)
    {
        /* Information. */
        ESP_LOGE(TAG, "user_ota_validate_image_header failed");
    }
}

