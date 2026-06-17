/*==================================================================================================
*    Copyright 2021-2024 NXP
*
*    NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be
*    used strictly in accordance with the applicable license terms. By expressly
*    accepting such terms or by downloading, installing, activating and/or otherwise
*    using the software, you are agreeing that you have read, and that you agree to
*    comply with and are bound by, such license terms. If you do not agree to be
*    bound by the applicable license terms, then you may not retain, install,
*    activate or otherwise use the software.
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "delay.h"
#include "S32K144.h"

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
/* SysTick reload is 24-bit, so cap a single hardware load well below the limit (0xFFFFFF ticks).
 * 100 ms at any core clock up to ~167 MHz still fits, so chunk longer waits 100 ms at a time. */
#define DELAY_MAX_CHUNK_US   (100000U)

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static uint32 DelayTicksPerUs = 48U; /* default for the project's 48 MHz core clock */

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
void DelayInit(uint32 CoreClockHz){
    DelayTicksPerUs = CoreClockHz / 1000000U;
    /* Select core clock as the SysTick source, keep the counter and interrupt disabled. */
    S32_SysTick->CSRr = S32_SysTick_CSR_CLKSOURCE(1U);
}

void DelayUs(uint32 Microseconds){
    while(Microseconds > 0U){
        uint32 Chunk = (Microseconds > DELAY_MAX_CHUNK_US) ? DELAY_MAX_CHUNK_US : Microseconds;
        uint32 Reload = (Chunk * DelayTicksPerUs) - 1U;

        S32_SysTick->CSRr = 0U;                                    /* stop counter             */
        S32_SysTick->RVR = Reload & S32_SysTick_RVR_RELOAD_MASK;  /* load 24-bit reload value */
        S32_SysTick->CVR = 0U;                                    /* clear current + COUNTFLAG */
        S32_SysTick->CSRr = S32_SysTick_CSR_CLKSOURCE(1U) | S32_SysTick_CSR_ENABLE(1U);

        /* Wait until the counter underflows (COUNTFLAG sets to 1). */
        while((S32_SysTick->CSRr & S32_SysTick_CSR_COUNTFLAG_MASK) == 0U){
            /* busy wait */
        }
        S32_SysTick->CSRr = 0U;                                    /* stop counter             */

        Microseconds -= Chunk;
    }
}

void DelayMs(uint32 Milliseconds){
    while(Milliseconds > 0U){
        DelayUs(1000U);
        Milliseconds--;
    }
}

void DelayStartPeriod(uint32 Microseconds){
    uint32 Reload = (Microseconds * DelayTicksPerUs) - 1U;

    S32_SysTick->CSRr = 0U;                                    /* stop counter             */
    S32_SysTick->RVR = Reload & S32_SysTick_RVR_RELOAD_MASK;  /* load 24-bit reload value */
    S32_SysTick->CVR = 0U;                                    /* clear current + COUNTFLAG */
    S32_SysTick->CSRr = S32_SysTick_CSR_CLKSOURCE(1U) | S32_SysTick_CSR_ENABLE(1U);
}

void DelayWaitPeriodEnd(void){
    /* Block until the counter underflows (COUNTFLAG sets). If the loop work already overran the
     * period, COUNTFLAG is already set and this returns immediately -- a missed deadline, not a
     * stall, so the control rate degrades gracefully instead of hanging. */
    while((S32_SysTick->CSRr & S32_SysTick_CSR_COUNTFLAG_MASK) == 0U){
        /* busy wait */
    }
    S32_SysTick->CSRr = 0U;                                    /* stop counter             */
}

#ifdef __cplusplus
}
#endif

/** @} */
