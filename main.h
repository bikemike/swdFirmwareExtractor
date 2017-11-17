/*
 * Copyright (C) 2017 Obermaier Johannes
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#ifndef INC_MAIN_H
#define INC_MAIN_H
#include <stdint.h>


#ifndef NULL
#define NULL ((void*) 0)
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define GPIO_LED_BLUE (GPIOC)
#define GPIO_LED_GREEN (GPIOC)
#define GPIO_BUTTON (GPIOA)

#define PIN_LED_BLUE (8u)
#define PIN_LED_GREEN (9u)
#define PIN_BUTTON (0u)

#define MAX_READ_ATTEMPTS (100u)

/* all times in milliseconds */
/* minimum wait time between reset deassert and attack */
#define DELAY_JITTER_MS_MIN (20u)
/* increment per failed attack */
#define DELAY_JITTER_MS_INCREMENT (1u)
/* maximum wait time between reset deassert and attack */
#define DELAY_JITTER_MS_MAX (50u)

/* flash readout statistics */
typedef struct {
	uint32_t numAttempts;
	uint32_t numSuccess;
	uint32_t numFailure;
} extractionStatistics_t;

void printExtractionStatistics( void );

#endif
