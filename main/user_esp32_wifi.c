/**
 *****************************************************************************
 * @file    : user_esp32_wifi.c
 * @brief   : ESP32 Wi-Fi Application
 * @author  : Cao Jin
 * @date    : 27-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_log.h"

#include "user_esp32_wifi.h"
#include "user_esp32_mqtt.h"
#include "user_esp32_ota.h"

/** @brief Default Wi-Fi station mode configuration. */
#define USER_WIFI_STA_SSID              "QianKun_Board_Wi-Fi"   /* Default Wi-Fi Station Mode Account. */
#define USER_WIFI_STA_PWSD              "12345678"              /* Default Wi-Fi Station Mode Password. */
#define USER_WIFI_STA_MAXIMUM_NUMBER    (0U)                    /* Default Maximum Number of Wi-Fi Station Mode reconnections. */

/** @brief Default Wi-Fi Soft-AP mode configuration.  */
#define USER_WIFI_AP_SSID               "QianKun_Board_Wi-Fi"   /* Default Wi-Fi Soft-AP Mode Account. */
#define USER_WIFI_AP_PWSD               "12345678"              /* Default Wi-Fi Soft-AP Mode Password. */
#define USER_WIFI_AP_MAXIMUM_CONNECT    (5U)                    /* Default Maximum Number of Wi-Fi Soft-AP Mode connected devices. */

/** @brief Default Wi-Fi reconnect timer time-out in seconds. */
#define USER_WIFI_RECONNECT_SHORT_TIME  (10U)
#define USER_WIFI_RECONNECT_LONG_TIME   (30U)

/** @brief Longest Wi-Fi smartconfig service duration in seconds. */
#define USER_WIFI_SC_MAXIMUM_TIME       (60U)

/** @brief Error checking function macro definition. */
#define USER_WIFI_ESP_ERROR_CHECK(x)                                                                                  \
    do                                                                                                                \
    {                                                                                                                 \
        esp_err_t err_rc_ = (x);                                                                                      \
        if (err_rc_ != ESP_OK)                                                                                        \
        {                                                                                                             \
            ESP_LOGE("Wi-Fi ERROR", "%s-%d-%s. Error Code: (%s).", __FILE__, __LINE__, #x, esp_err_to_name(err_rc_)); \
            while (1);                                                                                                \
        }                                                                                                             \
    } while (0)

/** @brief log output label. */
static const char *TAG = "Wi-Fi Application";

/** @brief When set, means the Wi-Fi sta-mode connection is successful. */
static const EventBits_t USER_WIFI_STA_CONNECTION = BIT0;

/** @brief When set, means the Wi-Fi smartconfig configuration is successful. */
static const EventBits_t USER_WIFI_SC_RUNNING = BIT1;

/** @brief FreeRTOS Wi-Fi EventGroups handle . */
static EventGroupHandle_t wifi_event_group_handle = NULL;

/** @brief FreeRTOS Wi-Fi reconnect timer handle . */
static TimerHandle_t wifi_reconnect_timer_handle = NULL;

/** @brief Wi-Fi Station Mode Reconnect Retry Number. */
#if USER_WIFI_STA_MAXIMUM_NUMBER
static uint8_t wifi_reconnect_counter = 0;
#endif

/**
 * @brief  Wi-Fi Station Mode Reconnect Service Callback.
 *
 * @param pxTimers[IN] Timer callback handle.
 */
static void wifi_sta_timer_callback(TimerHandle_t pxTimers)
{
    /* Reconnection Wi-Fi. */
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Wi-Fi connect error. error code(%s).", esp_err_to_name(ret));
    }
}
/**
 * @brief Start Wi-Fi station mode reconnect service.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_start_wifi_reconnect_service(void)
{
    BaseType_t uxRet = pdPASS;

    /* Create Wi-Fi station mode reconnection timer. */
    if (wifi_reconnect_timer_handle == NULL)
    {
        wifi_reconnect_timer_handle = xTimerCreate("Wi-Fi Station Mode Timer",              /* Just a text name, not used by the kernel. */
                                                   (USER_WIFI_RECONNECT_SHORT_TIME * 1000), /* The timer period in ticks. */
                                                   pdFALSE,                                 /* The timers will auto-reload themselves when they expire. */
                                                   NULL,                                    /* Assign each timer a unique id equal to its array index. */
                                                   &wifi_sta_timer_callback);               /* Each timer calls the same callback when it expires. */
        if (wifi_reconnect_timer_handle == NULL)
        {
            ESP_LOGE(TAG, "Wi-Fi station mode reconnect timer create failure.");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Create Wi-Fi reconnect service.");
    }

#if USER_WIFI_STA_MAXIMUM_NUMBER
    /* If the current number of reconnections is less than the maximum number of reconnections,
       the reconnection will be performed after a short delay. */
    if (wifi_reconnect_counter < USER_WIFI_STA_MAXIMUM_NUMBER)
    {
        /* Accumulate Wi-Fi station mode reconnection counter. */
        wifi_reconnect_counter += 1;

        /* Set Wi-Fi station mode reconnection interval. */
        uxRet = xTimerChangePeriod(wifi_reconnect_timer_handle, pdMS_TO_TICKS(USER_WIFI_RECONNECT_SHORT_TIME * 1000));
        if (uxRet != pdPASS)
        {
            ESP_LOGE(TAG, "Reconnect Timer change period failed.");
            return ESP_FAIL;
        }

        /* Start Wi-Fi station mode reconnection timer. */
        uxRet = xTimerStart(wifi_reconnect_timer_handle, pdMS_TO_TICKS(10));
        if (uxRet != pdPASS)
        {
            ESP_LOGE(TAG, "Reconnect Timer start failed.");
            return ESP_FAIL;
        }
    }
    else
    {
        /* Clear Wi-Fi station mode reconnection counter. */
        wifi_reconnect_counter = 0;

        /* Set Wi-Fi station reconnection interval. */
        uxRet = xTimerChangePeriod(wifi_reconnect_timer_handle, pdMS_TO_TICKS(USER_WIFI_RECONNECT_LONG_TIME * 1000));
        if (uxRet != pdPASS)
        {
            ESP_LOGE(TAG, "Reconnect Timer change period failed.");
            return ESP_FAIL;
        }

        /* Start Wi-Fi station reconnection timer. */
        uxRet = xTimerStart(wifi_reconnect_timer_handle, pdMS_TO_TICKS(10));
        if (uxRet != pdFAIL)
        {
            ESP_LOGE(TAG, "Reconnect Timer start failed.");
            return ESP_FAIL;
        }
    }
#else
    /* Start Wi-Fi station mode reconnection timer. */
    uxRet = xTimerStart(wifi_reconnect_timer_handle, pdMS_TO_TICKS(10));
    if (uxRet != pdPASS)
    {
        ESP_LOGE(TAG, "Reconnect Timer start failed.");
        return ESP_FAIL;
    }
#endif /* USER_WIFI_STA_MAXIMUM_NUMBER */

    return ESP_OK;
}
/**
 * @brief Stop Wi-Fi station mode reconnect service.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_stop_wifi_reconnect_service(void)
{
    BaseType_t uxRet = pdPASS;

    if (wifi_reconnect_timer_handle == NULL)
    {
        return ESP_OK;
    }

    /* It seems useless, you can delete it. */
    // uxRet = xTimerIsTimerActive(wifi_reconnect_timer_handle);
    // if (uxRet == pdPASS)
    // {
    //     uxRet = xTimerStop(wifi_reconnect_timer_handle, pdMS_TO_TICKS(1000));
    //     if (uxRet != pdPASS)
    //     {
    //         ESP_LOGE(TAG, "Reconnect timer stop failed.");
    //         return ESP_FAIL;
    //     }
    // }

    uxRet = xTimerDelete(wifi_reconnect_timer_handle, pdMS_TO_TICKS(10));
    if (uxRet != pdPASS)
    {
        ESP_LOGE(TAG, "Delete reconnect timer failed.");
        return ESP_FAIL;
    }

    wifi_reconnect_timer_handle = NULL;

    return ESP_OK;
}
/**
 * @brief Start Wi-Fi smartconfig service.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_start_wifi_smartconfig_service(void)
{
    esp_err_t ret = ESP_OK;

    /* Start Wi-Fi smartconfig service. */
    EventBits_t uxBits = xEventGroupGetBits(wifi_event_group_handle);
    if (uxBits & USER_WIFI_SC_RUNNING)
    {
        return ESP_OK;
    }

    /* Set timeout of SmartConfig process. */
    ret = esp_esptouch_set_timeout(USER_WIFI_SC_MAXIMUM_TIME);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Smartconfig set timeout failed. Error code: (%s).", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    /* Set protocol type of SmartConfig. */
    ret = esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Smartconfig set type failed. Error code: (%s).", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    /* Start SmartConfig Service. */
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ret = esp_smartconfig_start(&cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Smartconfig start failed. Error code: (%s).", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    /* Set Wi-Fi EventGroup Smartconfig running bit. */
    xEventGroupSetBits(wifi_event_group_handle, USER_WIFI_SC_RUNNING);

    /* It seems useless, you can delete it. */
    ESP_LOGI(TAG, "SmartConfig start version: %s.", esp_smartconfig_get_version());

    return ESP_OK;
}
/**
 * @brief Stop Wi-Fi smartconfig service.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_stop_wifi_smartconfig_service(void)
{
    esp_err_t ret = ESP_OK;

    /* Start Wi-Fi smartconfig service. */
    EventBits_t uxBits = xEventGroupGetBits(wifi_event_group_handle);
    if (uxBits & USER_WIFI_SC_RUNNING)
    {
        /* Stop Wi-Fi smartconfig service. */
        ret = esp_smartconfig_stop();
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "Smartconfig stop failed. Error code: (%s).", esp_err_to_name(ret));
            return ESP_FAIL;
        }

        /* Clear Wi-Fi EventGroup Smartconfig running bit. */
        xEventGroupClearBits(wifi_event_group_handle, USER_WIFI_SC_RUNNING);
    }

    return ESP_OK;
}
/**
 * @brief  Wi-Fi Station Mode Event Group CallBack.
 *
 * @param args[IN] user data registered to the event.
 * @param event_base[IN] Event base for the handler.
 * @param event_id[IN] The id for the received event.
 * @param event_data[IN] The data for the event.
 */
static void wifi_sta_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_err_t ret = ESP_OK;

    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            /* Connect the Wi-Fi Station to the AP.  */
            ret = esp_wifi_connect();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "ESP_WIFI_CONNECT ERROR CODE: (%s).", esp_err_to_name(ret));
            }
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            /* Disconnected reason. */
            wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;

            ESP_LOGI(TAG, "The Wi-Fi station mode is disconnected. Reason:%d.", disconnected->reason);

            // /* disconnected reason 8-> OTA upgrade successful esp32 restart. */
            // if (disconnected->reason != 8)
            // {
            //     /* Delete HTTP(S) OTA Service. */
            //     user_esp32_delete_ota_service();
            // }

            /* Clear Wi-Fi station mode Event Group Connection flag bit. */
            xEventGroupClearBits(wifi_event_group_handle, USER_WIFI_STA_CONNECTION);

            /* Start Wi-Fi station mode reconnet service. */
            user_start_wifi_reconnect_service();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Start Wi-Fi reconnect service failed.");
            }

            /* Start Wi-Fi smartconfig service. */
            ret = user_start_wifi_smartconfig_service();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "Start Wi-Fi smartconfig service failed.");
            }
        }
    }
    else if (event_base == SC_EVENT)
    {
        if (event_id == SC_EVENT_GOT_SSID_PSWD)
        {
            ESP_LOGI(TAG, "Smartconfig got Wi-Fi SSID and PWSD.");

            /* Wi-Fi smartconfig configuration param. */
            smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;

            /* Wi-Fi station mode configuration param. */
            wifi_config_t wifi_sta_config;
            memset(&wifi_sta_config, 0, sizeof(wifi_config_t));

            /* Set Wi-Fi station mode configuration parameter. */
            memcpy(wifi_sta_config.sta.ssid, evt->ssid, sizeof(wifi_sta_config.sta.ssid));
            memcpy(wifi_sta_config.sta.password, evt->password, sizeof(wifi_sta_config.sta.password));
            wifi_sta_config.sta.bssid_set = evt->bssid_set;
            if (wifi_sta_config.sta.bssid_set == true)
            {
                memcpy(wifi_sta_config.sta.bssid, evt->bssid, sizeof(wifi_sta_config.sta.bssid));
            }

            /* It seems useless, you can delete it.
             * if Wi-Fi station mode is connected, disconnect the Wi-Fi station mode connection. */
            BaseType_t uxBits = xEventGroupGetBits(wifi_event_group_handle);
            if (uxBits & USER_WIFI_STA_CONNECTION)
            {
                USER_WIFI_ESP_ERROR_CHECK(esp_wifi_disconnect());
            }

            /* Configuration Wi-Fi station mode parameters. */
            USER_WIFI_ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

            /* Start Wi-Fi station mode connection. */
            // esp_wifi_connect(); /* Seems useless. */
        }
        else if (event_id == SC_EVENT_SEND_ACK_DONE)
        {
            ESP_LOGI(TAG, "Smartconfig finish.");

            /* Stop Smartconfig service. */
            user_stop_wifi_smartconfig_service();
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            /* Set Wi-Fi event group station mode connection flag bit. */
            xEventGroupSetBits(wifi_event_group_handle, USER_WIFI_STA_CONNECTION);

            /* Get Wi-Fi station mode ip. */
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Get IP:" IPSTR, IP2STR(&event->ip_info.ip));

            /* Stop Wi-Fi reconnect service.  */
            user_stop_wifi_reconnect_service();

            /* Create MQTT client. */
            user_esp32_create_mqtt_client();

            /* Create HTTP(S) OTA Service. */
            user_esp32_create_ota_service();
        }
    }
}
/**
 * @brief Initialize ESP32 Wi-Fi Station Mode.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
esp_err_t user_esp32_wifi_init(void)
{
    /* Set log level for given tag. */
    esp_log_level_set("smartconfig", ESP_LOG_ERROR);
    esp_log_level_set("wifi", ESP_LOG_ERROR);

    /* Create Wi-Fi Station Mode Event Group. */
    wifi_event_group_handle = xEventGroupCreate();
    if (wifi_event_group_handle == NULL)
    {
        ESP_LOGE(TAG, "Wi-Fi Station Mode Event Group Create Failure.");
        return ESP_FAIL;
    }

    /* Initialize the underlying TCP/IP stack. */
    USER_WIFI_ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop.  */
    USER_WIFI_ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Creates Default WIFI Station Mode. */
    esp_netif_create_default_wifi_sta();

    /* Initialize Wi-Fi .*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    USER_WIFI_ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register All Wi-Fi Event ID to the system event loop. */
    USER_WIFI_ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));
    /* Register Wi-Fi Station Mode Got IP Event ID to the system event loop. */
    USER_WIFI_ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL));
    /* Register Wi-Fi All SmartConfig Event ID to the system event loop. */
    USER_WIFI_ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));

    /* Wi-Fi station mode configuration param. */
    wifi_config_t wifi_sta_config;
    memset(&wifi_sta_config, 0, sizeof(wifi_config_t));

    /* Get Wi-Fi configuration from nvs flash. */
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_sta_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Get Wi-Fi station mode configuration from NVS flash failed. Error Code: (%s)", esp_err_to_name(ret));
        ESP_LOGI(TAG, "Set Wi-Fi station mode default parameter.");

        /* Configuration Default Wi-Fi Station Mode Parameter. */
        memcpy(wifi_sta_config.sta.ssid, USER_WIFI_STA_SSID, sizeof(USER_WIFI_STA_SSID));
        memcpy(wifi_sta_config.sta.password, USER_WIFI_STA_PWSD, sizeof(USER_WIFI_STA_PWSD));
        USER_WIFI_ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        USER_WIFI_ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    }
    else
    {
        /* if the Wi-Fi account saved in the nvs flash is empty, set the Wi-Fi configuration to the default value. */
        if (strlen((const char *)wifi_sta_config.sta.ssid) == 0)
        {
            ESP_LOGI(TAG, "Wi-Fi Station Mode configuration is empty.");
            ESP_LOGI(TAG, "Set Wi-Fi Station Mode default parameter.");

            /* Configuration Default Wi-Fi Station Mode Parameter. */
            memcpy(wifi_sta_config.sta.ssid, USER_WIFI_STA_SSID, sizeof(USER_WIFI_STA_SSID));
            memcpy(wifi_sta_config.sta.password, USER_WIFI_STA_PWSD, sizeof(USER_WIFI_STA_PWSD));
            USER_WIFI_ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            USER_WIFI_ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
        }
    }

    ESP_LOGI(TAG, "Connection SSID: %s.", wifi_sta_config.sta.ssid);
    ESP_LOGI(TAG, "Connection PWSD: %s.", wifi_sta_config.sta.password);

    /* Start WiFi according to Current Configuration. */
    USER_WIFI_ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}
/**
 * @brief Get Wi-Fi connection status.
 *
 * @return  - ESP32_WIFI_NOT_INIT
 *          - ESP32_WIFI_CONNECTED
 *          - ESP32_WIFI_DISCONNECTED
 */
esp_wifi_status_t user_esp32_wifi_get_status(void)
{
    if (wifi_event_group_handle == NULL)
    {
        return ESP32_WIFI_NOT_INIT;
    }

    /* Get ESP32 Wi-Fi connection status. */
    EventBits_t uxBits = xEventGroupGetBits(wifi_event_group_handle);
    if (uxBits & USER_WIFI_STA_CONNECTION)
    {
        return ESP32_WIFI_CONNECTED;
    }

    return ESP32_WIFI_DISCONNECTED;
}
/******************************** End of File *********************************/