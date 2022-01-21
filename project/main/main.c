/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file main.c
 * @brief implementation of CMS application initialization and main loop.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "sdkconfig.h"
#include "cJSON.h"

#include "FreeRTOS.h"
#include "task.h"

#include "core_mqtt_agent.h"
#include "cms_log.h"

/*-----------------------------------------------------------*/

#define DEMO_CONFIG_FILE_PATH           CONFIG_FS_MOUNT_POINT "/cms_demo_config.json"
#define MAX_RTSP_STREAMING_URL          ( 128U )

/* stack size of app obd telemetry task */
#define democonfigOBD_TELEMETRY_TASK_STACK_SIZE     ( 1024 * 8 )

/*-----------------------------------------------------------*/

static const char *TAG = "main";

/*-----------------------------------------------------------*/

extern void sd_card_init(void);

static char rtspStreamingUrl[ MAX_RTSP_STREAMING_URL ] = CONFIG_RTSP_STREAMING_URL;

extern void vehicleTelemetryReportTask(void);

/*-----------------------------------------------------------*/

static bool prvReadDemoConfig( void )
{
    bool retKvsConfg = true;
    cJSON * pJson = NULL;
    cJSON * pSub = NULL;
    FILE * fp = NULL;
    size_t sz = 0, readSz = 0;
    char * pConfigBuffer = NULL;

    /* Check if file exist. */
    fp = fopen( DEMO_CONFIG_FILE_PATH, "rb" );
    if( fp == NULL )
    {
        CMS_LOGE( TAG, "Open %s failed.", DEMO_CONFIG_FILE_PATH );
        retKvsConfg = false;
    }
    else
    {
        fseek( fp, 0L, SEEK_END );
        sz = ftell( fp );
        fseek( fp, 0L, SEEK_SET );
        CMS_LOGI( TAG, "Open %s size %d.", DEMO_CONFIG_FILE_PATH, sz );
        pConfigBuffer = malloc( sz );
        if( pConfigBuffer == NULL )
        {
            CMS_LOGE( TAG, "Malloc buffer size %d failed.", sz );
            retKvsConfg = false;
        }
        else
        {
            readSz = fread( pConfigBuffer, 1, sz, fp );
            if( readSz != sz )
            {
                CMS_LOGE( TAG, "Read file failed." );
                retKvsConfg = false;
            }
        }
        fclose( fp );
    }

    /* Parse the RTSP server name. */
    if( retKvsConfg == true )
    {
        pJson = cJSON_Parse( pConfigBuffer );
        if( pJson == NULL )
        {
            CMS_LOGE( TAG, "cJSON_Parse failed." );
        }
        else
        {
            pSub = cJSON_GetObjectItem( pJson, "RTSP_STREAMING_URL" );
            if( pSub == NULL )
            {
                CMS_LOGE( TAG, "cJSON_GetObjectItem RTSP_STREAMING_URL failed use config." );
            }
            else
            {
                CMS_LOGI( TAG, "RTSP_STREAMING_URL : %s.", pSub->valuestring );
                strncpy( rtspStreamingUrl, pSub->valuestring, MAX_RTSP_STREAMING_URL );
            }
        }
    }

    if( pJson != NULL )
    {
        cJSON_Delete( pJson );
    }
    if( pConfigBuffer != NULL )
    {
        free( pConfigBuffer );
    }

    return retKvsConfg;
}

/*-----------------------------------------------------------*/

void app_main()
{
    CMS_LOGI( TAG, "[APP] Startup.." );
    CMS_LOGI( TAG, "[APP] Free memory: %d bytes.", esp_get_free_heap_size() );
    CMS_LOGI( TAG, "[APP] IDF version: %s.", esp_get_idf_version() );

    // Initialize network
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Read configurations from SD card
    #ifdef CONFIG_FILE_SYSTEM_ENABLE
        sd_card_init();
        prvReadDemoConfig();
    #endif
    
    // Create vehicle telemetry report app
    xTaskCreate( (TaskFunction_t) vehicleTelemetryReportTask,
                 "vehicleTelemetryReportTask",
                 democonfigOBD_TELEMETRY_TASK_STACK_SIZE,
                 NULL,
                 tskIDLE_PRIORITY+1,
                 NULL
                );
}

/*-----------------------------------------------------------*/
