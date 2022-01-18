#ifndef _EXPLINK_LIBRARY_H_
#define _EXPLINK_LIBRARY_H_

typedef enum ExplinkResponseCode
{
    EL_OK,
    EL_OVERFLOW,
    EL_PARSE_ERROR,
    EL_COMMAND_NOT_FOUND,
    EL_PARAMETER_ERROR,
    EL_INVALID_ESCAPE,
    EL_NO_CONNECTION,
    EL_TOPIC_OUT_OF_RANGE,
    EL_TOPIC_UNDEFINED,
    EL_INVALID_KEY_LENGTH,
    EL_INVALID_KEY_NAME,
    EL_UNKNOWN_KEY,
    EL_KEY_READONLY,
    EL_KEY_WRITEONLY,
    EL_UNABLE_TO_CONNECT,
    EL_TIME_NOT_AVAILABLE,
    EL_LOCATION_NOT_AVAILABLE,
    EL_MODE_NOT_AVAILABLE,
    EL_ACTIVE_CONNECTION,
    EL_HOST_IMAGE_NOT_AVAILABLE,
    EL_INVALID_ADDRESS,
    EL_INVALID_OTA_UPDATE,
    EL_INVALID_QUERY,
    EL_INVALID_SIGNATURE,
    EL_UNKNOWN_RESPONSE
} ExplinkResponseCode_t;

int Explink_Init( void );
int Explink_SendBuffer( char * sendBuffer, uint32_t sendBufferLen );
int Explink_SendLine( char * sendBuffer, uint32_t sendBufferLen );
int Explink_ReadLine( char * readBuffer, uint32_t readBufferLen );

ExplinkResponseCode_t Explink_SendCommand( char * cmdBuffer, uint32_t cmdBufferLen );
ExplinkResponseCode_t Explink_CheckResponse( char * respBuffer, uint32_t respBufferLen );

#endif /* _EXPLINK_LIBRARY_H_ */
