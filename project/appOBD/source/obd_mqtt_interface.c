#include <stdint.h>

#include "FreeRTOS.h"

#ifdef OBD_USE_CORE_MQTT

#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "core_mqtt_agent_tasks.h"

BaseType_t OBD_MqttPublish( uint8_t mqttQos, char * topciBuf, uint32_t topicLen, char * messageBuf, uint32_t messageLen )
{
    BaseType_t retMqtt = pdPASS;
    retMqtt = mqttAgentPublish( ( MQTTQoS_t )mqttQos,
                                topciBuf,
                                topicLen,
                                messageBuf,
                                messageLen );
    return retMqtt;
}

#else

#include <string.h>

#include "explink_library.h"

BaseType_t OBD_MqttPublish( uint8_t mqttQos, char * topicBuf, uint32_t topicLen,
                            char * messageBuf, uint32_t messageLen )
{
    BaseType_t retMqtt = pdPASS;
    char localBuffer[ 128 ] = { 0 };
    int retRead = 0;
    uint32_t sendIndex = 0;
    uint32_t i = 0;
    ExplinkResponseCode_t errCode = 0;
    
    /* Fix escaping. */
    sendIndex = snprintf( localBuffer, 128, "AT+CONF Topic1=" );
    for( i = 0; i < topicLen; i++ )
    {
        if( topicBuf[ i ] == '\\' )
        {
            localBuffer[ sendIndex++ ] = '\\';
            localBuffer[ sendIndex++ ] = '\\';
        }
        else
        {
            localBuffer[ sendIndex++ ] = topicBuf[ i ];
        }
    }
    
    /* Setup the topic. */
    Explink_SendCommand( localBuffer, 0 );
    
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

    retRead = Explink_ReadLine( localBuffer, 128 );
    errCode = Explink_CheckResponse( localBuffer, retRead );
    printf( "%d <=== %s\r\n", retRead, localBuffer );
    printf( "Error code %d\r\n", errCode );
    
    return retMqtt;
}

#endif

