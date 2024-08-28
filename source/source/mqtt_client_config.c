/******************************************************************************
* File Name:   mqtt_client_config.c
*
* Description: This file contains the configuration structures used by 
*              the MQTT task for MQTT connect operation.
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

/* Header file includes */
#include <stdio.h>
#include "mqtt_client_config.h"
#include "cy_mqtt_api.h"

/******************************************************************************
* Global Variables
*******************************************************************************/
/* MQTT Broker/Server details */
cy_mqtt_broker_info_t broker_info =
{
    .hostname = MQTT_BROKER_ADDRESS,
    .hostname_len = sizeof(MQTT_BROKER_ADDRESS) - 1,
    .port = MQTT_PORT
};

/* MQTT client credentials to be used for a secure connection. */
static cy_awsport_ssl_credentials_t credentials =
{
    /* Configure the client certificate. */
#ifdef CLIENT_CERTIFICATE
    .client_cert = (const char *)CLIENT_CERTIFICATE,
    .client_cert_size = sizeof(CLIENT_CERTIFICATE),
#else
    .client_cert = NULL,
    .client_cert_size = 0,
#endif

    /* Configure the client private key. */
#ifdef CLIENT_PRIVATE_KEY
    .private_key = (const char *)CLIENT_PRIVATE_KEY,
    .private_key_size = sizeof(CLIENT_PRIVATE_KEY),
#else
    .private_key = NULL,
    .private_key_size = 0,
#endif

    /* Configure the Root CA certificate of the MQTT Broker/Server. */
#ifdef ROOT_CA_CERTIFICATE
    .root_ca = (const char *)ROOT_CA_CERTIFICATE,
    .root_ca_size = sizeof(ROOT_CA_CERTIFICATE),
#else
    .root_ca = NULL,
    .root_ca_size = 0,
#endif
};

/* Pointer to the security details of the MQTT connection. */
cy_awsport_ssl_credentials_t *security_info = &credentials;

/* Last Will and Testament (LWT) message structure. The MQTT broker will
 * publish the LWT message if this client disconnects unexpectedly. 
 * The topic name and payload are replaced after the mqtt topics are constructed in mqtt_connect function.
 */
static cy_mqtt_publish_info_t will_msg_info =
{
    .qos = MQTT_MESSAGES_QOS,
    .topic = NULL,
    .topic_len = 0,
    .payload = NULL,
    .payload_len = 0,
    .retain = false,
    .dup = false
};

/* MQTT connection information structure 
*  This structure is filled up with relevant information in mqtt_connect function
*/
cy_mqtt_connect_info_t connection_info =
{
    .client_id = NULL,
    .client_id_len = 0,
    .username = NULL,
    .username_len = 0,
    .password = NULL,
    .password_len = 0,
    .clean_session = true,
    .keep_alive_sec = MQTT_KEEP_ALIVE_SECONDS,
    .will_info = &will_msg_info
};

/* Check for a valid QoS setting - QoS 0, QoS 1, or QoS 2. */
#if ((MQTT_MESSAGES_QOS != 0) && (MQTT_MESSAGES_QOS != 1) && (MQTT_MESSAGES_QOS != 2))
    #error "Invalid QoS setting! MQTT_MESSAGES_QOS must be either 0 or 1."
#endif

/* [] END OF FILE */