/*==================================================================================================
 * FreeMASTER LPUART0 hardware initialisation  –  include/freemaster_lpuart.h
 *
 * Provides FmstrLpuartInit(), which must be called ONCE, BEFORE FMSTR_Init(),
 * to bring up LPUART0 at 115 200 baud on PTA2 (RX) / PTA3 (TX).
 *
 * Everything is done directly against the S32K144 register map (S32K144.h),
 * exactly as delay.c does with SysTick – no extra UART stack required.
 ==================================================================================================*/
#ifndef FREEMASTER_LPUART_H
#define FREEMASTER_LPUART_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/

/**
 * @brief   Initialise LPUART0 for FreeMASTER serial communication.
 *
 * @details Steps performed:
 *          1. Select FIRCDIV2 (48 MHz FIRC / 1) as the LPUART0 clock source
 *             via the SCG and PCC registers.
 *          2. Mux PTA2 → LPUART0_RX (ALT 6) and PTA3 → LPUART0_TX (ALT 6).
 *          3. Configure LPUART0 for 115 200 baud, 8N1 (OSR = 15, SBR = 26).
 *          4. Enable the transmitter and receiver.
 *
 *          Call once, after DriversInit() and before FMSTR_Init().
 */
void FmstrLpuartInit(void);

#ifdef __cplusplus
}
#endif

#endif /* FREEMASTER_LPUART_H */
