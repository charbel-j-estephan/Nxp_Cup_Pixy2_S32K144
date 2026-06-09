/*==================================================================================================
 * FreeMASTER configuration for NXP Cup S32K144 / MR-CANHUBK344
 *
 * Transport : LPUART0  →  OpenSDA USB CDC  (virtual COM port on the debug-USB connector)
 * Baud rate : 115200
 * Mode      : Poll-driven  –  call FMSTR_Poll() from the main loop
 * TSA       : Enabled, ROM-resident  –  all variables auto-discovered by name in the
 *             FreeMASTER PC tool; no manual address entry needed.
 *
 * --------------------------------------------------------------------------
 * SDK REQUIREMENT
 * --------------------------------------------------------------------------
 * This file is the user configuration header consumed by the FreeMASTER
 * Embedded SDK (bare-metal, v3.x).  The SDK sources are NOT part of the
 * project repository.  Obtain them from one of:
 *   a) NXP MCUXpresso SDK builder – select component "FreeMASTER"
 *   b) GitHub: https://github.com/nxp-mcuxpresso/freemaster-bm
 *   c) S32DS install: <S32DS>\examples\<board>\freemaster_*
 *
 * Add to the S32DS project (Project → Properties → C/C++ Build → Settings →
 * GNU C Compiler → Includes):
 *   – The SDK top-level directory  (for freemaster.h, freemaster_tsa.h …)
 *   – src/drivers/drv_lpuart       (for freemaster_serial_lpuart.h)
 *   – This project's include/      (already there; picks up this file)
 *
 * Add to the source tree:
 *   freemaster.c
 *   freemaster_serial.c            (serial transport core)
 *   src/drivers/drv_lpuart/freemaster_serial_lpuart.c
 * --------------------------------------------------------------------------
 ==================================================================================================*/
#ifndef FREEMASTER_CFG_H
#define FREEMASTER_CFG_H

/* -----------------------------------------------------------------------
 * Platform
 * S32K144 is Cortex-M4 (32-bit little-endian) → selects freemaster_gen32le.h
 * ----------------------------------------------------------------------- */
#define FMSTR_PLATFORM_CORTEX_M     1

/* -----------------------------------------------------------------------
 * Global enable
 * Set FMSTR_DISABLE to 1 to stub out all FreeMASTER calls at zero cost.
 * ----------------------------------------------------------------------- */
#define FMSTR_DISABLE               0

/* -----------------------------------------------------------------------
 * Transport layer: use the built-in LPUART serial driver.
 * FMSTR_LPUART_BASE   : S32K144.h defines IP_LPUART0 = (LPUART_Type*)0x4006A000
 * FMSTR_LPUART_INDEX  : peripheral index 0
 *
 * Hardware wiring: LPUART0_TX = PTA3 (OpenSDA), LPUART0_RX = PTA2 (OpenSDA)
 * The OpenSDA chip on the debug-USB connector bridges these to a CDC virtual
 * COM port visible to the FreeMASTER PC tool as "COM x (Serial)".
 * ----------------------------------------------------------------------- */
#define FMSTR_TRANSPORT             FMSTR_SERIAL
#define FMSTR_SERIAL_DRV            FMSTR_SERIAL_S32_LPUART
#define FMSTR_LPUART_BASE           0x4006A000UL

/* -----------------------------------------------------------------------
 * Interrupt / polling model
 * POLL_DRIVEN = 1: call FMSTR_Poll() from the superloop.  No ISR needed.
 * ----------------------------------------------------------------------- */
#define FMSTR_LONG_INTR             0
#define FMSTR_SHORT_INTR            0
#define FMSTR_POLL_DRIVEN           1

/* -----------------------------------------------------------------------
 * Target-Side Addressing (TSA)
 * FMSTR_USE_TSA        – enable automatic symbol discovery
 * FMSTR_USE_TSA_SAFETY – 0 = full read-write access (debug; set 1 for
 *                        safety-critical builds to restrict writes)
 * FMSTR_USE_TSA_INROM  – store the TSA descriptor table in flash
 * ----------------------------------------------------------------------- */
#define FMSTR_USE_TSA               1
#define FMSTR_USE_TSA_SAFETY        0
#define FMSTR_USE_TSA_INROM         1

/* -----------------------------------------------------------------------
 * Oscilloscope recorder (optional – enable when you want time-domain traces)
 * Set FMSTR_USE_RECORDER to 1 and adjust FMSTR_REC_BUFF_SIZE as needed.
 * The buffer lives in RAM; 2 kB is a comfortable starting point.
 * ----------------------------------------------------------------------- */
#define FMSTR_USE_RECORDER          0
#define FMSTR_REC_BUFF_SIZE         2048U

/* -----------------------------------------------------------------------
 * Pipes – not needed for basic scope / TSA use
 * ----------------------------------------------------------------------- */
#define FMSTR_USE_PIPES             0

/* -----------------------------------------------------------------------
 * Miscellaneous
 * ------------------------------------------------------------