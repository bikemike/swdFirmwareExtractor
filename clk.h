/*
 * Copyright (C) 2017 Obermaier Johannes
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#ifndef INC_CLK_H
#define INC_CLK_H
#include "st/stm32f0xx.h"

void clkEnablePLLInt( void );
void clkEnableSystick( void );;

void waitus( uint16_t const us );
void waitms( uint16_t const ms );

#endif
