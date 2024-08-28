/******************************************************************************
* File Name:   common_variables.h
*
* Description:  This file contains common macros and variables used by all the source files
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

#ifndef INCLUDED_COMMON_VARIABLES_H
#define INCLUDED_COMMON_VARIABLES_H

/* Uncomment the macro definitions for enabling respective logs */
#define APP_LOG_INFO(message)               printf("\n");printf message;printf("\n")
#define APP_LOG_DEBUG(message)              //printf("\n");printf message;printf("\n")
#define APP_LOG_ERROR(message)              printf("\n");printf message;printf("\n")

/* Firmware version number. This will be published to the cloud */
#define CE_VERSION                    "1.0.0"

/* Function return type */
typedef enum subs_rslt {
    SUBS_SUCCESS    =   0,
    SUBS_FAILURE    =   1,
    SUBS_ERROR      =   2
}subs_rslt_t;

#endif //INCLUDED_COMMON_VARIABLES_H
