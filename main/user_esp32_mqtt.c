/**
 *****************************************************************************
 * @file    : user_esp32_mqtt.c
 * @brief   : ESP32 MQTT Application
 * @author  : Cao Jin
 * @date    : 27-Oct-2021
 * @version : 1.0.0
 *****************************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"

#include "mqtt_client.h"

#include "user_esp32_mqtt.h"
#include "user_esp32_ota.h"

/** @brief FreeRTOS MQTT message process task configuration. */
#define MQTT_MSG_PROC_TASK_STACK_DEPTH      (4 * 1024)
#define MQTT_MSG_PROC_TASK_PRIORITY         (3)

/** @brief MQTT Quality of Service 0, Means at most once.  */
#define MQTT_QOS_0                          (0U)

/** @brief MQTT Quality of Service 1, Means at least once.  */
#define MQTT_QOS_1                          (1U)

/** @brief MQTT Quality of Service 2, Means exactly once. */
#define MQTT_QOS_2                          (2U)

/** @brief Default MQTT used Quality of Service. */
#define MQTT_QOS_LEVEL                      MQTT_QOS_0


/** @brief MQTT message queue maximum topic length. (char) */
#define MAXIMUM_MQTT_TOPIC_LENGTH           (30U)

/** @brief MQTT message queue maximum data length. (char) */
#define MAXIMUM_MQTT_DATA_LENGTH            (30U)

/** @brief MQTT message queue maximum length. (esp_mqtt_message_t) */
#define MAXIMUM_MQTT_MSG_LENGTH             (10U)


/** @brief Complete MQTT broker URI  */
#define DEFAULT_MQTT_BROKER_URL             "mqtt://47.102.193.111:1883" 
                                            //"mqtt://106.14.31.82:2005"
                                            
/** @brief MQTT subscribe the topic groups. */
#define SUB_SWITCH_VALVE_STATE1 "firstSwitchCommand"  /* Switch valve1 -> Switch command topic. */
#define SUB_SWITCH_VALVE_STATE2 "secondSwitchCommand" /* Switch valve2 -> Switch command topic. */
#define SUB_SWITCH_VALVE_STATE3 "thirdSwitchCommand"  /* Switch valve3 -> Switch command topic. */
#define SUB_PUMP_STATE1 "pumpCommand"                 /* Water pump1 -> Switch command topic. */
#define SUB_RGB_STATE1 "firstLightCommand"            /* WS2812 RGB1 -> Switch command topic. */
#define SUB_RGB_STATE2 "secondLightCommand"           /* WS2812 RGB2 -> Switch command topic. */
#define SUB_RGB_LIGHT1 "firstBrightnessCommand"       /* WS2812 RGB1 -> Brightness command topic. */
#define SUB_RGB_LIGHT2 "secondBrightnessCommand"      /* WS2812 RGB2 -> Brightness command topic. */
#define SUB_RGB_COLOR1 "firstRgbCommand"              /* WS2812 RGB1 -> Color command topic. */
#define SUB_RGB_COLOR2 "secondRgbCommand"             /* WS2812 RGB2 -> Color command topic. */
#define SUB_FAN_STATE1 "fanCommand"                   /* Fan1 -> Switch command topic. */
#define SUB_FAN_SPEED1 "fanSpeedCommand"              /* Fan1 -> Speed command topic. */
#define SUB_OTA_SERVICE "OTAServiceCommand"            

/** @brief MQTT publish the topic groups. */
#define PUB_SWITCH_VALVE_STATE1 "firstSwitchState"  /* Switch valve1 -> Switch status topic. */
#define PUB_SWITCH_VALVE_STATE2 "secondSwitchState" /* Switch valve2 -> Switch status topic. */
#define PUB_SWITCH_VALVE_STATE3 "thirdSwitchState"  /* Switch valve3 -> Switch status topic. */
#define PUB_PUMP_STATE1 "pumpState"                 /* Water pump1 -> Switch status topic. */
#define PUB_RGB_STATE1 "firstLightState"            /* WS2812 RGB1 -> Switch status topic. */
#define PUB_RGB_STATE2 "secondLightState"           /* WS2812 RGB2 -> Switch status topic. */
#define PUB_RGB_LIGHT1 "firstBrightnessState"       /* WS2812 RGB1 -> Brightness status topic. */
#define PUB_RGB_LIGHT2 "secondBrightnessState"      /* WS2812 RGB2 -> Brightness status topic. */
#define PUB_RGB_COLOR1 "firstRgbState"              /* WS2812 RGB1 -> Color status topic. */
#define PUB_RGB_COLOR2 "secondRgbState"             /* WS2812 RGB2 -> Color status topic. */
#define PUB_FAN_STATE1 "fanState"                   /* Fan1 -> Switch status topic. */
#define PUB_FAN_SPEED1 "fanSpeedState"              /* Fan1 -> Speed status topic. */
#define PUB_SOIL_HUMI1 "firstSoilMoisture"          /* Soil moisture sensor1 -> Humidity information topic. */
#define PUB_SOIL_HUMI2 "secondSoilMoisture"         /* Soil moisture sensor2 -> Humidity information topic. */
#define PUB_SOIL_HUMI3 "thirdSoilMoisture"          /* Soil moisture sensor3 -> Humidity information topic. */
#define PUB_ENVM_HUMI1 "environmentMoisture"        /* Environmental temperature and humidity sensor1 -> Humidity information topic. */
#define PUB_ENVM_TEMP1 "environmentTemp"            /* Environmental temperature and humidity sensor1 -> Temperature information topic. */
#define PUB_ENVM_TMOS1 "atmos"                      /* Atmospheric pressure sensor -> Atmospheric pressure information topic. */
#define PUB_TDS_VALUE1 "tds"                        /* Water quality sensor -> Water quality information topic. */

/** @brief MQTT publish or subscribe msg_id error check. */
#define ESP_MQTT_MSG_ID_CHECK(x)                                                \
    do                                                                          \
    {                                                                           \
        if ((x) == -1)                                                          \
        {                                                                       \
            ESP_LOGE("MQTT MSG ID ERROR", "%s-%d-%d.", __FILE__, __LINE__, #x); \
        }                                                                       \
    } while (0)

/** @brief MQTT message structure. */
typedef struct
{
    char topic[MAXIMUM_MQTT_TOPIC_LENGTH];
    int topic_len;
    char data[MAXIMUM_MQTT_DATA_LENGTH];
    int data_len;
}esp_mqtt_message_t;

/** @brief log output label. */
static const char *TAG = "MQTT Application";

/** @brief FreeRTOS MQTT handles.  */
static QueueHandle_t mqtt_msg_queue_handle = NULL;
static TaskHandle_t mqtt_msg_proc_task_handle = NULL;

/** @brief MQTT client handle. */
static esp_mqtt_client_handle_t mqtt_client = NULL;

/** @brief MQTT SSL Certificate. */
extern const uint8_t mqtt_server_cert_pem_start[] asm("_binary_mqtt_ca_cert_pem_start");
extern const uint8_t mqtt_server_cert_pem_end[] asm("_binary__mqtt_ca_cert_pem_end");

/**
 * @brief  MQTT message processing task.
 * 
 * @param prvParameters[IN] Task create accept parameters. 
 */
static void mqtt_msg_proc_task(void * pvParameters)
{
    BaseType_t uxBits;
    esp_mqtt_message_t mqtt_msg;

    while(1)
    {
        uxBits = xQueueReceive(mqtt_msg_queue_handle, &mqtt_msg, portMAX_DELAY);
        if (uxBits == pdPASS)
        {
            ESP_LOGI(TAG, "Message processing.");

            if (memcmp(mqtt_msg.topic, SUB_SWITCH_VALVE_STATE1, mqtt_msg.topic_len) == 0)
            {
                if (memcmp(mqtt_msg.data, "on", mqtt_msg.data_len) == 0)
                {
                    ESP_LOGI(TAG, "First switch valve on.");
                }
                else if (memcmp(mqtt_msg.data, "off", mqtt_msg.data_len) == 0)
                {
                    ESP_LOGI(TAG, "First switch valve off.");
                }
                else
                {
                    ESP_LOGE(TAG, "UNKNOW DATA.");
                }
            }
            else if (memcmp(mqtt_msg.topic, SUB_SWITCH_VALVE_STATE2, mqtt_msg.topic_len) == 0)
            {
                if (memcmp(mqtt_msg.data, "on", mqtt_msg.data_len) == 0)
                {
                    ESP_LOGI(TAG, "Second switch valve on.");
                }
                else if (memcmp(mqtt_msg.data, "off", mqtt_msg.data_len) == 0)
                {
                    ESP_LOGI(TAG, "Second switch valve off.");
                }
            }
            else if (memcmp(mqtt_msg.topic, SUB_SWITCH_VALVE_STATE3, mqtt_msg.topic_len) == 0)
            {
                if (memcmp(mqtt_msg.data, "on", mqtt_msg.data_len) == 0)
                {
                    ESP_LOGI(TAG, "Third switch valve on.");
                }
                else if (memcmp(mqtt_msg.data, "off", mqtt_msg.data_len) == 0)
                {
                    ESP_LOGI(TAG, "Third switch valve off.");
                }
            }
            else if (memcmp(mqtt_msg.topic, SUB_PUMP_STATE1, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_RGB_STATE1, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_RGB_STATE2, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_RGB_LIGHT1, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_RGB_LIGHT2, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_RGB_COLOR1, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_RGB_COLOR2, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_FAN_STATE1, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_FAN_SPEED1, mqtt_msg.topic_len) == 0)
            {
            }
            else if (memcmp(mqtt_msg.topic, SUB_OTA_SERVICE, mqtt_msg.topic_len) == 0)
            {
                if (memcmp(mqtt_msg.data, "start", mqtt_msg.data_len) == 0)
                {
                    /* Start HTTPS OTA Service. */
                    user_start_ota_service();
                }
            }
            // else
            // {
            //     ESP_LOGE(TAG, "UNKNOW MQTT TOPIC.");
            //     ESP_LOGE(TAG, "TOPIC: %s.", mqtt_msg.topic);
            //     ESP_LOGE(TAG, "DATA: %s.", mqtt_msg.data);
            // }
        }
        else
        {
            ESP_LOGE(TAG, "MQTT message queue receive failed.");
        }
    }
}
/**
 * @brief  Subscribe default MQTT topic and publish default MQTT topic value.
 * 
 * @param client[IN] MQTT Client handle.
 */
static void user_mqtt_topic_init(esp_mqtt_client_handle_t client)
{
    /* Subscribe to MQTT topics */
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_SWITCH_VALVE_STATE1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_SWITCH_VALVE_STATE1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_SWITCH_VALVE_STATE1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_PUMP_STATE1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_RGB_STATE1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_RGB_STATE2, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_RGB_LIGHT1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_RGB_LIGHT2, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_RGB_COLOR1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_RGB_COLOR2, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_FAN_STATE1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_FAN_SPEED1, MQTT_QOS_LEVEL));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_subscribe(client, SUB_OTA_SERVICE, MQTT_QOS_LEVEL));

    // /* Publish default values to MQTT topics */
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SWITCH_VALVE_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SWITCH_VALVE_STATE2, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SWITCH_VALVE_STATE3, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_PUMP_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_STATE2, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_LIGHT1, "255", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_LIGHT2, "255", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_COLOR1, "255,255,255", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_COLOR2, "255,255,255", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_FAN_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_FAN_SPEED1, "100", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SOIL_HUMI1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SOIL_HUMI2, "0.0", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SOIL_HUMI3, "0.0", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_ENVM_HUMI1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_ENVM_TEMP1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_ENVM_TMOS1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    // ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_TDS_VALUE1, "0.0", 0, MQTT_QOS_LEVEL, 0));
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
 * @param event_data[IN] The data for the event
 */
static void mqtt_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    esp_mqtt_message_t msg;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        ESP_LOGI(TAG, "Connected to server.");

        /* Subscribe to related topics. */
        user_mqtt_topic_init(client);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
    {
        ESP_LOGI(TAG, "Disconnected from server.");
        break;
    }
    case MQTT_EVENT_SUBSCRIBED:
    {
        ESP_LOGI(TAG, "Subscribed topic ""%.*s"".", event->topic_len, event->topic);
        break;
    }
    case MQTT_EVENT_UNSUBSCRIBED:
    {
        ESP_LOGI(TAG, "Unsubscribed topic ""%.*s"".", event->topic_len, event->topic);
        break;
    }
    case MQTT_EVENT_PUBLISHED:
    {
        ESP_LOGI(TAG, "Published message, Topic=%.*s.", event->topic_len, event->topic);
        break;
    }
    case MQTT_EVENT_DATA:
    {
        ESP_LOGI(TAG, "Received message, Topic=%.*s.", event->topic_len, event->topic);

        /* Determine the length of the topic received. */
        if(event->topic_len < MAXIMUM_MQTT_TOPIC_LENGTH)
        {
            /* Complete information. */
            memcpy(msg.topic, event->topic, event->topic_len);
            msg.topic_len = event->topic_len;
        }
        else
        {
            ESP_LOGE(TAG, "The length of the topic received is too long.");
            /* Truncation information. */
            memcpy(msg.topic, event->topic, MAXIMUM_MQTT_TOPIC_LENGTH);
            msg.topic_len = MAXIMUM_MQTT_TOPIC_LENGTH;
        }

        /* Determine the length of topic received. */
        if (event->data_len < MAXIMUM_MQTT_DATA_LENGTH)
        {
            /* Complete information. */
            memcpy(msg.data, event->data, event->data_len);
            msg.data_len = event->data_len;
        }
        else
        {
            ESP_LOGE(TAG, "The length of data received is too long.");
            /* Truncation information. */
            memcpy(msg.data, event->data, MAXIMUM_MQTT_DATA_LENGTH);
            msg.data_len = MAXIMUM_MQTT_DATA_LENGTH;
        }

        /* Send topic messages to the MQTT message queue */
        xQueueSend(mqtt_msg_queue_handle, &msg, portMAX_DELAY);
        break;
    }
    case MQTT_EVENT_ERROR:
    {
        ESP_LOGE(TAG, "MQTT event error.");
        break;
    }
    default:
    {
        ESP_LOGI(TAG, "MQTT other event id:%d", event->event_id);
        break;
    }
    }
}
/**
 * @brief  Create MQTT client
 * 
 * @return - ESP_OK   succeed
 *         - ESP_FAIL failed
 */
esp_err_t user_create_mqtt_client(void)
{
    esp_err_t ret;

    /* Determine whether the MQTT service is created. */
    if (mqtt_client == NULL)
    {
        /* MQTT client configuration parameters.*/
        esp_mqtt_client_config_t mqtt_config = {
            .uri = DEFAULT_MQTT_BROKER_URL,
        };

        /* Creates MQTT client handle based on the configuration.  */
        mqtt_client = esp_mqtt_client_init(&mqtt_config);
        if (mqtt_client == NULL)
        {
            ESP_LOGE(TAG, "MQTT client initialize failure.");
            return ESP_FAIL;
        }

        /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
        ret = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "MQTT client register event failure. ERROR CODE:(%s).", esp_err_to_name(ret));
            return ret;
        }
        /* Starts MQTT client with already created client handle.  */
        ret = esp_mqtt_client_start(mqtt_client);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "MQTT client start failure. ERROR CODE:(%s).", esp_err_to_name(ret));
            return ret;
        }

        /* Create MQTT message queue. */
        mqtt_msg_queue_handle = xQueueCreate(MAXIMUM_MQTT_MSG_LENGTH, sizeof(esp_mqtt_message_t));
        if (mqtt_msg_queue_handle == NULL)
        {
            ESP_LOGE(TAG, "Message queue creation failed.");
            return ESP_FAIL;
        }

        /* Create MQTT message processing task. */
        BaseType_t uxBits = xTaskCreate(mqtt_msg_proc_task,             /* Pointer to the task entry function. */
                                        "MQTT message processing task", /*  Descriptive name for the task. */
                                        MQTT_MSG_PROC_TASK_STACK_DEPTH, /* The size of the task stack specified as the number of bytes. */
                                        NULL,                           /* Pointer that will be used as the parameter for the task being created. */
                                        MQTT_MSG_PROC_TASK_PRIORITY,    /* The priority at which the task should run.  */
                                        &mqtt_msg_proc_task_handle);    /* Used to pass back a handle by which the created task can be referenced. */
        if (uxBits != pdPASS)
        {
            ESP_LOGE(TAG, "Message processing task creation failed.");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}
/**
 * @brief  Delete MQTT client
 * 
 * @return - ESP_OK   succeed
 *         - ESP_FAIL failed
 */
esp_err_t user_delete_mqtt_client(void)
{
    if(mqtt_client != NULL)
    {

    }

    return ESP_OK;
}
/******************************** End of File *********************************/