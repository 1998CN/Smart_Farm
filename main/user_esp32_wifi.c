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

/** @brief FreeRTOS Wi-Fi reconnection Task configuration. */
#define WIFI_STA_TASK_STACK_DEPTH   (4 * 1024)
#define WIFI_STA_TASK_PRIORITY      (4)

/** @brief FreeRTOS Wi-Fi smartconfig Task configuration. */
#define WIFI_SC_TASK_STACK_DEPTH    (4 * 1024)
#define WIFI_SC_TASK_PRIORITY       (2)

/** @brief Longest Wi-Fi smartconfig service duration. */
#define WIFI_SC_MAXIMUM_TIME        (60U)

/** @brief Default Wi-Fi station mode configuration. */
#define DEFAULT_WIFI_STA_SSID "QianKun_Board_Wi-Fi" /* Default Wi-Fi Station Mode Account. */
#define DEFAULT_WIFI_STA_PWSD "12345678"            /* Default Wi-Fi Station Mode Password. */
#define DEFAULT_WIFI_STA_MAXIMUM_NUMBER (0U)        /* Default Maximum Number of Wi-Fi Station Mode reconnections. */
#define DEFAULT_WIFI_STA_RETRY_SHORT_TIME (10U)      /* Default Time interval for reconnecting when Wi-Fi Station Mode Connection is Disconnected. */
#define DEFAULT_WIFI_STA_RETRY_LONG_TIME (30U)      /* Default Time interval for reconnecting when Wi-Fi Station Mode Connection is Disconnected.*/

/** @brief Default Wi-Fi Soft-AP mode configuration.  */
#define DEFAULT_WIFI_AP_SSID "QianKun_Board_Wi-Fi" /* Default Wi-Fi Soft-AP Mode Account. */
#define DEFAULT_WIFI_AP_PWSD "12345678"            /* Default Wi-Fi Soft-AP Mode Password. */
#define DEFAULT_WIFI_AP_MAXIMUM_CONNECT (5U)       /* Default Maximum Number of Wi-Fi Soft-AP Mode connected devices. */

/** @brief Error checking function macro definition. */
#define WIFI_ESP_ERROR_CHECK(x)                                                                                       \
    do                                                                                                                \
    {                                                                                                                 \
        esp_err_t err_rc_ = (x);                                                                                      \
        if (err_rc_ != ESP_OK)                                                                                        \
        {                                                                                                             \
            ESP_LOGE("Wi-Fi ERROR", "%s-%d-%d. Error Code: (%s).", __FILE__, __LINE__, #x, esp_err_to_name(err_rc_)); \
            while(1);
        }                                                                                                             \
    } while (0)

/** @brief log output label. */
static const char *TAG = "Wi-Fi Application";

/** @brief When set, means the Wi-Fi sta-mode connection is successful. */
static const EventBits_t WIFI_USER_STA_CONNECTION_FL = BIT0;

/** @brief When set, means the Wi-Fi sta-mode reconnection is enabled. */
static const EventBits_t WIFI_USER_STA_RECONNECT_EN = BIT1;

/** @brief When set, means the Wi-Fi sta-mode reconnection is enabled.  */
static const EventBits_t WIFI_USER_STA_RECONNECT_FL = BIT2;

/** @brief When set, means the Wi-Fi smartconfig configuration is successful. */
static const EventBits_t WIFI_USER_SC_FINISH_FL = BIT3;

/** @brief When set, means the Wi-Fi smartconfig configuration is time out.  */
static const EventBits_t WIFI_USER_SC_TIMEOUT_FL = BIT4;


/** @brief FreeRTOS Wi-Fi handles. */
static EventGroupHandle_t wifi_event_group_handle = NULL;
static TaskHandle_t wifi_sta_task_handle = NULL;
static TaskHandle_t wifi_sc_task_handle = NULL;
static TimerHandle_t wifi_sta_timer_handle = NULL;
static TimerHandle_t wifi_sc_timer_handle = NULL;
static uint8_t wifi_sta_timer_id = 1;
static uint8_t wifi_sc_timer_id = 2;

/** @brief Wi-Fi Station Mode Reconnect Retry Number. */
#if DEFAULT_WIFI_STA_MAXIMUM_NUMBER
static uint8_t wifi_sta_reconnect_counter = 0; 
#endif

/**
 * @brief  Wi-Fi Station Mode Reconnect Service Callback.
 * 
 * @param pxTimers[IN] Timer callback handle.
 */
static void wifi_sta_timer_callback(TimerHandle_t pxTimers)
{
    /* Set Wi-Fi EventGroups Reconnect Flag. */
    xEventGroupSetBits(wifi_event_group_handle, WIFI_USER_STA_RECONNECT_FL);
}
/**
 * @brief  Wi-Fi SmartConfig Service Time-out Callback.
 * 
 * @param pxTimers[IN] Timer callback handle.
 */
static void wifi_sc_timer_callback(TimerHandle_t pxTimers)
{
    /* Set Wi-Fi EventGroups SmartConfig Time-out Flag. */
    xEventGroupSetBits(wifi_event_group_handle, WIFI_USER_SC_TIMEOUT_FL);
}
/**
 * @brief  Wi-Fi Station Mode Reconnection Task Function.
 * 
 * @param prvParameters[IN] Task Create Accept Parameters. 
 */
static void wifi_sta_task(void * pvParameters)
{
    while(1)
    {
        /* Block to wait for one or more bits to be set within a previously created event group. */
        xEventGroupWaitBits(wifi_event_group_handle,                               /* The Event Group Handle. */
                            WIFI_USER_STA_RECONNECT_EN | WIFI_USER_STA_RECONNECT_FL, /* The bits within the event group to wait for. */
                            pdTRUE,                                                  /* The bits will cleared before returning when they expire. */
                            pdTRUE,                                                  /* Will wait both bits when they expire. */
                            portMAX_DELAY);                                          /* Wait maximum time for either bit to be set. */
    
        /* Reconnection Wi-Fi. */
        esp_err_t ret = esp_wifi_connect();
        if(ret != ESP_OK)
        {
            /* Information. */
            ESP_LOGE(TAG, "Wi-Fi connect error. error code(%s).", esp_err_to_name(ret));
        }
    }
}
/**
 * @brief  Wi-Fi SmartConfig Task Function.
 * 
 * @param prvParameters[IN] Task Create Accept Parameters. 
 */
static void wifi_sc_task(void *prvParameters)
{
    EventBits_t uxBits = 0;
    BaseType_t uxRets = 0;

    /* Initialize SmartConfig. */
    WIFI_ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();

    /* Start SmartConfig Service. */
    WIFI_ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

    while (1)
    {
        uxBits = xEventGroupWaitBits(wifi_event_group_handle,
                                     WIFI_USER_SC_FINISH_FL | WIFI_USER_SC_TIMEOUT_FL | WIFI_USER_STA_CONNECTION_FL,
                                     pdTRUE,
                                     pdFALSE,
                                     portMAX_DELAY);
        
        if (uxBits & WIFI_USER_SC_TIMEOUT_FL)
        {
            ESP_LOGI(TAG, "Smartconfig timeout. Stop smartconfig service.");
        }

        if (uxBits & WIFI_USER_STA_CONNECTION_FL)
        {
            ESP_LOGI(TAG, "Wi-Fi connected. Stop smartconfig service");
        }

        if (uxBits & WIFI_USER_SC_FINISH_FL)
        {
            ESP_LOGI(TAG, "Smartconfig has been successful. Stop smartconfig service.");
        }

        /* Stop Wi-Fi smartconfig service. */
        esp_err_t ret = esp_smartconfig_stop();
        if(ret != ESP_OK)
        {
            /* Information. */
            ESP_LOGE(TAG, "Smartconfig stop error, code:(%s).", esp_err_to_name(ret));
        }

        /* Delete Wi-Fi smartconfig service. */
        xTimerStop(wifi_sc_timer_handle, pdMS_TO_TICKS(1000));
        uxRets = xTimerDelete(wifi_sc_timer_handle, pdMS_TO_TICKS(1000));
        if(uxRets != pdPASS)
        {
            ESP_LOGE(TAG, "Smartconfig timeout timer delete failed.");
        }
        else
        {
            wifi_sc_timer_handle = NULL;
        }
        wifi_sc_task_handle = NULL;        
        vTaskDelete(NULL);
        ESP_LOGI(TAG, "Delete Wi-Fi Smartconfig service.");
    }
}
/**
 * @brief Initialize ESP32 Wi-Fi Station Mode.
 *
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_create_wifi_reconnect_service(void)
{
    /* Set Wi-Fi station mode event group reconnection bit. */
    xEventGroupSetBits(wifi_event_group_handle, WIFI_USER_STA_RECONNECT_EN);

    /* Create Wi-Fi station mode reconnection Service. */
    if (wifi_sta_task_handle == NULL)
    {
        /* Create Wi-Fi station mode reconnection timer. */
        wifi_sta_timer_handle = xTimerCreate("Wi-Fi Station Mode Retry Timer",           /* Just a text name, not used by the kernel. */
                                             (DEFAULT_WIFI_STA_RETRY_SHORT_TIME * 1000), /* The timer period in ticks. */
                                             pdFALSE,                                    /* The timers will auto-reload themselves when they expire. */
                                             &wifi_sta_timer_id,                         /* Assign each timer a unique id equal to its array index. */
                                             wifi_sta_timer_callback);                   /* Each timer calls the same callback when it expires. */
        if (wifi_sta_timer_handle == NULL)
        {
            ESP_LOGE(TAG, "Wi-Fi Station Mode Reconnect Retry Timer Create Failure.");
            return ESP_FAIL;
        }

        /* Create Wi-Fi station mode reconnection task. */
        BaseType_t uxBits = xTaskCreate(wifi_sta_task,                 /* Pointer to the task entry function. */
                                        "Wi-Fi STA reconnection task", /*  Descriptive name for the task. */
                                        WIFI_STA_TASK_STACK_DEPTH,     /* The size of the task stack specified as the number of bytes. */
                                        NULL,                          /* Pointer that will be used as the parameter for the task being created. */
                                        WIFI_STA_TASK_PRIORITY,        /* The priority at which the task should run.  */
                                        &wifi_sta_task_handle);        /* Used to pass back a handle by which the created task can be referenced. */
        if (uxBits != pdPASS)
        {
            ESP_LOGE(TAG, "Wi-Fi Station Mode Task Create Failure.");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Create Wi-Fi reconnection service.");
    return ESP_OK;
}
/**
 * @brief Initialize ESP32 Wi-Fi Station Mode.
 * 
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_delete_wifi_reconnect_service(void)
{
    return ESP_OK;
}
/**
 * @brief Initialize ESP32 Wi-Fi Station Mode.
 * 
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_create_wifi_smartconfig_service(void)
{
    /* Create Wi-Fi Smartconfig Service. */
    if (wifi_sc_task_handle == NULL)
    {
        /* Create Wi-Fi smartconfig service task. */
        BaseType_t uxBits = xTaskCreate(wifi_sc_task,             /* Pointer to the task entry function. */
                                        "Wi-Fi Smartconfig task", /*  Descriptive name for the task. */
                                        WIFI_SC_TASK_STACK_DEPTH, /* The size of the task stack specified as the number of bytes. */
                                        NULL,                     /* Pointer that will be used as the parameter for the task being created. */
                                        WIFI_SC_TASK_PRIORITY,    /* The priority at which the task should run.  */
                                        &wifi_sc_task_handle);    /* Used to pass back a handle by which the created task can be referenced. */
        if (uxBits != pdPASS)
        {
            ESP_LOGE(TAG, "Smartconfig task create failure.");
            return ESP_FAIL;
        }
        /* Create Wi-Fi smartconfig time-out timer. */
        wifi_sc_timer_handle = xTimerCreate("Smartconfig timeout timer",                /* Just a text name, not used by the kernel. */
                                            pdMS_TO_TICKS(WIFI_SC_MAXIMUM_TIME * 1000), /* The timer period in ticks. */
                                            pdFALSE,                                    /* The timers will auto-reload themselves when they expire. */
                                            &wifi_sc_timer_id,                          /* Assign each timer a unique id equal to its array index. */
                                            wifi_sc_timer_callback);                    /* Each timer calls the same callback when it expires. */
        if (wifi_sc_timer_handle == NULL)
        {
            ESP_LOGE(TAG, "Smartconfig time-out timer create failure.");
            return ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Create Wi-Fi Smartconfig service.");
    return ESP_OK;
}
/**
 * @brief Initialize ESP32 Wi-Fi Station Mode.
 * 
 * @return  - ESP_OK    succeed.
 *          - ESP_FAIL  failed.
 */
static esp_err_t user_delete_wifi_smartconfig_service(void)
{
    return ESP_OK;
}
/**
 * @brief  Wi-Fi Station Mode Event Group CallBack.
 * 
 * @param args[IN] user data registered to the event.
 * 
 * @param event_base[IN] Event base for the handler.
 * 
 * @param event_id[IN] The id for the received event.
 * 
 * @param event_data[IN] The data for the event.
 */
static void wifi_sta_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        if(event_id == WIFI_EVENT_STA_START)
        {
            /* Connect the Wi-Fi Station to the AP.  */
            esp_err_t ret = esp_wifi_connect();
            if (ret != ESP_OK)
            {
                ESP_LOGE(TAG, "ESP_WIFI_CONNECT ERROR CODE: (%s).", esp_err_to_name(ret));
            }
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            wifi_event_sta_disconnected_t *disconnected = (wifi_event_sta_disconnected_t *)event_data;

            ESP_LOGI(TAG, "The Wi-Fi station mode is disconnected. Reason:%d.", disconnected->reason);

            // /* disconnected reason 8-> OTA upgrade successful esp32 restart. */
            // if (disconnected->reason != 8)
            // {
            //     /* Delete HTTP(S) OTA Service. */
            //     user_delete_ota_service();
            // }

            /* Clear Wi-Fi station mode Event Group Connection flag bit. */
            xEventGroupClearBits(wifi_event_group_handle, WIFI_USER_STA_CONNECTION_FL);

            /* Create Wi-Fi station mode reconnection service. */
            user_create_wifi_reconnect_service();

            /* Create Wi-Fi smartconfig service. */
            user_create_wifi_smartconfig_service();

#if DEFAULT_WIFI_STA_MAXIMUM_NUMBER
            /* If the current number of reconnections is less than the maximum number of reconnections, 
               the reconnection will be performed after a short delay. */
            if(wifi_sta_reconnect_counter < DEFAULT_WIFI_STA_MAXIMUM_NUMBER)
            {
                /* Accumulate Wi-Fi station mode reconnection counter. */
                wifi_sta_reconnect_counter += 1;
            
                /* Set Wi-Fi station mode reconnection interval. */
                xTimerChangePeriod(wifi_sta_timer_handle, pdMS_TO_TICKS(DEFAULT_WIFI_STA_RETRY_SHORT_TIME * 1000));

                /* Start Wi-Fi station mode reconnection timer. */
                xTimerStart(wifi_sta_timer_handle, pdMS_TO_TICKS(1000));
            }
            else
            {
                /* Clear Wi-Fi station mode reconnection counter. */
                wifi_sta_reconnect_counter = 0;
            
                /* Set Wi-Fi station reconnection interval. */
                xTimerChangePeriod(wifi_sta_timer_handle, pdMS_TO_TICKS(DEFAULT_WIFI_STA_RETRY_LONG_TIME * 1000));

                /* Start Wi-Fi station reconnection timer. */
                xTimerStart(wifi_sta_timer_handle, pdMS_TO_TICKS(1000));            
            }
#else
            /* Start Wi-Fi station mode reconnection timer. */
            xTimerStart(wifi_sta_timer_handle, pdMS_TO_TICKS(1000));
#endif /* DEFAULT_WIFI_STA_MAXIMUM_NUMBER */
        }
    }
    else if(event_base == SC_EVENT)
    {
        switch(event_id)
        {
            case SC_EVENT_SCAN_DONE:
            {
                ESP_LOGI(TAG, "Smartconfig scan done.");
                break;
            }
            case SC_EVENT_FOUND_CHANNEL:
            {
                ESP_LOGI(TAG, "Smartconfig found channel.");

                /* CLear Wi-Fi smartconfig service timeout. */
                xEventGroupClearBits(wifi_event_group_handle, WIFI_USER_SC_TIMEOUT_FL);

                /* Start Wi-Fi smartconfig time-out timer. */
                xTimerStart(wifi_sc_timer_handle, pdMS_TO_TICKS(1000));
                break;
            }
            case SC_EVENT_GOT_SSID_PSWD:
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
                if(wifi_sta_config.sta.bssid_set == true)
                {
                    memcpy(wifi_sta_config.sta.bssid, evt->bssid, sizeof(wifi_sta_config.sta.bssid));
                }

                /* if smartconfig is ESPTOUCH_V2 mode, get rvd_data. */
                if(evt->type == SC_TYPE_ESPTOUCH_V2)
                {
                    uint8_t rvd_data[33] = {0};
                    WIFI_ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
                    ESP_LOGI(TAG, "rvd_data:");
                    for (int i = 0; i < 33; i++)
                    {
                        printf("%02x ", rvd_data[i]);
                    }
                    printf("\n");
                }

                /* if Wi-Fi station mode is connected, disconnect the Wi-Fi station mode connection. */
                BaseType_t uxBits = xEventGroupGetBits(wifi_event_group_handle);
                if((uxBits & WIFI_USER_STA_CONNECTION_FL) == 1)
                {
                    WIFI_ESP_ERROR_CHECK(esp_wifi_disconnect());
                }

                /* Configuration Wi-Fi station mode parameters. */
                WIFI_ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

                /* Start Wi-Fi station mode connection. */
                // esp_wifi_connect();
                break;
            }
            case SC_EVENT_SEND_ACK_DONE:
            {
                ESP_LOGI(TAG, "Smartconfig finish.");
                /* Set Wi-Fi event group smartconfig finish flag bit. */
                xEventGroupSetBits(wifi_event_group_handle, WIFI_USER_SC_FINISH_FL);
                break;
            }
            default:
            {
                /* Information. */
                ESP_LOGI(TAG, "Smartconfig other event id:(%d)", event_id);
                break;
            }
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            /* Set Wi-Fi event group station mode connection flag bit. */
            xEventGroupSetBits(wifi_event_group_handle, WIFI_USER_STA_CONNECTION_FL);

            /* Clear Wi-Fi station mode event group reconnection bit. */
            xEventGroupClearBits(wifi_event_group_handle, WIFI_USER_STA_RECONNECT_EN);

            /* Get Wi-Fi station mode ip. */
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Get IP:" IPSTR, IP2STR(&event->ip_info.ip));
        
            /* Delete Wi-Fi reconnect service.  */
            user_delete_wifi_reconnect_service();

            /* Delete Wi-Fi station mode reconnection service. */
            if(wifi_sta_task_handle != NULL)
            {
                BaseType_t uxRets = xTimerDelete(wifi_sta_timer_handle, pdMS_TO_TICKS(1000));
                if(uxRets != pdPASS)
                {
                    ESP_LOGE(TAG, "Delete Wi-Fi station mode reconnection timer failure.");
                }
                vTaskDelete(wifi_sta_task_handle);
                wifi_sta_task_handle = NULL;
                ESP_LOGI(TAG, "Delete Wi-Fi reconnection service.");
            }

            /* Create MQTT client. */
            user_create_mqtt_client();
            /* Create HTTPS OTA Service. */
            user_create_ota_service();
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
    WIFI_ESP_ERROR_CHECK(esp_netif_init());

    /* Create default event loop.  */
    WIFI_ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Creates Default WIFI Station Mode. */
    esp_netif_create_default_wifi_sta();

    /* Initialize Wi-Fi .*/
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    WIFI_ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Register All Wi-Fi Event ID to the system event loop. */
    WIFI_ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));
    /* Register Wi-Fi Station Mode Got IP Event ID to the system event loop. */
    WIFI_ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_sta_event_handler, NULL));
    /* Register Wi-Fi All SmartConfig Event ID to the system event loop. */
    WIFI_ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &wifi_sta_event_handler, NULL));

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
        memcpy(wifi_sta_config.sta.ssid, DEFAULT_WIFI_STA_SSID, sizeof(DEFAULT_WIFI_STA_SSID));
        memcpy(wifi_sta_config.sta.password, DEFAULT_WIFI_STA_PWSD, sizeof(DEFAULT_WIFI_STA_PWSD));
        WIFI_ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        WIFI_ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    }
    else
    {
        /* if the Wi-Fi account saved in the nvs flash is empty, set the Wi-Fi configuration to the default value. */
        if (strlen((const char *)wifi_sta_config.sta.ssid) == 0)
        {
            ESP_LOGI(TAG, "Wi-Fi Station Mode configuration is empty.");
            ESP_LOGI(TAG, "Set Wi-Fi Station Mode default parameter.");

            /* Configuration Default Wi-Fi Station Mode Parameter. */
            memcpy(wifi_sta_config.sta.ssid, DEFAULT_WIFI_STA_SSID, sizeof(DEFAULT_WIFI_STA_SSID));
            memcpy(wifi_sta_config.sta.password, DEFAULT_WIFI_STA_PWSD, sizeof(DEFAULT_WIFI_STA_PWSD));
            WIFI_ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            WIFI_ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
        }
    }

    ESP_LOGI(TAG, "Connection SSID: %s.", wifi_sta_config.sta.ssid);
    ESP_LOGI(TAG, "Connection PWSD: %s.", wifi_sta_config.sta.password);

    /* Start WiFi according to Current Configuration. */
    WIFI_ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}
/******************************** End of File *********************************/

