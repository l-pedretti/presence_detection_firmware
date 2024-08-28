/*******************************************************************************
* File Name: device_porperties.h
*
* Description: This file contains functions for constructing and publishing messages in the JSON format accepted by the Sensor Cloud
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

#ifndef DEVICE_PROPERTIES_H
#define DEVICE_PROPERTIES_H

#include "common_variables.h"
#include "mqtt_task.h"
#include "cy_mqtt_api.h"
#include "publisher_task.h"

#define RADAR_BOARD                            		(0)
#define RADAR_SENSOR                           		(0)
#define DEVICE_PROPERTIES_MQTT_MESSAGE_SIZE         (3072)

typedef enum {
    PUB_FW_VERSION = 0,
    PUB_DEVICE_PROPERTIES_ACK
} pub_msg_type_t;

/*******************************************************************************
 * Extern variables
 ******************************************************************************/

extern cy_rslt_t parse_device_properties(cy_JSON_object_t *json_object, void* callback_arg);
extern subs_rslt_t publish_device_properties (pub_msg_type_t pub_msg_type, radar_presence_attributes_t device_attributes);
#endif
