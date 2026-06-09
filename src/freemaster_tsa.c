/*==================================================================================================
 * FreeMASTER TSA descriptor table  –  src/freemaster_tsa.c
 *
 * TARGET-SIDE ADDRESSING (TSA) lets the FreeMASTER PC tool discover every
 * variable automatically by name.  Just connect, load the .elf, and all
 * signals listed below appear in the Variable Watch / Scope pane – no
 * manual address entry required.
 *
 * HOW IT WORKS
 *   FMSTR_TSA_TABLE_BEGIN/END macros generate a compact ROM descriptor that
 *   pairs each variable's runtime address with its symbol name and type
 *   string.  FMSTR_TSA_TABLE_LIST_BEGIN/END registers the table with the
 *   FreeMASTER protocol engine.
 *
 * ACCESS RIGHTS
 *   FMSTR_TSA_RW_VAR  – FreeMASTER may read AND write the variable.
 *   FMSTR_TSA_RO_VAR  – read-only (safer for live data).
 *   FMSTR_TSA_RO_MEM  – read-only block of memory (arrays).
 *   FMSTR_TSA_RW_MEM  – read-write block.
 *
 * FMSTR_USE_TSA_SAFETY (0 in freemaster_cfg.h) means all entries are
 * accessible regardless of RW/RO – change to 1 for production builds.
 ==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
 ==================================================================================================*/
#include "freemaster.h"      /* FreeMASTER SDK – provides TSA macros     */
#include "debug_signals.h"   /* extern volatile declarations for all Fmstr* globals */

/*==================================================================================================
 *                                    TSA DESCRIPTOR TABLE
 ==================================================================================================*/

FMSTR_TSA_TABLE_BEGIN(NxpCupSignals)

    /*--------------------------------------------------------------------
     * ── main.c ──  steering + battery + throttle
     *------------------------------------------------------------------*/
    FMSTR_TSA_RW_VAR(FmstrDir,         FMSTR_TSA_SINT16)  /* steering –100(L)..+100(R)   */
    FMSTR_TSA_RO_VAR(FmstrPackMv,      FMSTR_TSA_UINT16)  /* whole 2S pack voltage, mV   */
    FMSTR_TSA_RO_VAR(FmstrCellMv,      FMSTR_TSA_UINT16)  /* per-cell voltage, mV        */
    FMSTR_TSA_RW_VAR(FmstrThrottle,    FMSTR_TSA_UINT16)  /* PWM duty on ESC ch0         */

    /*--------------------------------------------------------------------
     * ── servo.c ──  steering actuator
     *------------------------------------------------------------------*/
    FMSTR_TSA_RO_VAR(FmstrSteerPosition,  FMSTR_TSA_SINT16)  /* –100(left)..+100(right)  */
    FMSTR_TSA_RO_VAR(FmstrServoDutyCycle, FMSTR_TSA_UINT16)  /* raw PWM ticks            */

    /*--------------------------------------------------------------------
     * ── esc.c ──  single-motor Electronic Speed Controller
     *------------------------------------------------------------------*/
    FMSTR_TSA_RW_VAR(FmstrEscSpeed,      FMSTR_TSA_SINT16)  /* commanded –100..+100      */
    FMSTR_TSA_RO_VAR(FmstrEscBrake,      FMSTR_TSA_UINT8)   /* 0=off 1=braking           */
    FMSTR_TSA_RO_VAR(FmstrEscState,      FMSTR_TSA_UINT8)   /* 0=Fwd 1=Brk 2=Neu 3=Rev   */
    FMSTR_TSA_RO_VAR(FmstrEscDutyCycle,  FMSTR_TSA_UINT16)  /* raw PWM ticks             */

    /*--------------------------------------------------------------------
     * ── brushless.c ──  dual-motor brushless ESC pair
     *------------------------------------------------------------------*/
    FMSTR_TSA_RW_VAR(FmstrBrushlessSpeed,     FMSTR_TSA_SINT16) /* –100..+100             */
    FMSTR_TSA_RO_VAR(FmstrBrushlessBrake,     FMSTR_TSA_UINT8)  /* 0=off 1=braking        */
    FMSTR_TSA_RO_VAR(FmstrBrushlessState,     FMSTR_TSA_UINT8)  /* 0=Fwd 1=Brk 2=Neu 3=Rev*/
    FMSTR_TSA_RO_VAR(FmstrBrushlessDutyCycle, FMSTR_TSA_UINT16) /* raw PWM ticks          */

    /*--------------------------------------------------------------------
     * ── battery.c ──  voltage monitor
     *------------------------------------------------------------------*/
    FMSTR_TSA_RO_VAR(FmstrBatteryIsLow,  FMSTR_TSA_UINT8)   /* 0=OK, 1=low-voltage trip  */

    /*--------------------------------------------------------------------
     * ── pixy2.c ──  line vectors (up to 4)
     * FmstrPixyVectorCount  : number of currently detected vectors (0-4)
     * FmstrPixyX0/Y0        : tail pixel coordinates
     * FmstrPixyX1/Y1        : head pixel coordinates
     * FmstrPixyIndex        : Pixy2 internal vector index tag
     *------------------------------------------------------------------*/
    FMSTR_TSA_RO_VAR(FmstrPixyVectorCount, FMSTR_TSA_UINT8)
    FMSTR_TSA_RO_MEM("FmstrPixyX0",    FMSTR_TSA_UINT8,
                      &FmstrPixyX0[0],    4U * sizeof(uint8))
    FMSTR_TSA_RO_MEM("FmstrPixyY0",    FMSTR_TSA_UINT8,
                      &FmstrPixyY0[0],    4U * sizeof(uint8))
    FMSTR_TSA_RO_MEM("FmstrPixyX1",    FMSTR_TSA_UINT8,
                      &FmstrPixyX1[0],    4U * sizeof(uint8))
    FMSTR_TSA_RO_MEM("FmstrPixyY1",    FMSTR_TSA_UINT8,
                      &FmstrPixyY1[0],    4U * sizeof(uint8))
    FMSTR_TSA_RO_MEM("FmstrPixyIndex", FMSTR_TSA_UINT8,
                      &FmstrPixyIndex[0], 4U * sizeof(uint8))

    /*--------------------------------------------------------------------
     * ── button.c ──  start/stop button on PTE14
     *------------------------------------------------------------------*/
    FMSTR_TSA_RO_VAR(FmstrButtonPressed, FMSTR_TSA_UINT8)  /* 0=released 1=pressed       */

FMSTR_TSA_TABLE_END()

/*==================================================================================================
 *                            TSA DIRECTORY  (registers the table above)
 ==================================================================================================*/
FMSTR_TSA_TABLE_LIST_BEGIN()
    FMSTR_TSA_TABLE(NxpCupSignals)
FMSTR_TSA_TABLE_LIST_END()

#ifdef __cplusplus
}
#endif
