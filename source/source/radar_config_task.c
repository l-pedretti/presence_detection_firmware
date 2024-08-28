/******************************************************************************
 * File Name:   radar_config_task.c
 *
 * Description: This file contains the task that handles parsing the new
 *              configuration coming from remote server and setting it to the
 *              xensiv-radar-sensing library.
 *
 * Related Document: See README.md
 *
 * ===========================================================================
 * Copyright (C) 2021 Infineon Technologies AG. All rights reserved.
 * ===========================================================================
 *
 * ===========================================================================
 * Infineon Technologies AG (INFINEON) is supplying this file for use
 * exclusively with Infineon's sensor products. This file can be freely
 * distributed within development tools and software supporting such
 * products.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON
 * WHATSOEVER.
 * ===========================================================================
 */

/* Header file from system */
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"

/* Header file from library */
#include "cy_json_parser.h"

/* Header file for local tasks */
#include "publisher_task.h"
#include "radar_config_task.h"
#include "radar_task.h"
#include "subscriber_task.h"

#define RADAR_CONFIG_TASK_QUEUE_LENGTH                     (10u)

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
TaskHandle_t radar_config_task_handle = NULL;
QueueHandle_t radar_config_task_q;

xensiv_radar_presence_config_t config;



/*******************************************************************************
 * Function Name: radar_config_task
 *******************************************************************************
 * Summary:
 *      Parse incoming json string, and set new configuration to
 *      xensiv-radar-sensing library.
 *
 * Parameters:
 *   pvParameters: thread
 *
 * Return:
 *   none
 ******************************************************************************/



void radar_config_task(void *pvParameters)
{

    /* To avoid compiler warnings */
	radar_config_data_t radarData;
	cy_rslt_t result;
	float32_t bin_len = 0.0f;

	radar_config_task_q = xQueueCreate(RADAR_CONFIG_TASK_QUEUE_LENGTH, sizeof(radar_config_data_t));
	xensiv_radar_presence_handle_t handle = (xensiv_radar_presence_handle_t)pvParameters;
    while (true)
    {
		//printf("Running radar config task \r\n");

		if (pdTRUE == xQueueReceive(radar_config_task_q, &radarData, portMAX_DELAY))
		{
			switch(radarData.cmd)
            {

				 case UPDATE_RADAR_PRESENCE_MAX_RANGE_CONFIG:
				 {
					 result = xensiv_radar_presence_get_config(handle, &config);

					 if (result != XENSIV_RADAR_PRESENCE_OK)
					     {
					         APP_LOG_ERROR(("Error while reading presence config"));
					     }
					 	 bin_len = xensiv_radar_presence_get_bin_length(handle);
					 	 config.max_range_bin = (int32_t)(radarData.max_range/bin_len);

					 	if (xSemaphoreTake(sem_radar_sensing_context, portMAX_DELAY) == pdTRUE)
					 	{
						 result = xensiv_radar_presence_set_config(handle, &config);
						 xensiv_radar_presence_reset(handle);

						 xSemaphoreGive(sem_radar_sensing_context);
					 	}

					 if (result != XENSIV_RADAR_PRESENCE_OK)
						 {
							 APP_LOG_ERROR(("Error while setting new config"));
						 }
						 else
						 {
							 config.max_range_bin = 0;
							 xensiv_radar_presence_get_config(handle, &config);
							 APP_LOG_DEBUG(("Radar Presence max_range = %ld",config.max_range_bin));
						 }

				 	break;
				 }

				 case UPDATE_RADAR_MACRO_THRESHOLD_CONFIG:
				 {
						 result = xensiv_radar_presence_get_config(handle, &config);

						 if (result != XENSIV_RADAR_PRESENCE_OK)
							 {
							 APP_LOG_ERROR(("Error while reading presence config"));
							 }

						 config.macro_threshold = radarData.macro_threshold;
						 if (xSemaphoreTake(sem_radar_sensing_context, portMAX_DELAY) == pdTRUE)
						 {
						 result = xensiv_radar_presence_set_config(handle, &config);
						 xensiv_radar_presence_reset(handle);

						 xSemaphoreGive(sem_radar_sensing_context);
						 }

						 if (result != XENSIV_RADAR_PRESENCE_OK)
							 {
								 APP_LOG_ERROR(("Error while setting new config"));
							 }
							 else
							 {
								 config.macro_threshold = 0;
								 xensiv_radar_presence_get_config(handle, &config);
								 APP_LOG_DEBUG(("Radar Presence macro_threshold = %f",config.macro_threshold));
							 }


				 	break;
				 }


				 case UPDATE_RADAR_MICRO_THRESHOLD_CONFIG:
				 {
						 result = xensiv_radar_presence_get_config(handle, &config);

						 if (result != XENSIV_RADAR_PRESENCE_OK)
							 {
							 	 APP_LOG_ERROR(("Error while reading presence config"));
							 }

						 config.micro_threshold = radarData.micro_threshold;
						 if (xSemaphoreTake(sem_radar_sensing_context, portMAX_DELAY) == pdTRUE)
						 {
						 result = xensiv_radar_presence_set_config(handle, &config);
						 xensiv_radar_presence_reset(handle);

						 xSemaphoreGive(sem_radar_sensing_context);
						 }

						 if (result != XENSIV_RADAR_PRESENCE_OK)
							 {
								 APP_LOG_ERROR(("Error while setting new config"));
							 }
							 else
							 {
								 config.micro_threshold = 0;
								 xensiv_radar_presence_get_config(handle, &config);
								 APP_LOG_DEBUG(("Radar Presence micro_threshold = %f",config.micro_threshold));
							 }


					break;
				 }

				 case UPDATE_RADAR_MODE_CONFIG:
				 {
						 result = xensiv_radar_presence_get_config(handle, &config);

						 if (result != XENSIV_RADAR_PRESENCE_OK)
							 {
								 APP_LOG_ERROR(("Error while reading presence config"));
							 }

						 config.mode = (xensiv_radar_presence_mode_t)radarData.mode;
						 if (xSemaphoreTake(sem_radar_sensing_context, portMAX_DELAY) == pdTRUE)
						 {
						 result = xensiv_radar_presence_set_config(handle, &config);
						 xensiv_radar_presence_reset(handle);

						 xSemaphoreGive(sem_radar_sensing_context);
						 }

						 if (result != XENSIV_RADAR_PRESENCE_OK)
							 {
								 APP_LOG_ERROR(("Error while setting new config"));
							 }
							 else
							 {
								 config.mode = 0;
								 xensiv_radar_presence_get_config(handle, &config);
								 APP_LOG_DEBUG(("Radar Presence mode = %d",config.mode));
							 }


					break;
				 }



				default:
					break;

			}
		}
    }
}



/* [] END OF FILE */
