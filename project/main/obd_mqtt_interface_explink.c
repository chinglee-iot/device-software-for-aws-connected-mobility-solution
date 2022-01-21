#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

#include "obd_mqtt_interface.h"
#include "obd_config.h"

#include <string.h>
#include "explink_library.h"

#define TEMP_BUFFER_SIZE    ( 128U )

int OBD_MqttInit( void )
{
    ExplinkResponseCode_t errCode = EL_OK;
    char tempBuffer[ TEMP_BUFFER_SIZE ];
    
    /* Initialize Expresslink commnunication interface. */
    Explink_Init();
    
    /* Wait device ready. */
    while( ( errCode = Explink_SendCommand( "AT", 0 ) ) != EL_OK )
    {
        printf( "Failed to receive AT respone from expresslink %d, retry after delay...", errCode );
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }

    /* Configure the WIFI credential. */
    snprintf( tempBuffer, TEMP_BUFFER_SIZE, "AT+CONF SSID=%s", CONFIG_EXAMPLE_WIFI_SSID );
    errCode = Explink_SendCommand( tempBuffer, 0 );

    snprintf( tempBuffer, TEMP_BUFFER_SIZE, "AT+CONF Passphrase=%s", CONFIG_EXAMPLE_WIFI_PASSWORD );
    errCode = Explink_SendCommand( tempBuffer, 0 );

    snprintf( tempBuffer, TEMP_BUFFER_SIZE, "AT+CONF TopicRoot=%s", OBD_ROOT_TOPIC );
    errCode = Explink_SendCommand( tempBuffer, 0 );

    snprintf( tempBuffer, TEMP_BUFFER_SIZE, "AT+CONF Endpoint=%s", CONFIG_MQTT_BROKER_ENDPOINT );
    errCode = Explink_SendCommand( tempBuffer, 0 );

    /* Connect to IoT core. */
    while( ( errCode = Explink_SendCommand( "AT+CONNECT", 0 ) ) != EL_OK )
    {
        printf( "Failed to connect expresslink %d, retry after delay...", errCode );
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
    return 0;
}

BaseType_t OBD_MqttPublish( uint8_t mqttQos, char * topicBuf, uint32_t topicLen,
                            char * messageBuf, uint32_t messageLen )
{
    BaseType_t retMqtt = pdPASS;
    char tempBuffer[ TEMP_BUFFER_SIZE ] = { 0 };
    int retRead = 0;
    uint32_t sendIndex = 0;
    uint32_t i = 0;
    ExplinkResponseCode_t errCode = 0;
    
    Explink_SendCommand( "AT+CONNECT", 0 );
    
    /* Fix escaping. */
    sendIndex = snprintf( tempBuffer, TEMP_BUFFER_SIZE, "AT+CONF Topic1=" );
    for( i = 0; i < topicLen; i++ )
    {
        if( topicBuf[ i ] == '\\' )
        {
            tempBuffer[ sendIndex++ ] = '\\';
            tempBuffer[ sendIndex++ ] = '\\';
        }
        else
        {
            tempBuffer[ sendIndex++ ] = topicBuf[ i ];
        }
    }
    
    /* Setup the topic. */
    Explink_SendCommand( tempBuffer, 0 );
    
    /* Fix escaping. */
    for( i = 0; i < messageLen; i++ )
    {
        if( messageBuf[ i ] == '\r' )
        {
            messageBuf[ i ] = ' ';
        }
        if( messageBuf[ i ] == '\n' )
        {
            messageBuf[ i ] = ' ';
        }
        else if( messageBuf[ i ] == '\\' )
        {
            messageBuf[ i ]  = ' ';
        }
    }

    /* Send the message. */
    printf( "%d ===> AT+SEND1 %s\r\n", messageLen, messageBuf );
    Explink_SendBuffer( "AT+SEND1 ", strlen( "AT+SEND1 " ) );
    Explink_SendLine( messageBuf, messageLen );

    retRead = Explink_ReadLine( tempBuffer, TEMP_BUFFER_SIZE );
    errCode = Explink_CheckResponse( tempBuffer, retRead );
    printf( "%d <=== %s\r\n", retRead, tempBuffer );
    printf( "Error code %d\r\n", errCode );
    
    return retMqtt;
}
