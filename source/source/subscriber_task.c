/******************************************************************************
* File Name:   subscriber_task.c
*
* Description: This file contains the subscriber task that reads and parses the JSON messages received from 
*              sensor cloud.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2021, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"
#include "string.h"
#include "FreeRTOS.h"

/* Task header files */
#include "subscriber_task.h"
#include "mqtt_task.h"
#include "publisher_task.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"
#include "common_variables.h"
#include "radar_task.h"
/******************************************************************************
* Macros
******************************************************************************/
/* Maximum number of retries for MQTT subscribe operation */
#define MAX_SUBSCRIBE_RETRIES                   (3u)

/* Time interval in milliseconds between MQTT subscribe retries. */
#define MQTT_SUBSCRIBE_RETRY_INTERVAL_MS        (1000)

/* Queue length of a message queue that is used to communicate with the 
 * subscriber task.
 */
#define SUBSCRIBER_TASK_QUEUE_LENGTH            (1u)

/* Max number of subscriptions */
#define NUMBER_OF_SUBSCRIPTIONS (1u)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* Task handle for this task. */
TaskHandle_t subscriber_task_handle;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t subscriber_task_q;

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void subscribe_to_mqtt_topics(void);
subs_rslt_t construct_module_subscribe_mqtt_topic(char *topic);

/******************************************************************************
 * Function Name: subscriber_task
 ******************************************************************************
 * Summary:
 *  Task that subscribes to mqtt topic and parses the received messages
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void subscriber_task(void *pvParameters)
{
    subscriber_data_t subscriber_q_data;
    publisher_data_t publisher_q_data;
    /* To avoid compiler warnings */
    (void) pvParameters;
    cy_rslt_t result;

    /* Subscribe to the specified MQTT topic. */
    subscribe_to_mqtt_topics();

    /* Create a message queue to communicate with other tasks and callbacks. */
    subscriber_task_q = xQueueCreate(SUBSCRIBER_TASK_QUEUE_LENGTH, sizeof(subscriber_data_t));

    /* Register JSON parser to parse device properties JSON string */
    // cy_JSON_parser_register_callback(parse_device_properties, (void*) &radar_sensing_context);
    cy_JSON_parser_register_callback(parse_device_properties, (void*) NULL);

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(subscriber_task_q, &subscriber_q_data, portMAX_DELAY))
        {
            switch(subscriber_q_data.cmd)
            {
                case SUBSCRIBE_TO_TOPIC:
                {
                    /* Subscribe to device property topic to received updated from sensor cloud */
                    subscribe_to_mqtt_topics();
                    break;
                }

                case PARSE_INCOMING_PUBLISH:
                {
                    /* Parse the data received from sensor cloud */
                    APP_LOG_DEBUG(("PARSE_INCOMING_PUBLISH :- recieved json = %s, json_length = %d",subscriber_q_data.json_data, subscriber_q_data.json_data_length));
                    result = cy_JSON_parser(subscriber_q_data.json_data, subscriber_q_data.json_data_length);
                    if (result != CY_RSLT_SUCCESS)
                    {
                        APP_LOG_ERROR(("Device property Json parser error!"));
                    }
                    else {
                        APP_LOG_DEBUG(("Publishing the DP ack"));
                        /* Once parsing is done, send an acknowledgement to the cloud */
                        publisher_q_data.cmd = PUBLISH_DEVICE_PROPERTIES_UPDATE_ACK;
                        xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);
                    }
                    break;
                }
            }
        }
    }
}

/******************************************************************************
 * Function Name: subscribe_to_mqtt_topics
 ******************************************************************************
 * Summary:
 *  Function that subscribes to the MQTT topic specified by mqtt_topic_subscribe_device_properties
 *  In case of subscribtion failure, retries are done 'MAX_SUBSCRIBE_RETRIES' times with interval of 
 *  'MQTT_SUBSCRIBE_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void subscribe_to_mqtt_topics(void)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* MQTT Topics to subscribe to */
    cy_mqtt_subscribe_info_t SubscriptionTopic = { 0 };

    SubscriptionTopic.qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS;
    SubscriptionTopic.topic = (char *)mqtt_topic_subscribe_device_properties;
    SubscriptionTopic.topic_len = strlen((char *)mqtt_topic_subscribe_device_properties);

    /* Subscribe with the configured parameters. */
    for (uint32_t retry_count = 0; retry_count < MAX_SUBSCRIBE_RETRIES; retry_count++)
    {
        result = cy_mqtt_subscribe(mqtt_connection, &SubscriptionTopic, NUMBER_OF_SUBSCRIPTIONS);
        if (result == CY_RSLT_SUCCESS)
        {
            APP_LOG_INFO(("MQTT client subscribed to the topics successfully"));
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(MQTT_SUBSCRIBE_RETRY_INTERVAL_MS));
    }

    if (result != CY_RSLT_SUCCESS)
    {
        APP_LOG_ERROR(("MQTT Subscribe failed with error 0x%0X after %d retries...", 
               (int)result, MAX_SUBSCRIBE_RETRIES));
    }
}

/******************************************************************************
 * Function Name: mqtt_subscription_callback
 ******************************************************************************
 * Summary:
 *  Callback to handle incoming MQTT messages. This callback prints the 
 *  contents of the incoming message and informs the subscriber task, via a 
 *  message queue to parse the incoming message
 *
 * Parameters:
 *  cy_mqtt_publish_info_t *received_msg_info : Information structure of the 
 *                                              received MQTT message
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_subscription_callback(cy_mqtt_publish_info_t *received_msg_info)
{
    /* Received MQTT message */
    char *received_msg = (char*) received_msg_info->payload;
    int received_msg_len = received_msg_info->payload_len;

    /* Data to be sent to the subscriber task queue. */
    subscriber_data_t subscriber_q_data;

    APP_LOG_DEBUG(("Subsciber: Incoming MQTT message received:\n"
           "    Topic name: %.*s\n"
           "    QoS: %d\n"
           "    Payload: %.*s\n"
           "    Payload length = %d",
           received_msg_info->topic_len, received_msg_info->topic,
           (int) received_msg_info->qos,
           (int) received_msg_info->payload_len, (const char *)received_msg_info->payload, (int) received_msg_info->payload_len));

    /* Assign the command to be sent to the subscriber task. */
    subscriber_q_data.cmd = PARSE_INCOMING_PUBLISH;
    subscriber_q_data.json_data_length = received_msg_info->payload_len;
    subscriber_q_data.json_data = (char* ) malloc((received_msg_info->payload_len + 1));
    /* need to make sure that the incoming message is terminated by NULL Character to proceed with strcpy*/
    received_msg[received_msg_len + 1] = '\0';
    strcpy(subscriber_q_data.json_data, (char *)received_msg);
    /* terminate with a Null character to avoid any unforeseen errors */
    subscriber_q_data.json_data[received_msg_info->payload_len] = '\0';

    /* Send the command and data to subscriber task queue */
    xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);
}
/* [] END OF FILE */
