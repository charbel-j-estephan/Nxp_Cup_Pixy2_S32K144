/*==================================================================================================
 * FreeMASTER LPUART0 hardware initialisation  –  src/freemaster_lpuart.c
 *
 * Brings up LPUART0 at 115 200 baud on PTA2 (RX) / PTA3 (TX) using direct
 * register access via S32K144.h, exactly the same approach that delay.c
 * uses for SysTick.  No AUTOSAR UART stack is required.
 *
 * Call order in main():
 *   DriversInit();           // clock tree, Port, I2C, PWM, ADC, etc.
 *   FmstrLpuartInit();       // PCC clock + pin mux + LPUART0 baud rate
 *   FMSTR_Init();            // FreeMASTER protocol layer
 *   ...
 *   while(1) { FMSTR_Poll(); ... }
 ==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "freemaster_lpuart.h"
#include "S32K144.h"          /* IP_LPUART0, IP_SCG, IP_PCC, IP_PORTA, register bit-field macros */

/*==================================================================================================
 *                                         LOCAL MACROS
 ==================================================================================================*/

/*
 * Baud-rate computation  (all values for 48 MHz source clock):
 *   Desired baud  : 115 200
 *   Oversampling  : 16x  → OSR field = 15  (field value = desired ratio – 1)
 *   SBR divisor   : 48 000 000 / (115 200 × 16) = 26.04  → 26
 *   Actual baud   : 48 000 000 / (26 × 16)       = 115 385  (+0.16 %, well within spec)
 */
#define FMSTR_LPUART_OSR_VAL    (15U)   /* 16× oversampling (field value = OSR – 1) */
#define FMSTR_LPUART_SBR_VAL    (26U)   /* baud-rate divisor                        */

/*
 * PCC peripheral-clock-source selector for LPUART0.
 *   PCS = 011b = FIRCDIV2_CLK  (Fast IRC 48 MHz divided by FIRCDIV2 factor)
 * The FIRC is always available on S32K144; FIRCDIV2 is set to /1 below.
 */
#define FMSTR_LPUART_PCS        (3U)    /* PCS = 011 = FIRCDIV2 clock source */

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
 ==================================================================================================*/

void FmstrLpuartInit(void)
{
    /*------------------------------------------------------------------
     * Step 1 – Configure SCG FIRCDIV2 = divide-by-1 → 48 MHz.
     *
     * SCG_FIRCDIV register: bits [13:8] = FIRCDIV2
     *   000 = clock disabled
     *   001 = /1  (48 MHz)
     *   010 = /2  (24 MHz)   etc.
     *
     * We only touch FIRCDIV2 (bits 13:8); FIRCDIV1 (bits 5:0) is left
     * as-is so we don't disturb any other peripheral already using it.
     *------------------------------------------------------------------*/
    IP_SCG->FIRCDIV = (IP_SCG->FIRCDIV & ~SCG_FIRCDIV_FIRCDIV2_MASK)
                   |  SCG_FIRCDIV_FIRCDIV2(1U);

    /*------------------------------------------------------------------
     * Step 2 – Enable the LPUART0 peripheral clock via PCC.
     *
     * PCC rules: CGC must be 0 before changing PCS.
     * PCC_LPUART0_INDEX is defined in S32K144.h.
     *------------------------------------------------------------------*/
    IP_PCC->PCCn[PCC_LPUART0_INDEX] &= ~PCC_PCCn_CGC_MASK;                      /* gate off  */
    IP_PCC->PCCn[PCC_LPUART0_INDEX]  = PCC_PCCn_PCS(FMSTR_LPUART_PCS)          /* FIRCDIV2  */
                                      | PCC_PCCn_CGC_MASK;                       /* gate on   */

    /*------------------------------------------------------------------
     * Step 3 – Mux PTA2 → LPUART0_RX and PTA3 → LPUART0_TX (ALT 6).
     *
     * DriversInit() has already called Port_Init() which may have set
     * these as GPIO.  Overwriting PCR here is safe because PTA2/PTA3
     * are the OpenSDA UART bridge pins and are unused by car-control
     * logic.  PORT_PCR_MUX(6) = bit-field MUX = 0b110.
     *------------------------------------------------------------------*/
    IP_PORTA->PCR[2] = PORT_PCR_MUX(6U);   /* PTA2 = LPUART0_RX */
    IP_PORTA->PCR[3] = PORT_PCR_MUX(6U);   /* PTA3 = LPUART0_TX */

    /*------------------------------------------------------------------
     * Step 4 – Disable TX and RX before touching the BAUD register
     * (required by the S32K144 Reference Manual, §47.5.1).
     *------------------------------------------------------------------*/
    IP_LPUART0->CTRL &= ~(LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK);

    /*------------------------------------------------------------------
     * Step 5 – Program baud rate: 115 200 @ 48 MHz, 8N1.
     *
     * BAUD[OSR]  (bits 28:24) = oversampling ratio – 1  → 15 for 16×
     * BAUD[SBR]  (bits 12:0)  = baud-rate divisor       → 26
     * All other BAUD fields (SBNS, RXEDGIE, LBKDIE, …) are left 0
     * giving: 1 stop bit, no parity, no LIN break detect.
     *------------------------------------------------------------------*/
    IP_LPUART0->BAUD = LPUART_BAUD_OSR(FMSTR_LPUART_OSR_VAL)
                     | LPUART_BAUD_SBR(FMSTR_LPUART_SBR_VAL);

    /*------------------------------------------------------------------
     * Step 6 – Enable transmitter and receiver (8N1 is the reset default
     * for CTRL so no parity or loop-back bits need to be cleared).
     *------------------------------------------------------------------*/
    IP_LPUART0->CTRL = LPUART_CTRL_TE_MASK | LPUART_CTRL_RE_MASK;
}