/*
 * Copyright (C) 2017 Obermaier Johannes
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */
 
#include "clk.h"

/* Systick is used for wait* functions. We use it as a raw timer (without interrupts) for simplicity. */
#define F_CPU (48000000u)
#define SYSTICK_MAX (0xFFFFFFu)
#define SYSTICK_TICKS_PER_US (F_CPU / 1000000u)
#define SYSTICK_CSR_ADDR ((uint32_t *) 0xE000E010u)
#define SYSTICK_CVR_ADDR ((uint32_t *) 0xE000E018u)
#define SYSTICK_CSR_ON (0x00000005u)
/* Systick config end */

static uint32_t volatile *sysTickCSR = SYSTICK_CSR_ADDR;
static uint32_t volatile *sysTickCVR = SYSTICK_CVR_ADDR;


/* Choose 48MHz system clock using the PLL */
void clkEnablePLLInt( void )
{
	/* Flash Latency for >=24MHz: One wait state */
	FLASH->ACR |= FLASH_ACR_LATENCY;

	/* Prediv = 2 */
	RCC->CFGR2 = RCC_CFGR2_PREDIV1_DIV2;
	RCC->CFGR |= RCC_CFGR_PLLMUL12 | RCC_CFGR_PLLSRC_HSI_PREDIV;

	RCC->CR |= RCC_CR_PLLON;

	while (!(RCC->CR & RCC_CR_PLLRDY))
	{
		; /* Wait for clock to become stable */
	}

	RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_SW)) | RCC_CFGR_SW_PLL;

	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
	{
		; /* Wait for clock to become selected */
	}

	return ;
}


void clkEnableSystick( void )
{
	/* Initialize the Systick timer (no interrupts!) */
	*sysTickCSR = SYSTICK_CSR_ON;
	*sysTickCVR = SYSTICK_MAX;

	return ;
}


void waitus( uint16_t const us )
{
	uint32_t cmpTicks = 0u;

	*sysTickCVR = SYSTICK_MAX;

	cmpTicks = SYSTICK_MAX - ((uint32_t) us * SYSTICK_TICKS_PER_US);

	while (*sysTickCVR >= cmpTicks)
	{
		; /* Wait */
	}

	return ;
}


void waitms( uint16_t const ms )
{
	uint16_t timeMs = ms;

	while (timeMs)
	{
		waitus(1000u);
		--timeMs;
	}

	return ;
}
