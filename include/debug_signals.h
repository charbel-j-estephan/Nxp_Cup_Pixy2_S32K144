/*==================================================================================================
 * debug_signals.h  –  extern declarations for every volatile FreeMASTER debug global.
 *
 * Each variable is DEFINED in the module that owns it (servo.c, esc.c, …).
 * This single header is the one-stop include for freemaster_tsa.c so that the
 * TSA table can reference every symbol without pulling in unrelated module
 * headers.
 *
 * Naming convention: Fmstr<Module><Signal>
 * Types: use AUTOSAR Std_Types (sint16, uint16, uint8, boolean) so they match
 *        the rest of the codebase and map cleanly to FreeMASTER TSA type strings.
 ==================================================================================================*/
#ifndef DEBUG_SIGNALS_H
#define DEBUG_SIGNALS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"   /* sint16, uint16, uint8, boolean */

/* -----------------------------------------------------------------------
 * main.c  –  steering + battery + throttle  (already defined there)
 * ----------------------------------------------------------------------- */
extern volatile sint16 FmstrDir;          /* steering command  –100(L)..+100(R) */
extern volatile uint16 FmstrPackMv;       /* whole 2S pack voltage, mV          */
extern volatile uint16 FmstrCellMv;       /* per-cell voltage estimate, mV       */
extern volatile uint16 FmstrThrottle;     /* PWM duty currently on ESC channel   */

/* -----------------------------------------------------------------------
 * servo.c
 * ----------------------------------------------------------------------- */
extern volatile sint16 FmstrSteerPosition;  /* mirrored from Steer() arg, –100..+100 */
extern volatile uint16 FmstrServoDutyCycle; /* raw PWM duty ticks written to servo    */

/* -----------------------------------------------------------------------
 * esc.c
 * ----------------------------------------------------------------------- */
extern volatile sint16 FmstrEscSpeed;       /* commanded speed –100..+100            */
extern volatile uint8  FmstrEscBrake;       /* 0 = off, 1 = braking                  */
extern volatile uint8  FmstrEscState;       /* EscStates cast to uint8:
                                             *   0=Forward 1=Braking 2=Neutral 3=Rev  */
extern volatile uint16 FmstrEscDutyCycle;   /* raw PWM duty ticks on ESC channel      */

/* -----------------------------------------------------------------------
 * brushless.c
 * ----------------------------------------------------------------------- */
extern volatile sint16 FmstrBrushlessSpeed;    /* commanded speed –100..+100         */
extern volatile uint8  FmstrBrushlessBrake;    /* 0 = off, 1 = braking               */
extern volatile uint8  FmstrBrushlessState;    /* BrushlessStates cast to uint8:
                                                *   0=Fwd 1=Braking 2=Neutral 3=Rev  */
extern volatile uint16 FmstrBrushlessDutyCycle;/* raw PWM duty ticks                 */

/* -----------------------------------------------------------------------
 * battery.c
 * ----------------------------------------------------------------------- */
extern volatile uint8  FmstrBatteryIsLow;      /* 0 = OK, 1 = low voltage trip       */

/* -----------------------------------------------------------------------
 * pixy2.c  –  up to 4 line vectors
 * ----------------------------------------------------------------------- */
extern volatile uint8  FmstrPixyVectorCount;   /* 0..N currently detected            */
extern volatile uint8  FmstrPixyX0[4];         /* vector[i] tail X  (0..78)          */
extern volatile uint8  FmstrPixyY0[4];         /* vector[i] tail Y  (0..51)          */
extern volatile uint8  FmstrPixyX1[4];         /* vector[i] head X                   */
extern volatile uint8  FmstrPixyY1[4];         /* vector[i] head Y                   */
extern volatile uint8  FmstrPixyIndex[4];      /* vector[i] index tag from Pixy2      */

/* -----------------------------------------------------------------------
 * button.c
 * ----------------------------------------------------------------------- */
extern volatile uint8  FmstrButtonPressed;     /* 0 = released, 1 = pressed          */

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_SIGNALS_H */
