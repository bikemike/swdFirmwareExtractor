/*
 * Copyright (C) 2017 Obermaier Johannes
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#ifndef INC_UART_H
#define INC_UART_H
#include "st/stm32f0xx.h"

typedef struct {
	uint32_t transmitHex;
	uint32_t transmitLittleEndian;
	uint32_t readoutAddress;
	uint32_t readoutLen;
	uint32_t active;
} uartControl_t;

void uartInit( void );
void uartReceiveCommands( uartControl_t * const ctrl );
void uartSendWordBin( uint32_t const val, uartControl_t const * const ctrl );
void uartSendWordHex( uint32_t const val, uartControl_t const * const ctrl );
void uartSendWordBinLE( uint32_t const val );
void uartSendWordBinBE( uint32_t const val );
void uartSendWordHexLE( uint32_t const val );
void uartSendWordHexBE( uint32_t const val );
void uartSendByteHex( uint8_t const val );
void uartSendStr( const char * const str );


#endif
