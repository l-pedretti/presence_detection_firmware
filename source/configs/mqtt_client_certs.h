/******************************************************************************
* File Name:   mqtt_client_certs.h
*
* Description: This file contains the certificates used for the subsystems sensor cloud connection.
*
* Related Document: See README.md
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


#ifndef MQTT_CLIENT_CERTS_H_
#define MQTT_CLIENT_CERTS_H_

/* Configure the below credentials in case of a secure MQTT connection. */
/* PEM-encoded client certificate */
#define CLOUD_CERT_KEY_LEN              (2048u)
#define AWS_CLOUD_KEY                   (0x14004000)
#define AWS_CLOUD_CERTFICATE            (AWS_CLOUD_KEY + CLOUD_CERT_KEY_LEN)
#define AWS_CLOUD_ROOT_CERTFICATE       (AWS_CLOUD_CERTFICATE + CLOUD_CERT_KEY_LEN)

#define CLIENT_CERTIFICATE      AWS_CLOUD_CERTFICATE
/* PEM-encoded client private key */
#define CLIENT_PRIVATE_KEY      AWS_CLOUD_KEY
/* PEM-encoded Root CA certificate */
#define ROOT_CA_CERTIFICATE    AWS_CLOUD_ROOT_CERTFICATE

#endif /* MQTT_CLIENT_CERTS_H_ */
