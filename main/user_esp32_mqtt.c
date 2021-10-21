/**
 *****************************************************************************
 * @file    : user_esp32_mqtt.c
 * @brief   : ESP32 MQTT Application
 * @author  : Cao Jin
 * @date    : 19-Oct-2021
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

#define WIFI_MQTT_MSG_PROC_TASK_STACK_DEPTH   (4 * 1024)  /* Wi-Fi Station Mode Task Default StackDepth. */
#define WIFI_MQTT_MSG_PROC_TASK_PRIORITY      (2)         /* Wi-Fi Station Mode Task Default Priority. */

/** @brief Complete MQTT broker URI  */
#define DEFAULT_MQTT_BROKER_URL "mqtt://47.102.193.111:1883" //"mqtt://106.14.31.82:2005"

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

#define MQTT_QOS_LEVEL  (1U)

#define MAXIMUM_MQTT_TOPIC_LENGTH   (30U)
#define MAXIMUM_MQTT_DATA_LENGTH    (30U)
#define MAXIMUM_MQTT_MSG_LENGTH     (10U)

/** @brief MQTT publish or subscribe msg_id error check. */
static void esp_mqtt_msg_id_check_printf(char *file, int line, char *fuc);
#define ESP_MQTT_MSG_ID_CHECK(x)                                  \
    do                                                            \
    {                                                             \
        if ((x) == -1)                                            \
        {                                                         \
            esp_mqtt_msg_id_check_printf(__FILE__, __LINE__, #x); \
        }                                                         \
    } while (0)

/** @brief MQTT message structure. */
typedef struct
{
    char topic[MAXIMUM_MQTT_TOPIC_LENGTH];
    char data[MAXIMUM_MQTT_DATA_LENGTH];
}esp_mqtt_message_t;

/** @brief log output label. */
static const char *TAG = "MQTT Application";

/** @brief MQTT message queue handle. */
static QueueHandle_t mqtt_msg_queue_handle = NULL;
/** @brief MQTT message processing task handle. */
static TaskHandle_t mqtt_msg_proc_task_handle = NULL;

/** @brief MQTT client handle. */
static esp_mqtt_client_handle_t mqtt_client = NULL;

/** @brief MQTT client initialize flag. */
// static uint8_t mqtt_client_init_flag = false;

extern const uint8_t mqtt_server_cert_pem_start[] asm("_binary_mqtt_ca_cert_pem_start");
extern const uint8_t mqtt_server_cert_pem_end[] asm("_binary__mqtt_ca_cert_pem_end");

static void mqtt_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void mqtt_msg_proc_task(void * pvParameters);

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
            /* Information. */
            ESP_LOGE(TAG, "MQTT client initialize failure.");
            return ESP_FAIL;
        }

        /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
        ret = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        if (ret != ESP_OK)
        {
            /* Information. */
            ESP_LOGE(TAG, "MQTT client register event failure. ERROR CODE:(%s).", esp_err_to_name(ret));
            return ret;
        }
        /* Starts MQTT client with already created client handle.  */
        ret = esp_mqtt_client_start(mqtt_client);
        if (ret != ESP_OK)
        {
            /* Information. */
            ESP_LOGE(TAG, "MQTT client start failure. ERROR CODE:(%s).", esp_err_to_name(ret));
            return ret;
        }

        /* Create MQTT message queue. */
        mqtt_msg_queue_handle = xQueueCreate(MAXIMUM_MQTT_MSG_LENGTH, sizeof(esp_mqtt_message_t));
        if(mqtt_msg_queue_handle == NULL)
        {
            /* Information. */
            ESP_LOGE(TAG, "Message queue creation failed.");
            return ESP_FAIL;
        }

        /* Create MQTT message processing task. */
        BaseType_t uxBits = xTaskCreate(mqtt_msg_proc_task,                  /* Pointer to the task entry function. */
                                        "MQTT message processing task",      /*  Descriptive name for the task. */
                                        WIFI_MQTT_MSG_PROC_TASK_STACK_DEPTH, /* The size of the task stack specified as the number of bytes. */
                                        NULL,                                /* Pointer that will be used as the parameter for the task being created. */
                                        WIFI_MQTT_MSG_PROC_TASK_PRIORITY,    /* The priority at which the task should run.  */
                                        &mqtt_msg_proc_task_handle);         /* Used to pass back a handle by which the created task can be referenced. */
        if (uxBits != pdPASS)
        {
            ESP_LOGE(TAG, "Message processing task creation failed.");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}
/**
 * @brief  MQTT message processing task.
 * @param prvParameters Task create accept parameters. 
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
            /* Information. */
            ESP_LOGI(TAG, "Message processing.");

            if (strcmp(mqtt_msg.topic, SUB_SWITCH_VALVE_STATE1) == 0)
            {
                if (strcmp(mqtt_msg.data, "on") == 0)
                {
                    /* Information. */
                    ESP_LOGI(TAG, "First switch valve on.");
                }
                else if (strcmp(mqtt_msg.data, "off") == 0)
                {
                    /* Information. */
                    ESP_LOGI(TAG, "First switch valve off.");
                }
            }
            else if (strcmp(mqtt_msg.topic, SUB_SWITCH_VALVE_STATE2) == 0)
            {
                if (strcmp(mqtt_msg.data, "on") == 0)
                {
                    /* Information. */
                    ESP_LOGI(TAG, "Second switch valve on.");
                }
                else if (strcmp(mqtt_msg.data, "off") == 0)
                {
                    /* Information. */
                    ESP_LOGI(TAG, "Second switch valve off.");
                }
            }
            else if (strcmp(mqtt_msg.topic, SUB_SWITCH_VALVE_STATE3) == 0)
            {
                if (strcmp(mqtt_msg.data, "on") == 0)
                {
                    /* Information. */
                    ESP_LOGI(TAG, "Third switch valve on.");
                }
                else if (strcmp(mqtt_msg.data, "off") == 0)
                {
                    /* Information. */
                    ESP_LOGI(TAG, "Third switch valve off.");
                }
            }
            else if (strcmp(mqtt_msg.topic, SUB_PUMP_STATE1) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_RGB_STATE1) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_RGB_STATE2) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_RGB_LIGHT1) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_RGB_LIGHT2) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_RGB_COLOR1) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_RGB_COLOR2) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_FAN_STATE1) == 0)
            {
            }
            else if (strcmp(mqtt_msg.topic, SUB_FAN_SPEED1) == 0)
            {
            }
        }
    }
}
/**
 * @brief  Subscribe default MQTT topic and publish default MQTT topic value.
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

    /* Publish default values to MQTT topics */
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SWITCH_VALVE_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SWITCH_VALVE_STATE2, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SWITCH_VALVE_STATE3, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_PUMP_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_STATE2, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_LIGHT1, "255", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_LIGHT2, "255", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_COLOR1, "255,255,255", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_RGB_COLOR2, "255,255,255", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_FAN_STATE1, "off", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_FAN_SPEED1, "100", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SOIL_HUMI1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SOIL_HUMI2, "0.0", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_SOIL_HUMI3, "0.0", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_ENVM_HUMI1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_ENVM_TEMP1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_ENVM_TMOS1, "0.0", 0, MQTT_QOS_LEVEL, 0));
    ESP_MQTT_MSG_ID_CHECK(esp_mqtt_client_publish(client, PUB_TDS_VALUE1, "0.0", 0, MQTT_QOS_LEVEL, 0));
}
/**
 * @brief  Wi-Fi Station Mode Event Group CallBack.
 * @param args user data registered to the event.
 * @param event_base Event base for the handler.
 * @param event_id The id for the received event.
 * @param event_data The data for the event
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
        /* Information. */
        ESP_LOGI(TAG, "Connected to server.");

        /* If MQTT connects to the server, subscribe and publish related topics. */
        user_mqtt_topic_init(client);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
    {
        /* Information. */
        ESP_LOGI(TAG, "Disconnected from server.");
        break;
    }
    case MQTT_EVENT_SUBSCRIBED:
    {
        /* Information. */
        ESP_LOGI(TAG, "Subscribed topic, msg_id=%d", event->msg_id);
        break;
    }
    case MQTT_EVENT_UNSUBSCRIBED:
    {
        /* Information. */
        ESP_LOGI(TAG, "Unsubscribed topic, msg_id=%d", event->msg_id);
        break;
    }
    case MQTT_EVENT_PUBLISHED:
    {
        /* Information. */
        ESP_LOGI(TAG, "Published message, msg_id=%d", event->msg_id);
        break;
    }
    case MQTT_EVENT_DATA:
    {
        /* Information */
        ESP_LOGI(TAG, "Topic message received.");

        /* Determine the length of the topic received. */
        if(event->topic_len < MAXIMUM_MQTT_TOPIC_LENGTH)
        {
            /* Complete information. */
            memcpy(msg.topic, event->topic, event->topic_len);
        }
        else
        {
            /* Information. */
            ESP_LOGE(TAG, "The length of the topic received is too long.");
            /* Truncation information. */
            memcpy(msg.topic, event->topic, MAXIMUM_MQTT_TOPIC_LENGTH);
        }

        /* Determine the length of topic received. */
        if (event->data_len < MAXIMUM_MQTT_DATA_LENGTH)
        {
            /* Complete information. */
            memcpy(msg.data, event->data, event->data_len);
        }
        else
        {
            /* Information. */
            ESP_LOGE(TAG, "The length of data received is too long.");
            /* Truncation information. */
            memcpy(msg.data, event->data, MAXIMUM_MQTT_DATA_LENGTH);
        }

        /* Send topic messages to the MQTT message queue */
        xQueueSend(mqtt_msg_queue_handle, &msg, portMAX_DELAY);
        break;
    }
    case MQTT_EVENT_ERROR:
    {
        /* Information. */
        ESP_LOGE(TAG, "MQTT event error.");
        break;
    }
    default:
    {
        /* Information. */
        ESP_LOGI(TAG, "MQTT other event id:%d", event->event_id);
        break;
    }
    }
}
/**
 * @brief  User MQTT message id error check callback
 * @param file
 * @param line
 * @param fuc
 */
static void esp_mqtt_msg_id_check_printf(char *file, int line, char *fuc)
{
    ESP_LOGE(TAG, "File(%s) Line(%d) Fuc(%s) Error.", file, line, fuc);
}