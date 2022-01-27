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
 * @file obd_device.h
 * @brief Implementation of functions to access obd device io.
 */

#include <string.h>

#include "FreeRTOS_DriverInterface.h"
#include "driver/uart.h"
#include "driver/gpio.h"

/*-----------------------------------------------------------*/

#define ECHO_TEST_TXD ( GPIO_NUM_26 )
#define ECHO_TEST_RXD ( GPIO_NUM_34 )
#define ECHO_TEST_RTS ( UART_PIN_NO_CHANGE )
#define ECHO_TEST_CTS ( UART_PIN_NO_CHANGE )

#define ECHO_UART_PORT_NUM      ( UART_NUM_1 )
#define ECHO_UART_BAUD_RATE     ( 115200 )

#define BUF_SIZE ( 2048 )

#define DEFAULT_READ_TIMEOUT_MS     ( 1000 )
#define DEFAULT_READ_RETRY_COUNT    ( 10 )
#define DEFAULT_POWER_ON_DELAY_MS   ( 3000 )

typedef struct explinkDeviceContext
{
    uint32_t dummy;
} explinkDeviceContext_t;

/*-----------------------------------------------------------*/

static Peripheral_Descriptor_t Explink_Open( const int8_t * pcPath,
                                            const uint32_t ulFlags );

static size_t Explink_Write( Peripheral_Descriptor_t const pxPeripheral,
                         const void * pvBuffer,
                         const size_t xBytes );

static size_t Explink_Read( Peripheral_Descriptor_t const pxPeripheral,
                        void * const pvBuffer,
                        const size_t xBytes );

static explinkDeviceContext_t explinkDeviceContext = { 0 };

Peripheral_device_t gExplinkDevice =
{
    "/dev/explink",
    Explink_Open,
    Explink_Write,
    Explink_Read,
    NULL,
    &explinkDeviceContext
};

/*-----------------------------------------------------------*/

static Peripheral_Descriptor_t Explink_Open( const int8_t * pcPath,
                                      const uint32_t ulFlags )
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    Peripheral_Descriptor_t retDesc = NULL;

    /* Parameters are not used. */
    ( void ) pcPath;
    ( void ) ulFlags;

    // Setup 5V output to driver ESP32
    gpio_config_t io_conf;
    // Disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    // Set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // Bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = ( 1ULL<< 12 );
    // Disable pull-down mode
    io_conf.pull_down_en = 0;
    // Disable pull-up mode
    io_conf.pull_up_en = 0;
    // Configure GPIO with the given settings
    gpio_config( &io_conf );
    
    gpio_set_direction( GPIO_NUM_12, GPIO_MODE_OUTPUT );
    gpio_set_level( GPIO_NUM_12, 1 );

    /* Proper delay after power on. */
    vTaskDelay( pdMS_TO_TICKS( DEFAULT_POWER_ON_DELAY_MS ) );

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    /* Configure UART parameters. */
    uart_param_config(ECHO_UART_PORT_NUM, &uart_config);

    /* Set UART pins. */
    uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);

    /* Install UART driver. */
    if( uart_driver_install( ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags ) == ESP_OK )
    {
        retDesc = ( Peripheral_Descriptor_t ) &gExplinkDevice;
    }
    else
    {
        printf( "Open Explink device fail\r\n" );
        retDesc = NULL;
    }

    return retDesc;
}

/*-----------------------------------------------------------*/

static size_t Explink_Write( Peripheral_Descriptor_t const pxPeripheral,
                  const void * pvBuffer,
                  const size_t xBytes )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    size_t retSize = 0;

    if( pDevice == NULL )
    {
        printf( "Obd_Write bad param pxPeripheral" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else if( pDevice->pDeviceData == NULL )
    {
        printf( "Obd_Write bad param pDeviceData" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else
    {
        retSize = uart_write_bytes( ECHO_UART_PORT_NUM, pvBuffer, xBytes );
    }

    return retSize;
}

/*-----------------------------------------------------------*/

static size_t Explink_Read( Peripheral_Descriptor_t const pxPeripheral,
                 void * const pvBuffer,
                 const size_t xBytes )
{
    Peripheral_device_t * pDevice = ( Peripheral_device_t * ) pxPeripheral;
    size_t retSize = 0;
    char * const readBuffer = ( char * const ) pvBuffer;
    uint32_t readBufferLen = xBytes;
    uint32_t bufferIndex = 0U;
    uint32_t retryCount = 0U;
    int len = 0;

    if( pDevice == NULL )
    {
        printf( "Obd_Read bad param pxPeripheral" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else if( pDevice->pDeviceData == NULL )
    {
        printf( "Obd_Read bad param pDeviceData" );
        retSize = FREERTOS_IO_ERROR_BAD_PARAM;
    }
    else
    {
        for( bufferIndex = 0U; bufferIndex < readBufferLen; bufferIndex++ )
        {
            /* The retry is only required for the first byte. */
            if( bufferIndex == 0 )
            {
                for( retryCount = 0; retryCount < DEFAULT_READ_RETRY_COUNT; retryCount++ )
                {
                    len = uart_read_bytes( ECHO_UART_PORT_NUM, 
                                           &readBuffer[bufferIndex],
                                           1,
                                           pdMS_TO_TICKS( DEFAULT_READ_TIMEOUT_MS ) );
                    if( len > 0 ) break;
                }
            }
            else
            {
                len = uart_read_bytes( ECHO_UART_PORT_NUM, 
                       &readBuffer[bufferIndex],
                       1,
                       pdMS_TO_TICKS( DEFAULT_READ_TIMEOUT_MS ) );
            }

            if( ( len > 0 ) && ( readBuffer[ bufferIndex ] == '\n' ) )
            {
                if( ( xBytes - 1 ) > bufferIndex )
                {
                    readBuffer[ bufferIndex + 1 ] = '\0';
                }
                printf( "Received %s\r\n", readBuffer );
                break;
            }
            else if( len <= 0 )
            {
                break;
            }

        }
        retSize = bufferIndex;
    }

    return retSize;
}

/*-----------------------------------------------------------*/
