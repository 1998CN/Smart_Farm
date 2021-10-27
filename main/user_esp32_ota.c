/**
 *****************************************************************************
 * @file    : user_esp32_ota.c
 * @brief   : ESP32 OAT service Application
 * @author  : Cao Jin
 * @date    : 27-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_efuse.h"

#include "user_esp32_ota.h"

/** @brief FreeRTOS HTTP(S) OTA Task configuration. */
#define HTTPS_OTA_TASK_STACK_DEPTH                      (4 * 1024U)
#define HTTPS_OTA_TASK_PRIORITY                         (1U)


/** @brief When set, means OTA supports downloaded over multiple HTTP requests. */
#define ESP32_OTA_PARTIAL_HTTP_DOWNLOAD_ENABLE          (1U)

/** @brief When set, means OTA supports firmware version check. */

#define ESP32_OTA_VERSION_CHECK_ENABLE                  (1U)

/** @brief When set, means OTA supports secure version check. */
#define ESP32_OTA_BOOTLOADER_APP_ANTI_ROLLBACK_ENABLE   (0U)


/** @brief Complete HTTP(S) OTA broker URL. */
#define ESP32_HTTP_OTA_BROKER_URL                       "http://47.102.193.111/smart_farm.bin" 
                                                        //"HTTP(S)://192.168.16.128/smart_farm.bin"
/** @brief Network timeout in milliseconds. */
#define ESP32_HTTP_OTA_REV_TIMEOUT                      (5000U)

/** @brief Maximum request size for partial HTTP download. */
#if ESP32_OTA_PARTIAL_HTTP_DOWNLOAD_ENABLE
#define ESP32_HTTP_REQUEST_SIZE                         (16384U)
#endif

/** @brief Log output label. */
static const char *TAG = "OTA Application";

/** @brief When set, means start HTTP(S) OTA upgrade. */
static const EventBits_t HTTPS_OTA_USER_UPGRADE_FL = BIT0;

/** @brief FreeRTOS HTTP(S) OTA handles. */
static TaskHandle_t https_ota_task_handle = NULL;
static EventGroupHandle_t https_ota_event_groups_handle = NULL;

/** @brief HTTP(S) OTA SSL Certificate. */
extern const uint8_t server_cert_pem_start[] asm("_binary_ota_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ota_ca_cert_pem_end");

/**
 * @brief ESP32 HTTP(S) OTA validata image header.
 * 
 * @param upgrade_app_info[IN] Image information to be updated. 
 * 
 * @return  - ESP_OK    succeed.
 *          - other     failed.
 */
static esp_err_t https_ota_validate_image_header(esp_app_desc_t *upgrade_app_info)
{
    esp_err_t ret = ESP_OK;

    // if (upgrade_app_info == NULL)
    // {
    //     return ESP_ERR_INVALID_ARG;
    // }

    /* Get partition info of currently running app. */
    const esp_partition_t *running_partition = esp_ota_get_running_partition();

    /* Get description for the requested app partition. */
    esp_app_desc_t running_app_info;
    ret = esp_ota_get_partition_description(running_partition, &running_app_info);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Running app info get failed.");
        
        return ret;
    }

    /* Information-> Firmware version. */
    ESP_LOGI(TAG, "Current firmware version: %s.", running_app_info.version);
    ESP_LOGI(TAG, "Upgrade firmware version: %s.", upgrade_app_info->version);

#if ESP32_OTA_VERSION_CHECK_ENABLE
    /* Add version control to OTA updates. */
    if (memcmp(running_app_info.version, upgrade_app_info->version, sizeof(running_app_info.version)) == 0)
    {
        ESP_LOGI(TAG, "Current running firmware version is the same as a new. We will not continue the update.");
        return ESP_FAIL;
    }
#endif

#if ESP32_OTA_BOOTLOADER_APP_ANTI_ROLLBACK_ENABLE
    /**
     * Secure version check from firmware image header prevents subsequent download and flash write of
     * entire firmware image. However this is optional because it is also taken care in API
     * esp_https_ota_finish at the end of OTA update procedure.
     */
    const uint32_t running_secure_version = esp_efuse_read_secure_version();
    if (upgrade_app_info->secure_version < running_secure_version)
    {
        ESP_LOGI(TAG, "New firmware security version is less than eFuse programmed, %d < %d.",
                 upgrade_app_info->secure_version, running_secure_version);
        return ESP_FAIL;
    }
#endif

    return ESP_OK;
}
/**
 * @brief ESP32 HTTP(S) OTA Service.
 * 
 * @return  - ESP_OK    succeed.
 *          - other     failed.
 */
static esp_err_t https_ota_upgrade(void)
{
    esp_err_t ret = ESP_OK;
    
    /* Configuration HTTP client information. */
    esp_http_client_config_t http_client_config = {
        .url = ESP32_HTTP_OTA_BROKER_URL,          /* HTTP URL, the information on the URL is most important, it overrides the other fields below. */
        .cert_pem = (char *)server_cert_pem_start, /* SSL server certification, PEM format as string. */
        .timeout_ms = ESP32_HTTP_OTA_REV_TIMEOUT,  /* Network timeout in milliseconds. */
        .keep_alive_enable = true,                 /* When it's true, Means enable keep-alive timeout. */
#ifdef OTA_SKIP_CERT_COMMON_NAME_CHECK
        .skip_cert_common_name_check = true, /* When it's true, Means skip any validation of server certificate CN field. */
#endif
    };

    /* Configuration HTTP(S) OTA information. */
    esp_https_ota_config_t ota_config = {
        .http_config = &http_client_config, /* ESP HTTP client configuration */
        .http_client_init_cb = NULL,        /* Callback after ESP HTTP client is initialised */
        .bulk_flash_erase = false,          /* When it's true, Means erase entire flash partition during initialization.
                                             * Otherwise flash partition is erased during write operation and in chunk of 4K sector size.*/
#if ESP32_OTA_PARTIAL_HTTP_DOWNLOAD_ENABLE
        .partial_http_download = true,                    /* When it's true, Means enable firmware image to be downloaded over multiple HTTP requests. */
        .max_http_request_size = ESP32_HTTP_REQUEST_SIZE, /* Maximum request size for partial HTTP download. */
#endif
    };

    /* Start HTTP(S) OTA Firmware upgrade. */
    esp_https_ota_handle_t https_ota_handle = NULL;
    ret = esp_https_ota_begin(&ota_config, &https_ota_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_https_ota_begin failed. Error Code:(%s).", esp_err_to_name(ret));
        
        return ret;
    }

    /* Reads app description from image header. The app description provides
       information like the “Firmware version” of the image. */
    esp_app_desc_t app_desc;
    memset(&app_desc, 0, sizeof(esp_app_desc_t));
    ret = esp_https_ota_get_img_desc(https_ota_handle, &app_desc);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_https_ota_get_img_desc failed. Error Code:(%s).", esp_err_to_name(ret));

        /* Clean-up HTTP(S) OTA Firmware upgrade and close HTTP(S) connection. */
        esp_https_ota_abort(https_ota_handle);

        return ret;
    }
    /* Determine whether Firmware version needs to be updated. */
    ret = https_ota_validate_image_header(&app_desc);
    if (ret != ESP_OK)
    {
        /* Clean-up HTTP(S) OTA Firmware upgrade and close HTTP(S) connection. */
        esp_https_ota_abort(https_ota_handle);

        return ret;
    }

    ESP_LOGI(TAG, "Read image data from HTTP(S) stream and write it to OTA partition.");
    while (1)
    {
        /* Read image data from HTTP(S) stream and write it to OTA partition. */
        ret = esp_https_ota_perform(https_ota_handle);
        if (ret != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
        {
            break;
        }

        /* Information - OTA image total bytes read so far. */
        // ESP_LOGI(TAG, "Image bytes read: %d.", esp_https_ota_get_image_len_read(https_ota_handle));
    }
    ESP_LOGI(TAG, "End of reading");

    /* Checks if complete data was received or not. */
    if (esp_https_ota_is_complete_data_received(https_ota_handle) != true)
    {
        ESP_LOGE(TAG, "Complete data was not received.");
        
        /* Clean-up HTTP(S) OTA Firmware upgrade and close HTTP(S) connection. */
        esp_https_ota_abort(https_ota_handle);
        
        return ESP_FAIL;
    }

    /* Clean-up HTTP(S) OTA Firmware upgrade and close HTTP(S) connection. */
    esp_err_t ota_finish_ret = esp_https_ota_finish(https_ota_handle);
    if ((ret == ESP_OK) && (ota_finish_ret == ESP_OK))
    {
        ESP_LOGI(TAG, "OTA upgrade successful. Restarting.");

        /* Wait 3 seconds, and the system restarts. */
        for (int i = 3; i > 0; i--)
        {
            ESP_LOGI(TAG, "Restarting in %d seconds...", i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        /* Restart PRO and APP CPUs. */
        esp_restart();

        return ESP_OK;
    }
    else
    {
        if (ota_finish_ret == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "OTA upgrade failed, 0x%X.", ota_finish_ret);
    }

    return ESP_FAIL;
}
/**
 * @brief ESP32 OTA HTTP(S) Service Task.
 * 
 * @param pvParameters[IN] Task create accept parameters. 
 */
static void https_ota_task(void *pvParameters)
{
    esp_err_t ret = ESP_OK;

    /* Create ESP32 HTTP(S) OTA service event group. */
    if (https_ota_event_groups_handle == NULL)
    {
        https_ota_event_groups_handle = xEventGroupCreate();
        if (https_ota_event_groups_handle == NULL)
        {
            ESP_LOGE(TAG, "HTTP(S) OTA EventGroups Creation Failed.");
        }
    }

    while (1)
    {
        /* Block to wait for one or more bits to be set within a previously created event group. */
        xEventGroupWaitBits(https_ota_event_groups_handle, HTTPS_OTA_USER_UPGRADE_FL, pdTRUE, pdFALSE, portMAX_DELAY);

        /* Start HTTP(S) OTA upgrade. */
        ret = https_ota_upgrade();
        if(ret != ESP_OK)
        {
            ESP_LOGE(TAG, "OTA upgrade failed.");
        }
    }
}
/**
 * @brief Verify the received new firmware.
 * 
 * @return  -ESP_OK     succeed.
 *          -ESP_FAIL   failed.
 */
esp_err_t user_esp32_ota_data_verification(void)
{

    return ESP_OK;
}
/**
 * @brief Create ESP32 HTTP(S) OTA service.
 * 
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_create_ota_service(void)
{
    /* Create ESP32 HTTP(S) OTA service task. */
    if (https_ota_task_handle == NULL)
    {
        BaseType_t uxBits = xTaskCreate(https_ota_task,             /* Pointer to the task entry function. */
                                        "HTTP(S) ota task",         /*  Descriptive name for the task. */
                                        HTTPS_OTA_TASK_STACK_DEPTH, /* The size of the task stack specified as the number of bytes. */
                                        NULL,                       /* Pointer that will be used as the parameter for the task being created. */
                                        HTTPS_OTA_TASK_PRIORITY,    /* The priority at which the task should run.  */
                                        &https_ota_task_handle);    /* Used to pass back a handle by which the created task can be referenced. */
        if (uxBits != pdPASS)
        {
            ESP_LOGE(TAG, "HTTP(S) OTA Task Creation Failed.");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "HTTP(S) OTA service created successfully.");
    }

    ESP_LOGI(TAG, "HTTP(S) OTA service has been created.");

    return ESP_OK;
}
/**
 * @brief Start ESP32 HTTP(S) OTA service.
 * 
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_start_ota_service(void)
{
    /* Check HTTP(S) OTA event groups handle. */
    if(https_ota_event_groups_handle == NULL)
    {
        ESP_LOGE(TAG, "Start HTTP(S) OTA service failed. Please create OTA service first.");
        return ESP_FAIL;
    }

    /* Check HTTP(S) OTA task running status. */
    EventBits_t uxBits = xEventGroupGetBits(https_ota_event_groups_handle);
    if(uxBits & HTTPS_OTA_USER_UPGRADE_FL)
    {
        ESP_LOGE(TAG, "Start HTTP(S) OTA service failed. HTTP(S) OTA service is running.");
        return ESP_FAIL;
    }

    /* Start HTTP(S) OTA upgrade. */
    xEventGroupSetBits(https_ota_event_groups_handle, HTTPS_OTA_USER_UPGRADE_FL);

    return ESP_OK;
}
/**
 * @brief Delete ESP32 HTTP(S) OTA service.
 * 
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_delete_ota_service(void)
{
    /* Delete HTTP(S) OTA Service EventGroups. */
    if (https_ota_event_groups_handle != NULL)
    {
        vEventGroupDelete(https_ota_event_groups_handle);
        https_ota_event_groups_handle = NULL;
    }

    /* Delete HTTP(S) OTA Service Task. */
    if (https_ota_task_handle != NULL)
    {
        vTaskDelete(https_ota_task_handle);
        https_ota_task_handle = NULL;
    }

    return ESP_OK;
}
/******************************** End of File *********************************/