/******************************************************************************
* File Name:   publisher_task.c
*
* Description: This file contains the task that publishes the firmware version, 
*              device properties update acknowledgement and sensor values to 
*              the Sensor Cloud. This file also inits and runs a timer for sensor read 
*              depending on the measurement period specified from cloud
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
#include "FreeRTOS.h"

/* Task header files */
#include "publisher_task.h"
#include "mqtt_task.h"
#include "subscriber_task.h"
#include "device_properties.h"

/* Configuration file for MQTT client */
#include "mqtt_client_config.h"

/* Middleware libraries */
#include "cy_mqtt_api.h"
#include "cy_retarget_io.h"
#include "cyabs_rtos.h"
#include "radar_config_task.h"
/******************************************************************************
* Macros
******************************************************************************/

/* Queue length of a message queue that is used to communicate with the 
 * publisher task.
 */

#define PUBLISHER_TASK_QUEUE_LENGTH                     (10u)
#define DEFAULT_RADAR_MAX_RANGE							(2.0)
#define DEFAULT_RADAR_MACRO_THRESHOLD					(1.0)
#define DEFAULT_RADAR_MICRO_THRESHOLD					(25.0)
#define DEFAULT_RADAR_MODE								(2)
#define CONVERT_TO_MS                                   (1000)
#define SENSOR_TELEMETRY_BUFFER_SIZE					(240)
#define PRESENCE_OUT_EVENT								(0)
#define PRESENCE_MACRO_EVENT							(1)
#define PRESENCE_MICRO_EVENT							(2)

/******************************************************************************
* Global Variables
*******************************************************************************/
/* FreeRTOS task handle for this task. */
TaskHandle_t publisher_task_handle;

/* Handle of the queue holding the commands for the publisher task */
QueueHandle_t publisher_task_q;

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/******************************************************************************
 * Function Name: publisher_task
 ******************************************************************************
 * Summary:
 *  Task that sets up the sensor read timer, constructs messages and publishes them to sensor cloud.
 *
 * Parameters:
 *  void *pvParameters : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 ******************************************************************************/
void publisher_task(void *pvParameters)
{
    /* Status variable */
    subs_rslt_t rc;
    publisher_data_t publisher_q_data;
	radar_config_data_t radar_config_q_data;
	char *buffer_to_publish = NULL;
	int sensor = RADAR_SENSOR+1;
	int board = RADAR_BOARD+1;

    /* To avoid compiler warnings */
    (void) pvParameters;
	
	/* Allocate memory for buffer to publish */
	buffer_to_publish = (char *)calloc(DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE, sizeof(char));
	if(NULL == buffer_to_publish)
	{
		APP_LOG_ERROR(( "Failed to allocate memory for buffer_to_publish"));
		return;
	}
    

    radar_presence_attributes_t radar_presence_attributes =
    	{
    		.max_range = DEFAULT_RADAR_MAX_RANGE,
    		.macro_threshold = DEFAULT_RADAR_MACRO_THRESHOLD,
			.micro_threshold = DEFAULT_RADAR_MICRO_THRESHOLD,
			.mode = DEFAULT_RADAR_MODE
    	};


    /* Create a message queue to communicate with other tasks and callbacks. */
    publisher_task_q = xQueueCreate(PUBLISHER_TASK_QUEUE_LENGTH, sizeof(publisher_data_t));

    while (true)
    {
        /* Wait for commands from other tasks and callbacks. */
        if (pdTRUE == xQueueReceive(publisher_task_q, &publisher_q_data, portMAX_DELAY))
        {
            switch(publisher_q_data.cmd)
            {
                case PUBLISHER_INIT:
                {
                    /* Initiate the communication with cloud by sending the first message */
                    publisher_q_data.cmd = PUBLISH_FW_VERSION;
                    xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);
                    break;
                }

                case PUBLISHER_DEINIT:
                {
                    break;
                }

                case PUBLISH_FW_VERSION:
                {
                    /* Publish the fimrware version */
                    APP_LOG_DEBUG(("PUBLISH_FW_VERSION"));
                    publish_device_properties(PUB_FW_VERSION, radar_presence_attributes);
                    break;
                }

                case PUBLISH_DEVICE_PROPERTIES_UPDATE_ACK:
                {
                    /* Publish acknowledgement for the device properties */
                    APP_LOG_DEBUG(("PUBLISH_DEVICE_PROPERTIES_UPDATE_ACK"));
                    publish_device_properties(PUB_DEVICE_PROPERTIES_ACK, radar_presence_attributes);
                    break;
                }


				case PUBLISH_RADAR_TELEMETRY:
				{
					/* Clear buffer */
					buffer_to_publish[0] = '\0';
					/*Initialise buffer_to_publish with no characters*/
					strncpy(buffer_to_publish, "", 1);
					/* Current RTC Time */
					uint32_t time_val = (uint32_t)time(NULL);
					if ( publisher_q_data.event == XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE)
					{
						snprintf(buffer_to_publish, SENSOR_TELEMETRY_BUFFER_SIZE, "{\"e\":{\"n\":\"RDR_SENSOR_PRESENCE_IN_OUT_EVENT\",\"io\":%u,\"b\":%u,\"s\":%u,\"d\":%.2f,\"t\":%lu}}",
							PRESENCE_MACRO_EVENT, board, sensor,publisher_q_data.distance, time_val);
						//APP_LOG_INFO(("XENSIV_RADAR_PRESENCE_STATE_MACRO_PRESENCE buffer_to_publish = %s", buffer_to_publish));
					}

					else if (publisher_q_data.event == XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE)
					{
						snprintf(buffer_to_publish, SENSOR_TELEMETRY_BUFFER_SIZE, "{\"e\":{\"n\":\"RDR_SENSOR_PRESENCE_IN_OUT_EVENT\",\"io\":%u,\"b\":%u,\"s\":%u,\"d\":%.2f,\"t\":%lu}}",
								PRESENCE_MICRO_EVENT, board, sensor, publisher_q_data.distance, time_val);
						//APP_LOG_INFO(("XENSIV_RADAR_PRESENCE_STATE_MICRO_PRESENCE buffer_to_publish = %s", buffer_to_publish));
					}
					
					else
					{
						snprintf(buffer_to_publish, SENSOR_TELEMETRY_BUFFER_SIZE, "{\"e\":{\"n\":\"RDR_SENSOR_PRESENCE_IN_OUT_EVENT\",\"io\":%u,\"b\":%u,\"s\":%u,\"d\":%.2f,\"t\":%lu}}",
								PRESENCE_OUT_EVENT, board, sensor, time_val);
						//APP_LOG_INFO(("ABSENCE buffer_to_publish = %s", buffer_to_publish));
					}
					APP_LOG_DEBUG(("buffer_to_publish = %s", buffer_to_publish));


					/* Publish the message to respective topic */
					rc = publish_to_mqtt_topic(buffer_to_publish, strnlen(buffer_to_publish, DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE)+1, 
									(char*)mqtt_topic_publish_telemetry);
		
					if(SUBS_SUCCESS != rc)
					{
						APP_LOG_ERROR(("Device Properties publish failed %d", rc));
					}
					else
					{
						APP_LOG_DEBUG(("Device properties reported %d", rc));
					}
					break;
				}
				case UPDATE_RADAR_MAX_RANGE:
					{
						radar_presence_attributes.max_range = publisher_q_data.max_range;

						/* Notify radar_config_task to set radar configuration */
						radar_config_q_data.cmd = UPDATE_RADAR_PRESENCE_MAX_RANGE_CONFIG;
						radar_config_q_data.max_range = radar_presence_attributes.max_range;

						xQueueSend(radar_config_task_q, &radar_config_q_data, portMAX_DELAY);
						break;
					}

				case UPDATE_RADAR_MACRO_THRESHOLD:
					{
						radar_presence_attributes.macro_threshold = publisher_q_data.macro_threshold;

						/* Notify radar_config_task to set radar configuration */
						radar_config_q_data.cmd = UPDATE_RADAR_MACRO_THRESHOLD_CONFIG;
						radar_config_q_data.macro_threshold = radar_presence_attributes.macro_threshold;

						xQueueSend(radar_config_task_q, &radar_config_q_data, portMAX_DELAY);
						break;
					}

				case UPDATE_RADAR_MICRO_THRESHOLD:
					{
						radar_presence_attributes.micro_threshold = publisher_q_data.micro_threshold;

						/* Notify radar_config_task to set radar configuration */
						radar_config_q_data.cmd = UPDATE_RADAR_MICRO_THRESHOLD_CONFIG;
						radar_config_q_data.micro_threshold = radar_presence_attributes.micro_threshold;

						xQueueSend(radar_config_task_q, &radar_config_q_data, portMAX_DELAY);
						break;
					}

				case UPDATE_RADAR_MODE:
					{
						radar_presence_attributes.mode = publisher_q_data.mode;	// TODO: Is this is Required?

						/* Notify radar_config_task to set radar configuration */
						radar_config_q_data.cmd = UPDATE_RADAR_MODE_CONFIG;
						radar_config_q_data.mode = radar_presence_attributes.mode;

						xQueueSend(radar_config_task_q, &radar_config_q_data, portMAX_DELAY);
						break;
					}
            }
        }
    }
	// Cleanup garbage
    if(NULL != buffer_to_publish)
    {
        free(buffer_to_publish);
        buffer_to_publish = NULL;
    }
}

/******************************************************************************
 * Function Name: publish_to_mqtt_topic
 ******************************************************************************
 * Summary:
 *  Publish messages to given topic
 *
 * Parameters:
 *  char* publish_message : Message to be published
 *  int publish_message_length : Message length to be published
 *  char* mqtt_topic : Topic that the message has to be published to
 *
 * Return:
 *  subs_rslt_t - SUBS_SUCCESS on success
 *
 ******************************************************************************/
subs_rslt_t publish_to_mqtt_topic(char* publish_message, int publish_message_length, char* mqtt_topic)
{
    cy_rslt_t result;

    /* Construct the message to be published*/
    cy_mqtt_publish_info_t publish_info =
    {
        .qos = (cy_mqtt_qos_t) MQTT_MESSAGES_QOS,
        .topic = mqtt_topic,
        .topic_len = (strlen(mqtt_topic)),
        .retain = false,
        .dup = false,
        .payload = publish_message,
        .payload_len = publish_message_length
    };

    APP_LOG_DEBUG(("Publish to Topic[%s]: Publishing[%d]: %s", publish_info.topic, publish_info.payload_len, publish_info.payload));
    
    /* Publish the message */
    result = cy_mqtt_publish(mqtt_connection, &publish_info);

    if (result != CY_RSLT_SUCCESS)
    {
        APP_LOG_ERROR(("Publisher: MQTT Publish failed with error 0x%0X", (int)result));
    }
    return result;
}

/* [] END OF FILE */
