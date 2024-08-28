/*******************************************************************************
* File Name: device_porperties.c
*
* Description: This file contains functions for constructing and publishing messages in the JSON format expected by the Sensor Cloud
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

#include "common_variables.h"
#include "device_properties.h"
#include "mqtt_task.h"
#include "cy_time.h" 
#include "publisher_task.h"
#include "radar_task.h"
#include "stdio.h"
/*******************************************************************************
*  Macros
*******************************************************************************/
#define FW_VERSION_SIZE                             (16)
#define GET_DESIRED_PROPERTIES_STATE_TRUE           (1u)
#define GET_DESIRED_PROPERTIES_STATE_FALSE          (0u)
#define RX_DP_BUFFER_LENGTH                         (64)
#define MODE_LEN							  (20)


/*JSON keys for device paramters*/
#define PARAMS_PARENT_OBJECT                        "state"
#define RDR_PRESENCE_MAX_RANGE           			"max_range"
#define RDR_PRESENCE_MACRO_THRESHOLD          		"macro_threshold"
#define RDR_PRESENCE_MICRO_THRESHOLD          		"micro_threshold"
#define RDR_PRESENCE_MODE         					"mode"

/* Max range min - max */
#define MAX_RANGE_MIN_LIMIT (0.66f)
#define MAX_RANGE_MAX_LIMIT (10.2f)

/* Macro threshold min - max */
#define MACRO_THRESHOLD_MIN_LIMIT (0.1f)
#define MACRO_THRESHOLD_MAX_LIMIT (100.0f)

/* Micro threshold min - max */
#define MICRO_THRESHOLD_MIN_LIMIT (0.2f)
#define MICRO_THRESHOLD_MAX_LIMIT (99.0f)

/* Names for presence mode */
#define MACRO_ONLY_STRING      ("macro_only")
#define MICRO_ONLY_STRING      ("micro_only")
#define MICRO_IF_MACRO_STRING  ("micro_if_macro")
#define MICRO_AND_MACRO_STRING ("micro_and_macro")

/*******************************************************************************
*  Global Variables
*******************************************************************************/

/* macros used for json construction */
typedef enum {
    FLOAT_VALUE = 0,
    INT_VALUE = 1,
    CHAR_VALUE = 2
} json_value_type_t;

/*******************************************************************************
*  Function Signatures
*******************************************************************************/
cy_rslt_t   parse_device_properties(cy_JSON_object_t *json_object, void* callback_arg);
subs_rslt_t compare_and_store(cy_JSON_object_t* json_object, void* device_parameter,
                                 char* expected_key, char* expected_parent_key, bool is_float);
subs_rslt_t publish_device_properties (pub_msg_type_t pub_msg_type, radar_presence_attributes_t device_attributes);

static inline xensiv_radar_presence_mode_t string_to_mode(const char *mode);



static inline xensiv_radar_presence_mode_t string_to_mode(const char *mode) {
    xensiv_radar_presence_mode_t result = XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO;

    if (strcmp(mode, MACRO_ONLY_STRING) == 0) {
        result = XENSIV_RADAR_PRESENCE_MODE_MACRO_ONLY;
    } else if (strcmp(mode, MICRO_ONLY_STRING) == 0) {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_ONLY;
    } else if (strcmp(mode, MICRO_IF_MACRO_STRING) == 0) {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_IF_MACRO;
    } else if (strcmp(mode, MICRO_AND_MACRO_STRING) == 0) {
        result = XENSIV_RADAR_PRESENCE_MODE_MICRO_AND_MACRO;
    } else {
    }

    return result;
}


/*******************************************************************************
* Function Name: publish_device_properties
********************************************************************************
* Summary:
* Publish fw_version, device properties acknowledgement and sensor values
*
* Parameters:
*  pub_msg_type_t pub_msg_type: Message type to be published

* Return:
*  subs_rslt_t - SUBS_SUCCESS in case of successful publish
*
*******************************************************************************/
subs_rslt_t publish_device_properties (pub_msg_type_t pub_msg_type, radar_presence_attributes_t device_attributes)
{
    subs_rslt_t rc = SUBS_SUCCESS;
    char *buffer_to_publish = NULL;
    int location_sharing = false;
    int deprovision = false;

    /* Allocate memory for buffer to publish */
    buffer_to_publish = (char *)calloc(DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE, sizeof(char));
    if(NULL == buffer_to_publish)
    {
        APP_LOG_ERROR(( "Failed to allocate memory for buffer_to_publish"));
        return SUBS_ERROR;
    }

    /* Clear buffer */
    buffer_to_publish[0] = '\0';

    /*Initialise buffer_to_publish with no characters*/
    strncpy(buffer_to_publish, "", 1);

    switch (pub_msg_type)
    {
        case PUB_FW_VERSION:
        {
            char* connected_status = "Connected";
            snprintf(buffer_to_publish, DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE, "{\"state\":{\"reported\":{\"get_desired_state\":%d,\"fw_version\":\"%s\",\"ConnectedStatus\":\"%s\"}}}",
                                                                       GET_DESIRED_PROPERTIES_STATE_TRUE, (char*)CE_VERSION, (char*)connected_status);
            APP_LOG_DEBUG(("buffer_to_publish = %s", buffer_to_publish));

            /* Publish the fimrware version to the respective topic */
            rc = publish_to_mqtt_topic(buffer_to_publish, strnlen(buffer_to_publish, DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE)+1, 
                                    (char*)mqtt_topic_publish_device_properties);

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

		case PUB_DEVICE_PROPERTIES_ACK:
		{
			//To-Do: By default micro_if_macro mode is sent as of today,since it is supported to micro_if_macro only, it is hardcoded.
			snprintf(buffer_to_publish, DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE, "{\"state\":{\"reported\":{\"get_desired_state\":%d,\"LocationSharing\":%d,\"Deprovision\":%d,\"max_range\":%.2f,\"macro_threshold\":%.2f,\"micro_threshold\":%.2f,\"mode\":\"micro_if_macro\",\"Sensor_Solution\":\"XENSIV BGT60TR13C Presence Detection\"},\"desired\":null}}",

					GET_DESIRED_PROPERTIES_STATE_FALSE, location_sharing, deprovision, device_attributes.max_range,device_attributes.macro_threshold,device_attributes.micro_threshold);
			APP_LOG_DEBUG(("buffer_to_publish = %s", buffer_to_publish));
			/* Publish to respective topic */
			rc = publish_to_mqtt_topic(buffer_to_publish, strnlen(buffer_to_publish, DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE)+1,
									(char*)mqtt_topic_publish_device_properties);

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

    }

    // Cleanup garbage
    if(NULL != buffer_to_publish)
    {
        free(buffer_to_publish);
        buffer_to_publish = NULL;
    }

    return rc;
}

/*******************************************************************************
* Function Name: parse_device_properties
********************************************************************************
* Summary:
* Callback function for JSON parser for device properties received from cloud
*
* Parameters:
*  json_object: JSON key-value pair containing the device properties 
*
* Return:
*  void
*
*******************************************************************************/

cy_rslt_t parse_device_properties(cy_JSON_object_t *json_object, void* callback_arg)
{
    float max_range;
	float macro_threshold;
	float micro_threshold;
	char mode[MODE_LEN];

    publisher_data_t publisher_q_data;
    if(NULL == json_object)
    {
        APP_LOG_ERROR(("MQTT JSON object is null"));
        return CY_RSLT_TYPE_FATAL;
    }
    /*All the device parameters come with a parent JSON object*/
    if(NULL == json_object->parent_object)
    {
        return CY_RSLT_TYPE_ERROR;
    }

	/* Compare and store the max_range, if the period is out of range set the period to min */
    if(SUBS_SUCCESS == compare_and_store(json_object, &max_range, (char*)RDR_PRESENCE_MAX_RANGE, (char*)PARAMS_PARENT_OBJECT, true))
    {
		if ((max_range > MAX_RANGE_MAX_LIMIT) || (max_range < MAX_RANGE_MIN_LIMIT)) {
			max_range = MAX_RANGE_MIN_LIMIT;
			APP_LOG_ERROR(("max_range parameter out of range"));
		}

		publisher_q_data.cmd = UPDATE_RADAR_MAX_RANGE;
		publisher_q_data.max_range = max_range;

        xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

    }

	/* Compare and store the radar_presence_sensitivity range, if the period is out of range set the period to min */


    if(SUBS_SUCCESS == compare_and_store(json_object, &macro_threshold, (char*)RDR_PRESENCE_MACRO_THRESHOLD, (char*)PARAMS_PARENT_OBJECT, true))
        {
    		if ((macro_threshold > MACRO_THRESHOLD_MAX_LIMIT) || (macro_threshold < MACRO_THRESHOLD_MIN_LIMIT)) {
    			macro_threshold = MACRO_THRESHOLD_MIN_LIMIT;
    			APP_LOG_ERROR(("macro_threshold parameter out of range"));
    		}

    		publisher_q_data.cmd = UPDATE_RADAR_MACRO_THRESHOLD;
    		publisher_q_data.macro_threshold = macro_threshold;

            xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

        }

    if(SUBS_SUCCESS == compare_and_store(json_object, &micro_threshold, (char*)RDR_PRESENCE_MICRO_THRESHOLD, (char*)PARAMS_PARENT_OBJECT, true))
        {
    		if ((micro_threshold > MICRO_THRESHOLD_MAX_LIMIT) || (micro_threshold < MICRO_THRESHOLD_MIN_LIMIT)) {
    			micro_threshold = MICRO_THRESHOLD_MIN_LIMIT;
    			APP_LOG_ERROR(("micro_threshold parameter out of range"));
    		}

    		publisher_q_data.cmd = UPDATE_RADAR_MICRO_THRESHOLD;
    		publisher_q_data.micro_threshold = micro_threshold;

            xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

        }

    if(SUBS_SUCCESS == compare_and_store(json_object, &mode, (char*)RDR_PRESENCE_MODE, (char*)PARAMS_PARENT_OBJECT, true))
		{

    		xensiv_radar_presence_mode_t result = string_to_mode(mode);

			publisher_q_data.cmd = UPDATE_RADAR_MODE;
			publisher_q_data.mode = (char)result;

			xQueueSend(publisher_task_q, &publisher_q_data, portMAX_DELAY);

		}


	return CY_RSLT_SUCCESS;
}




/*******************************************************************************
* Function Name: compare_and_store
********************************************************************************
* Summary:
*  Compare the JSON object key and JSON parent object key with the expected and
*  update the respective device properties.
*
* Parameters:
*  json_object          :   Parsed key-value pair of the JSON payload
*  device_parameter     :   Pointer to the device storage location
*  expected_key         :   Expected key of the json object
*  expected_parent_key  :   Expected key of the parent json object
*  is_float             :   If the value is float type
*
* Return:
* subs_rslt_t
*
*******************************************************************************/
subs_rslt_t compare_and_store(cy_JSON_object_t* json_object, void* device_parameter,
                                 char* expected_key, char* expected_parent_key, bool is_float)
{
    subs_rslt_t rc = SUBS_FAILURE;
    /*
    * Process:  1. Compare object key
    *           2. Compare parent object key
    *           3. Copy value of the JSON object to destination buffer
    */
    char string_to_number_buffer[RX_DP_BUFFER_LENGTH] = {0};
	if(SUBS_SUCCESS == strncmp(json_object->object_string, expected_key, json_object->object_string_length)) {
        if(SUBS_SUCCESS == strncmp(json_object->parent_object->object_string, expected_parent_key,
                                    json_object->parent_object->object_string_length)) {
            /*Success if the object key and parent key match*/
            rc = SUBS_SUCCESS;
            switch (json_object->value_type)
            {
                case JSON_NUMBER_TYPE:
                {
					snprintf((char*)string_to_number_buffer, json_object->value_length+1,
                                            "%s", json_object->value);                  
                    if(is_float)
                    {
                        *((float*)device_parameter) = strtof(string_to_number_buffer, NULL);
						
                    }
                    else
                    {
                        *((uint32_t*)device_parameter) = atoi(string_to_number_buffer);
                    }
                    break;
                }
				case JSON_FLOAT_TYPE:
				{
					snprintf((char*)string_to_number_buffer, json_object->value_length+1,
                                            "%s", json_object->value); 
					*((float*)device_parameter) = strtof(string_to_number_buffer, NULL);
					break;
				}
                case JSON_STRING_TYPE:
				{	
				    snprintf((char*)device_parameter, json_object->value_length+1,
                                        "%s", json_object->value);
				}                
					break;
                default:
					break;
            }
        }
    }
    return rc;
}
