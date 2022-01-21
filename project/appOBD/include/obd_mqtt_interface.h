#ifndef _OBD_MQTT_INTERFACE_H__
#define _OBD_MQTT_INTERFACE_H__

int OBD_MqttInit( void );

BaseType_t OBD_MqttPublish( uint8_t mqttQos, char * topciBuf, uint32_t topicLen,
    char * messageBuf, uint32_t messageLen );

#endif
