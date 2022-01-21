#include <stdint.h>
#include "FreeRTOS.h"

#include "FreeRTOS.h"
#include "task.h"

#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "core_mqtt_agent_tasks.h"

#include "obd_mqtt_interface.h"
#include "obd_config.h"

#include "esp_sntp.h"
#include "protocol_examples_common.h"
#include "cms_log.h"

/*-----------------------------------------------------------*/

#define democonfigDOWNLOAD_AGENT_TASK_STACK_SIZE    ( 1024 * 8 )

/* max retry count of syncing up with ntp server*/
#define OBD_OBTAIN_TIME_RETRY_COUNT     ( 20U )

/* ntp server sync interval*/
#define OBD_OBTAIN_TIME_LOOP_DELAY_MS   ( 2000U )

/*-----------------------------------------------------------*/

extern MQTTAgentContext_t xGlobalMqttAgentContext;

static const char *TAG = "main";

/*-----------------------------------------------------------*/

extern void startMqttAgentTask( void );

/*-----------------------------------------------------------*/

static void syncUpDateTime(void)
{
    CMS_LOGI( TAG, "Initializing SNTP." );
    sntp_setoperatingmode( SNTP_OPMODE_POLL );
    sntp_setservername( 0, "pool.ntp.org" );
    sntp_set_time_sync_notification_cb( NULL );
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode( SNTP_SYNC_MODE_SMOOTH );
#endif
    sntp_init();

    // wait for time to be set
    int retry = 0;
    while ( sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry <= OBD_OBTAIN_TIME_RETRY_COUNT )
    {
        CMS_LOGI( TAG, "Waiting for system time to be set... (%d/%u).", retry, OBD_OBTAIN_TIME_RETRY_COUNT );
        vTaskDelay( pdMS_TO_TICKS( OBD_OBTAIN_TIME_LOOP_DELAY_MS ) );
    }

    if( retry < OBD_OBTAIN_TIME_RETRY_COUNT )
    {
        /* Set timezone */
        unsetenv( "TZ" );
        tzset();
    }
    else
    {
        CMS_LOGE( TAG, "Failed to sync up date and time." ); 
    }

}

/*-----------------------------------------------------------*/

int OBD_MqttInit( void )
{
    // Establish network connection (either wifi or ethernet)
    ESP_ERROR_CHECK(example_connect());

    // Sync up data and time
    syncUpDateTime();

    // Start MQTT Agent
    startMqttAgentTask();

    // Wait until mqtt is connected
    while( MQTTNotConnected == xGlobalMqttAgentContext.mqttContext.connectStatus )
    {
        CMS_LOGI( TAG, "Waiting for MQTT to connect." );
        vTaskDelay( pdMS_TO_TICKS(1000) );
    }
    return 0;
}

BaseType_t OBD_MqttPublish( uint8_t mqttQos, char * topciBuf, uint32_t topicLen, char * messageBuf, uint32_t messageLen )
{
    char tempBuffer[ 128 ];
    uint32_t fixTopicLen = 0U;
    BaseType_t retMqtt = pdPASS;
    
    fixTopicLen = snprintf( tempBuffer, 128, OBD_ROOT_TOPIC"/%s", topciBuf );
    
    retMqtt = mqttAgentPublish( ( MQTTQoS_t )mqttQos,
                                tempBuffer,
                                fixTopicLen,
                                messageBuf,
                                messageLen );
    return retMqtt;
}
