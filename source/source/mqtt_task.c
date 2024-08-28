/******************************************************************************
* File Name:   mqtt_task.c
*
* Description: This file contains the MQTT task that handles initialization & 
*              connection of Wi-Fi and the MQTT client. The task then starts 
*              the subscriber, the publisher and radar tasks. This task also implements
*              reconnection mechanisms to handle WiFi and MQTT disconnections.
*              The task also handles all the cleanup operations to gracefully 
*              terminate the Wi-Fi and MQTT connections in case of any failure.
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

/* FreeRTOS header files */
#include "FreeRTOS.h"
#include "task.h"

/* Task header files */
#include "mqtt_task.h"
#include "subscriber_task.h"
#include "publisher_task.h"

/* Configuration file for Wi-Fi and MQTT client */
#include "wifi_config.h"
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_retarget_io.h"
#include "cy_wcm.h"
#include "cy_lwip.h"
#include "cy_time.h"
#include "cy_mqtt_api.h"
#include "clock.h"

/* LwIP header files */
#include "lwip/netif.h"
#include "common_variables.h"
#include <inttypes.h>
#include <time.h>
#include "radar_task.h"
/******************************************************************************
* Macros
******************************************************************************/
/* Queue length of a message queue that is used to communicate the status of 
 * various operations.
 */
#define MQTT_TASK_QUEUE_LENGTH           (3u)

/* Maximum length for client identifier */
#define MQTT_CLIENT_IDENTIFIER_MAX_LEN    ( 23 )

/* Length of Tenant ID used for MQTT Connection to the Sensor Cloud */
#define TENANT_ID_LEN                    (64)

/* Time in milliseconds to wait before creating the publisher task. */
#define TASK_CREATION_DELAY_MS           (2000u)

/* Flag Masks for tracking which cleanup functions must be called. */
#define WCM_INITIALIZED                  (1lu << 0)
#define WIFI_CONNECTED                   (1lu << 1)
#define LIBS_INITIALIZED                 (1lu << 2)
#define BUFFER_INITIALIZED               (1lu << 3)
#define MQTT_INSTANCE_CREATED            (1lu << 4)
#define MQTT_CONNECTION_SUCCESS          (1lu << 5)
#define MQTT_MSG_RECEIVED                (1lu << 6)

/* Macros used for constructing the last will message(LMT) topic */
#define MQTT_TOPIC_LASTWILL_START           "lwt/things/"
#define MQTT_TOPIC_LASTWILL_END             "/shadow/update"
#define MQTT_TOPIC_LASTWILL_TOPIC_LEN        (64)

/* Macros used for constructing Telemetry, Publish and subscribe topic names */
#define TELEMETRY                           "/telemetry"
#define AWS_THING_START                     "$aws/things/"
#define AWS_SUB_DEVICE_PROPERTIES           "/shadow/update/delta"
#define AWS_PUB_DEVICE_PROPERTIES           "/shadow/update"

/* Last will testament message */
#define MQTT_LAST_WILL_MESSAGE              "{\"state\":{\"reported\":{\"ConnectedStatus\":\"Disconnected\"},\"desired\":null}}"
/* Macro to check if the result of an operation was successful and set the 
 * corresponding bit in the status_flag based on 'init_mask' parameter. When 
 * it has failed, print the error message and return the result to the 
 * calling function.
 */
#define CHECK_RESULT(result, init_mask, error_message...)      \
                     do                                        \
                     {                                         \
                         if ((int)result == CY_RSLT_SUCCESS)   \
                         {                                     \
                             status_flag |= init_mask;         \
                         }                                     \
                         else                                  \
                         {                                     \
                             printf(error_message);            \
                             return result;                    \
                         }                                     \
                     } while(0)



/******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT connection handle. */
cy_mqtt_t mqtt_connection;

/* Queue handle used to communicate results of various operations - MQTT 
 * Publish, MQTT Subscribe, MQTT connection, and Wi-Fi connection between tasks 
 * and callbacks.
 */
QueueHandle_t mqtt_task_q;

/* Flag to denote initialization status of various operations. */
uint32_t status_flag;

/* Pointer to the network buffer needed by the MQTT library for MQTT send and 
 * receive operations.
 */
uint8_t *mqtt_network_buffer = NULL;

/*MQTT topics for publish and subscribe to MQTT broker*/
uint8_t mqtt_topic_publish_telemetry [MQTT_TOPIC_PUBLISH_TELEMETRY_LEN];
uint8_t mqtt_topic_publish_device_properties [MQTT_TOPIC_PUBLISH_DEVICE_PROPERTIES_LEN];
uint8_t mqtt_topic_subscribe_device_properties [MQTT_TOPIC_SUBSCRIBE_LEN];
uint8_t mqtt_topic_lastwill [MQTT_TOPIC_LASTWILL_TOPIC_LEN];

/******************************************************************************
* Function Prototypes
*******************************************************************************/
static cy_rslt_t wifi_connect(void);
static cy_rslt_t mqtt_init(void);
static cy_rslt_t mqtt_connect(void);
void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data);
static void cleanup(void);
static subs_rslt_t construct_mqtt_topics();
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier);

/******************************************************************************
 * Function Name: mqtt_client_task
 ******************************************************************************
 * Summary:
 *  Task for handling initialization & connection of Wi-Fi and the MQTT client.
 *  The task also creates and manages the subscriber and publisher tasks upon 
 *  successful MQTT connection. The task also handles the WiFi and MQTT 
 *  connections by initiating reconnection on the event of disconnections.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_client_task(void *pvParameters)
{
    /* Structures that store the data to be sent/received to/from various
     * message queues.
     */
    mqtt_task_cmd_t mqtt_status;
    subscriber_data_t subscriber_q_data;
    publisher_data_t publisher_q_data;
    /* Configure the Wi-Fi interface as a Wi-Fi STA (i.e. Client). */
    cy_wcm_config_t config = {.interface = CY_WCM_INTERFACE_TYPE_STA};
    /* to store return values */
    cy_rslt_t rslt;
    subs_rslt_t rc;
    /* RTC variables */
    cyhal_rtc_t rtc_obj;
    cyhal_rtc_t rtc_instance;
    
    rslt = cy_log_init(CY_LOG_MAX, NULL, NULL);
        if (rslt != CY_RSLT_SUCCESS)
        {
            printf("cy_log_init() FAILED %ld\n", rslt);
        }

    /* RTC will already be initialized by CM0p core, the following lines prevent RTC re-intialization */
    cy_set_rtc_instance(&rtc_instance);
    
    rslt = cyhal_rtc_init(&rtc_obj);
    if (CY_RSLT_SUCCESS != rslt)
    {
        APP_LOG_ERROR(("RTC Init failed"));
    }

    /* To avoid compiler warnings */
    (void) pvParameters;

    /* Create a message queue to communicate with other tasks and callbacks. */
    mqtt_task_q = xQueueCreate(MQTT_TASK_QUEUE_LENGTH, sizeof(mqtt_task_cmd_t));

    /* Initialize the Wi-Fi Connection Manager and jump to the cleanup block 
     * upon failure.
     */
    if (CY_RSLT_SUCCESS != cy_wcm_init(&config))
    {
        APP_LOG_ERROR(("Wi-Fi Connection Manager initialization failed!"));
        goto exit_cleanup;
    }

    /* Set the appropriate bit in the status_flag to denote successful 
     * WCM initialization.
     */
    status_flag |= WCM_INITIALIZED;
    APP_LOG_INFO(("Wi-Fi Connection Manager initialized."));

    /* Initiate connection to the Wi-Fi AP and cleanup if the operation fails. */
    if (CY_RSLT_SUCCESS != wifi_connect())
    {
        goto exit_cleanup;
    }
    
    /* Construct MQTT Topics, these will be used by publisher and subscriber tasks */
    rc = construct_mqtt_topics();
    if (rc != SUBS_SUCCESS) {
        APP_LOG_ERROR(("MQTT Topic construction failed"));
        goto exit_cleanup;
    }
    /* Set-up the MQTT client and connect to the MQTT broker. Jump to the 
     * cleanup block if any of the operations fail.
     */
    if ( (CY_RSLT_SUCCESS != mqtt_init()) || (CY_RSLT_SUCCESS != mqtt_connect()) )
    {
        goto exit_cleanup;
    }
    
    /* Create the subscriber task and cleanup if the operation fails. */
    if (pdPASS != xTaskCreate(subscriber_task, "Subscriber task", SUBSCRIBER_TASK_STACK_SIZE,
                              NULL, SUBSCRIBER_TASK_PRIORITY, &subscriber_task_handle))
    {
        APP_LOG_ERROR(("Failed to create the Subscriber task!"));
        goto exit_cleanup;
    }

    /* Create the publisher task and cleanup if the operation fails. */
    if (pdPASS != xTaskCreate(publisher_task, "Publisher task", PUBLISHER_TASK_STACK_SIZE, 
                              NULL, PUBLISHER_TASK_PRIORITY, &publisher_task_handle))
    {
        APP_LOG_ERROR(("Failed to create the Publisher task!"));
        goto exit_cleanup;
    }
	
	/* Initializes context object of Radar Sensing library, sets default */
    /* parameters values for sensor and continuously acquire data from sensor. */
    if (pdPASS != xTaskCreate(radar_task, RADAR_TASK_NAME, RADAR_TASK_STACK_SIZE,
                              NULL, RADAR_TASK_PRIORITY, &radar_task_handle))
    {
        printf("Failed to create '%s' task!\n", RADAR_TASK_NAME);
        goto exit_cleanup;
    }



    /* Wait for the task creation to complete. */
    vTaskDelay(pdMS_TO_TICKS(TASK_CREATION_DELAY_MS));

    /* Start exchanging messages with cloud */
    publisher_q_data.cmd = PUBLISHER_INIT;
    xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

    while (true)
    {
        /* Wait for results of MQTT operations from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(mqtt_task_q, &mqtt_status, portMAX_DELAY))
        {
            /* In this code example, the disconnection from the MQTT Broker or 
             * the Wi-Fi network is handled by the case 'HANDLE_DISCONNECTION'. 
             * 
             * The publish and subscribe failures (`HANDLE_MQTT_PUBLISH_FAILURE`
             * and `HANDLE_MQTT_SUBSCRIBE_FAILURE`) does not initiate 
             * reconnection in this example, but they can be handled as per the 
             * application requirement in the following switch cases.
             */
            switch(mqtt_status)
            {
                case HANDLE_DISCONNECTION:
                {
                    /* Deinit the publisher before initiating reconnection. */
                    publisher_q_data.cmd = PUBLISHER_DEINIT;
                    xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

                    /* Although the connection with the MQTT Broker is lost, 
                     * call the MQTT disconnect API for cleanup of threads and 
                     * other resources before reconnection.
                     */
                    cy_mqtt_disconnect(mqtt_connection);

                    /* Check if Wi-Fi connection is active. If not, update the 
                     * status flag and initiate Wi-Fi reconnection.
                     */
                    if (cy_wcm_is_connected_to_ap() == 0)
                    {
                        status_flag &= ~(WIFI_CONNECTED);
                        APP_LOG_INFO(("Initiating Wi-Fi Reconnection..."));
                        if (CY_RSLT_SUCCESS != wifi_connect())
                        {
                            goto exit_cleanup;
                        }
                    }

                    APP_LOG_INFO(("Initiating MQTT Reconnection..."));
                    if (CY_RSLT_SUCCESS != mqtt_connect())
                    {
                        goto exit_cleanup;
                    }

                    /* Initiate MQTT subscribe post the reconnection. */
                    subscriber_q_data.cmd = SUBSCRIBE_TO_TOPIC;
                    xQueueSend(subscriber_task_q, &subscriber_q_data, portMAX_DELAY);

                    /* Initialize Publisher post the reconnection. */
                    publisher_q_data.cmd = PUBLISHER_INIT;
                    xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);
                    break;
                }

                default:
                    break;
            }
        }
    }

    /* Cleanup section: Delete subscriber and publisher tasks and perform
     * cleanup for various operations based on the status_flag.
     */
    exit_cleanup:
    APP_LOG_INFO(("Terminating Publisher and Subscriber tasks..."));
    if (subscriber_task_handle != NULL)
    {
        vTaskDelete(subscriber_task_handle);
    }

    if (publisher_task_handle != NULL)
    {
        vTaskDelete(publisher_task_handle);
    }

	if (radar_task_handle != NULL)
    {
        radar_task_cleanup();
        vTaskDelete(radar_task_handle);
    }
    cleanup();
    APP_LOG_INFO(("Cleanup Done\nTerminating the MQTT task..."));
    vTaskDelete(NULL);
}

/******************************************************************************
 * Function Name: wifi_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates connection to the Wi-Fi Access Point using the 
 *  specified SSID and PASSWORD. The connection is retried a maximum of 
 *  'MAX_WIFI_CONN_RETRIES' times with interval of 'WIFI_CONN_RETRY_INTERVAL_MS'
 *  milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful Wi-Fi connection, else an 
 *              error code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t wifi_connect(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;

    /* Check if Wi-Fi connection is already established. */
    if (cy_wcm_is_connected_to_ap() == 0)
    {
        /* Configure the connection parameters for the Wi-Fi interface. */
        memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
        memcpy(connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
        memcpy(connect_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
        connect_param.ap_credentials.security = WIFI_SECURITY;

        APP_LOG_INFO(("Connecting to Wi-Fi AP '%s'", connect_param.ap_credentials.SSID));

        /* Connect to the Wi-Fi AP. */
        for (uint32_t retry_count = 0; retry_count < MAX_WIFI_CONN_RETRIES; retry_count++)
        {
            result = cy_wcm_connect_ap(&connect_param, &ip_address);

            if (result == CY_RSLT_SUCCESS)
            {
                APP_LOG_INFO(("Successfully connected to Wi-Fi network '%s'.", connect_param.ap_credentials.SSID));
                
                /* Set the appropriate bit in the status_flag to denote 
                 * successful Wi-Fi connection, print the assigned IP address.
                 */
                status_flag |= WIFI_CONNECTED;
                if (ip_address.version == CY_WCM_IP_VER_V4)
                {
                    APP_LOG_INFO(("IPv4 Address Assigned: %s", ip4addr_ntoa((const ip4_addr_t *) &ip_address.ip.v4)));
                }
                else if (ip_address.version == CY_WCM_IP_VER_V6)
                {
                    APP_LOG_INFO(("IPv6 Address Assigned: %s", ip6addr_ntoa((const ip6_addr_t *) &ip_address.ip.v6)));
                }
                return result;
            }
            APP_LOG_ERROR(("Connection to Wi-Fi network failed with error code 0x%0X. Retrying in %d ms. Retries left: %d",
                (int)result, WIFI_CONN_RETRY_INTERVAL_MS, (int)(MAX_WIFI_CONN_RETRIES - retry_count - 1)));
            vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MS));
        }

        APP_LOG_ERROR(("Exceeded maximum Wi-Fi connection attempts! Wi-Fi connection failed"));
    }
    return result;
}

/******************************************************************************
 * Function Name: mqtt_init
 ******************************************************************************
 * Summary:
 *  Function that initializes the MQTT library and creates an instance for the 
 *  MQTT client. The network buffer needed by the MQTT library for MQTT send 
 *  send and receive operations is also allocated by this function.  
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS on a successful initialization, else an error
 *              code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t mqtt_init(void)
{
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the MQTT library. */
    result = cy_mqtt_init();
    CHECK_RESULT(result, LIBS_INITIALIZED, "MQTT library initialization failed!\n\n");

    /* Allocate buffer for MQTT send and receive operations. */
    mqtt_network_buffer = (uint8_t *) pvPortMalloc(sizeof(uint8_t) * MQTT_NETWORK_BUFFER_SIZE);
    if(mqtt_network_buffer == NULL)
    {
        result = ~CY_RSLT_SUCCESS;
    }
    CHECK_RESULT(result, BUFFER_INITIALIZED, "Network Buffer allocation failed!\n\n");

    security_info->client_cert_size = CLOUD_CERT_KEY_LEN;
    security_info->private_key_size = CLOUD_CERT_KEY_LEN;
    security_info->root_ca_size = CLOUD_CERT_KEY_LEN;

    /* Create the MQTT client instance. */
    result = cy_mqtt_create(mqtt_network_buffer, MQTT_NETWORK_BUFFER_SIZE,
                            security_info, &broker_info,
                            (cy_mqtt_callback_t)mqtt_event_callback, NULL,
                            &mqtt_connection);

    CHECK_RESULT(result, MQTT_INSTANCE_CREATED, "MQTT instance creation failed!\n\n");
    APP_LOG_INFO(("MQTT library initialization successful."));

    return result;
}

/******************************************************************************
 * Function Name: mqtt_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates MQTT connect operation. The connection is retried
 *  a maximum of 'MAX_MQTT_CONN_RETRIES' times with interval of 
 *  'MQTT_CONN_RETRY_INTERVAL_MS' milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful MQTT connection, else an 
 *              error code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t mqtt_connect(void)
{
    /* Variable to indicate status of various operations. */
    cy_rslt_t result = CY_RSLT_SUCCESS;
    
    /* MQTT client identifier string. */
    char mqtt_client_identifier[(MQTT_CLIENT_IDENTIFIER_MAX_LEN + 1)];

    /* Generate a unique client identifier using the Device Id 
     */
    result = mqtt_get_unique_client_identifier(mqtt_client_identifier);
    CHECK_RESULT(result, 0, "Failed to generate unique client identifier for the MQTT client!\n");

    /* Set the client identifier buffer and length. */
    connection_info.client_id = mqtt_client_identifier;
    connection_info.client_id_len = strlen(mqtt_client_identifier);
    /* Set the last will message */
    connection_info.will_info->qos = CY_MQTT_QOS1;
    connection_info.will_info->topic = (char*)mqtt_topic_lastwill;
    connection_info.will_info->topic_len = (strlen((char*)mqtt_topic_lastwill));
    connection_info.will_info->payload = (char*)MQTT_LAST_WILL_MESSAGE;
    connection_info.will_info->payload_len = (strlen((char*)MQTT_LAST_WILL_MESSAGE));
    connection_info.will_info->retain = false;
    connection_info.will_info->dup = false;

    APP_LOG_INFO(("MQTT client '%.*s' connecting to MQTT broker '%.*s'...",
           connection_info.client_id_len,
           connection_info.client_id,
           broker_info.hostname_len,
           broker_info.hostname));

    for (uint32_t retry_count = 0; retry_count < MAX_MQTT_CONN_RETRIES; retry_count++)
    {
        if (cy_wcm_is_connected_to_ap() == 0)
        {
            APP_LOG_ERROR(("Unexpectedly disconnected from Wi-Fi network! Initiating Wi-Fi reconnection..."));
            status_flag &= ~(WIFI_CONNECTED);

            /* Initiate Wi-Fi reconnection. */
            result = wifi_connect();
            if (CY_RSLT_SUCCESS != result)
            {
                return result;
            }
        }

        /* Establish the MQTT connection. */
        
        result = cy_mqtt_connect(mqtt_connection, &connection_info);
        if (result == CY_RSLT_SUCCESS)
        {
            APP_LOG_INFO(("MQTT connection successful."));

            /* Set the appropriate bit in the status_flag to denote successful
             * MQTT connection, and return the result to the calling function.
             */
            status_flag |= MQTT_CONNECTION_SUCCESS;
            return result;
        }

        APP_LOG_ERROR(("MQTT connection failed with error code 0x%0X. Retrying in %d ms. Retries left: %d", 
               (int)result, MQTT_CONN_RETRY_INTERVAL_MS, (int)(MAX_MQTT_CONN_RETRIES - retry_count - 1)));
        vTaskDelay(pdMS_TO_TICKS(MQTT_CONN_RETRY_INTERVAL_MS));
    }

    APP_LOG_ERROR(("Exceeded maximum MQTT connection attempts! MQTT connection failed"));
    return result;
}

/******************************************************************************
 * Function Name: mqtt_event_callback
 ******************************************************************************
 * Summary:
 *  Callback invoked by the MQTT library for events like MQTT disconnection, 
 *  incoming MQTT subscription messages from the MQTT broker. 
 *    1. In case of MQTT disconnection, the MQTT client task is communicated 
 *       about the disconnection using a message queue. 
 *    2. When an MQTT subscription message is received, the subscriber callback
 *       function implemented in subscriber_task.c is invoked to handle the 
 *       incoming MQTT message.
 *
 * Parameters:
 *  cy_mqtt_t mqtt_handle : MQTT handle corresponding to the MQTT event (unused)
 *  cy_mqtt_event_t event : MQTT event information
 *  void *user_data : User data pointer passed during cy_mqtt_create() (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void mqtt_event_callback(cy_mqtt_t mqtt_handle, cy_mqtt_event_t event, void *user_data)
{
    cy_mqtt_publish_info_t *received_msg;
    mqtt_task_cmd_t mqtt_task_cmd;

    (void) mqtt_handle;
    (void) user_data;

    switch(event.type)
    {
        case CY_MQTT_EVENT_TYPE_DISCONNECT:
        {
            /* Clear the status flag bit to indicate MQTT disconnection. */
            status_flag &= ~(MQTT_CONNECTION_SUCCESS);

            /* MQTT connection with the MQTT broker is broken as the client
             * is unable to communicate with the broker. Set the appropriate
             * command to be sent to the MQTT task.
             */
            APP_LOG_ERROR(("Unexpectedly disconnected from MQTT broker!"));
            mqtt_task_cmd = HANDLE_DISCONNECTION;

            /* Send the message to the MQTT client task to handle the 
             * disconnection. 
             */
            xQueueSend(mqtt_task_q, &mqtt_task_cmd, portMAX_DELAY);
            break;
        }

        case CY_MQTT_EVENT_TYPE_SUBSCRIPTION_MESSAGE_RECEIVE:
        {
            status_flag |= MQTT_MSG_RECEIVED;

            /* Incoming MQTT message has been received. Send this message to 
             * the subscriber callback function to handle it. 
             */
            received_msg = &(event.data.pub_msg.received_message);
            mqtt_subscription_callback(received_msg);
            break;
        }
        default :
        {
            /* Unknown MQTT event */
            APP_LOG_ERROR(("Unknown Event received from MQTT callback!"));
            break;
        }
    }
}

/******************************************************************************
 * Function Name: mqtt_get_unique_client_identifier
 ******************************************************************************
 * Summary:
 *  Function that retrieves unique id to be used as client identifier
 *
 * Parameters:
 *  char *mqtt_client_identifier : Pointer to the string that stores the 
 *                                 generated unique identifier
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS on successful generation of the client 
 *              identifier, else a non-zero value indicating failure.
 *
 ******************************************************************************/
static cy_rslt_t mqtt_get_unique_client_identifier(char *mqtt_client_identifier)
{
    cy_rslt_t status = CY_RSLT_SUCCESS;
    /* Get the Unique ID, this will be the Cliend ID */
    uint64_t unique_id;
    unique_id = Cy_SysLib_GetUniqueId();

    /* Check for errors from snprintf. */
    if (0 > snprintf(mqtt_client_identifier,
                     (MQTT_CLIENT_IDENTIFIER_MAX_LEN + 1),
                     "%08lx%08lx",
                    (uint32_t)(unique_id>>32), (uint32_t)unique_id))
    {
        status = ~CY_RSLT_SUCCESS;
    }

    APP_LOG_DEBUG(("mqtt client identifier = %s", mqtt_client_identifier));

    return status;
}

/******************************************************************************
 * Function Name: cleanup
 ******************************************************************************
 * Summary:
 *  Function that invokes the deinit and cleanup functions for various 
 *  operations based on the status_flag.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  void
 *
 ******************************************************************************/
static void cleanup(void)
{
    /* Disconnect the MQTT connection if it was established. */
    if (status_flag & MQTT_CONNECTION_SUCCESS)
    {
        APP_LOG_ERROR(("Disconnecting from the MQTT Broker..."));
        cy_mqtt_disconnect(mqtt_connection);
    }
    /* Delete the MQTT instance if it was created. */
    if (status_flag & MQTT_INSTANCE_CREATED)
    {
        cy_mqtt_delete(mqtt_connection);
    }
    /* Deallocate the network buffer. */
    if (status_flag & BUFFER_INITIALIZED)
    {
        vPortFree((void *) mqtt_network_buffer);
    }
    /* Deinit the MQTT library. */
    if (status_flag & LIBS_INITIALIZED)
    {
        cy_mqtt_deinit();
    }
    /* Disconnect from Wi-Fi AP. */
    if (status_flag & WIFI_CONNECTED)
    {
        if (cy_wcm_disconnect_ap() == CY_RSLT_SUCCESS)
        {
            APP_LOG_ERROR(("Disconnected from the Wi-Fi AP!"));
        }
    }
    /* De-initialize the Wi-Fi Connection Manager. */
    if (status_flag & WCM_INITIALIZED)
    {
        cy_wcm_deinit();
    }
}

/*******************************************************************************
* Function Name: construct_mqtt_topics
********************************************************************************
* Summary:
*   Construct MQTT topics using parameters such as device ID,
*   tenant ID.
*
* Parameters:
*  void
*
* Return:
*  subs_rslt_t
*
*******************************************************************************/
static subs_rslt_t construct_mqtt_topics(void)
{
    char mqtt_client_identifier[(MQTT_CLIENT_IDENTIFIER_MAX_LEN + 1)];
    uint8_t cloud_tenant_id[TENANT_ID_LEN];
    cy_rslt_t result = CY_RSLT_SUCCESS;
    /* Generate a unique client identifier using the device Id */
    result = mqtt_get_unique_client_identifier(mqtt_client_identifier);
    CHECK_RESULT(result, 0, "Failed to generate unique client identifier for the MQTT client!\n");

    strncpy((char*)cloud_tenant_id, TENANT_ID, strlen(TENANT_ID) + 1);

    snprintf((char *)mqtt_topic_publish_telemetry, sizeof(mqtt_topic_publish_telemetry), "%s%s%s%s", cloud_tenant_id, "/", mqtt_client_identifier, TELEMETRY);
    snprintf((char *)mqtt_topic_publish_device_properties, sizeof(mqtt_topic_publish_device_properties), "%s%s%s", AWS_THING_START, mqtt_client_identifier, AWS_PUB_DEVICE_PROPERTIES);
    snprintf((char *)mqtt_topic_subscribe_device_properties, sizeof(mqtt_topic_subscribe_device_properties), "%s%s%s", AWS_THING_START, mqtt_client_identifier, AWS_SUB_DEVICE_PROPERTIES);
    snprintf((char *)mqtt_topic_lastwill, sizeof(mqtt_topic_lastwill), "%s%s%s", MQTT_TOPIC_LASTWILL_START, mqtt_client_identifier, MQTT_TOPIC_LASTWILL_END);
    return SUBS_SUCCESS;
}
/* [] END OF FILE */
