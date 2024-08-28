/******************************************************************************
* File Name:   mqtt_task.h
*
* Description: This file is the public interface of mqtt_task.c
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

#ifndef MQTT_TASK_H_
#define MQTT_TASK_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "cy_mqtt_api.h"
#include "common_variables.h"
#include "cy_json_parser.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* Task parameters for MQTT Client Task. */
#define MQTT_CLIENT_TASK_PRIORITY                   (2)
#define MQTT_CLIENT_TASK_STACK_SIZE                 (1024 * 2)

/* Maximum Length of Publish and subscriber topic names */
#define MQTT_TOPIC_PUBLISH_TELEMETRY_LEN            (128)
#define MQTT_TOPIC_PUBLISH_DEVICE_PROPERTIES_LEN    (64)
#define MQTT_TOPIC_SUBSCRIBE_LEN                    (64)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Commands for the MQTT Client Task. */
typedef enum
{
    HANDLE_DISCONNECTION
} mqtt_task_cmd_t;

/*******************************************************************************
 * Extern variables
 ******************************************************************************/
extern cy_mqtt_t mqtt_connection;
extern QueueHandle_t mqtt_task_q;
extern uint8_t mqtt_topic_publish_telemetry [MQTT_TOPIC_PUBLISH_TELEMETRY_LEN];
extern uint8_t mqtt_topic_publish_device_properties [MQTT_TOPIC_PUBLISH_DEVICE_PROPERTIES_LEN];
extern uint8_t mqtt_topic_subscribe_device_properties [MQTT_TOPIC_SUBSCRIBE_LEN];

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void mqtt_client_task(void *pvParameters);

#endif /* MQTT_TASK_H_ */

/* [] END OF FILE */
