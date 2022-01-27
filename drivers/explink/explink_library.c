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
#include <string.h>

#include "FreeRTOS_DriverInterface.h"
#include "explink_library.h"

static Peripheral_Descriptor_t sExplinkDesc = NULL;

ExplinkResponseCode_t Explink_CheckResponse( char * respBuffer, uint32_t respBufferLen )
{
    ExplinkResponseCode_t errCode = EL_OK;

    /* Check if this is OK. */
    if( ( strstr( respBuffer, "OK\n" ) != NULL ) || ( strstr( respBuffer, "OK " ) != NULL )  )
    {
        printf( "Command Okay\r\n" );
        errCode = EL_OK;
    }
    else if( strstr( respBuffer, "ERR" ) != NULL )
    {
        printf( "Command Error %s\r\n", respBuffer );
        errCode =  (ExplinkResponseCode_t) atoi( &respBuffer[3] );
    }
    else
    {
        /* Unknown error code. */
        printf( "Unknown response string %s\r\n", respBuffer );
        errCode = EL_UNKNOWN_RESPONSE;
    }
    
    return errCode;
}

int Explink_Init( void )
{
    int retValue = 0;
    
    sExplinkDesc = FreeRTOS_open( ( int8_t * )( "/dev/explink" ), 0 );

    return retValue;
}

int Explink_SendBuffer( char * sendBuffer, uint32_t sendBufferLen )
{
    int retSendLine = 0;
    
    if( sExplinkDesc != NULL )
    {
        retSendLine = FreeRTOS_write( sExplinkDesc, sendBuffer, sendBufferLen );
    }

    return retSendLine;
}


int Explink_SendLine( char * sendBuffer, uint32_t sendBufferLen )
{
    int retSendLine = 0;
    uint32_t cmdLineLength = sendBufferLen;
    
    if( cmdLineLength == 0 )
    {
        cmdLineLength = strlen( sendBuffer );
    }

    if( sExplinkDesc != NULL )
    {
        retSendLine = FreeRTOS_write( sExplinkDesc, sendBuffer, cmdLineLength );
        if( sendBuffer[ cmdLineLength - 1 ] != '\n' )
        {
            retSendLine = retSendLine + FreeRTOS_write( sExplinkDesc, "\n", 1 );
        }
    }
    return retSendLine;
}

int Explink_ReadLine( char * readBuffer, uint32_t readBufferLen )
{
    uint32_t bufferIndex = 0U;
    if( sExplinkDesc != NULL )
    {
        bufferIndex = FreeRTOS_read( sExplinkDesc, readBuffer, readBufferLen );
    }
    return bufferIndex;
}

ExplinkResponseCode_t Explink_SendCommand( char * cmdBuffer, uint32_t cmdBufferLen )
{
    int retSend = 0;
    int retRead = 0;
    char respBuffer[ 128 ] = { 0 };
    int sendLineLength = cmdBufferLen;
    ExplinkResponseCode_t errCode = 0;

    retSend = Explink_SendLine( cmdBuffer, sendLineLength );

    retRead = Explink_ReadLine( respBuffer, 128 );
    errCode = Explink_CheckResponse( respBuffer, retRead );
    printf( "%d ===> %s\r\n", retSend, cmdBuffer );
    printf( "%d <=== %s\r\n", retRead, respBuffer );
    printf( "Error code %d\r\n", errCode );
    return errCode;
}
