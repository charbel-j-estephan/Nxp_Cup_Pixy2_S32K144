# ESC 2 Setup — Handover

**Project:** Nxp_Cup_Pixy2_S32K144 (NXP Cup line-follower)
**MCU:** S32K144 LQFP100, Cortex-M4F @ 48 MHz
**Toolchain:** S32 Design Studio 3.6.5, AUTOSAR RTD 4.7
**Goal:** Add a second brushless ESC (ESC 2) alongside the already-working ESC 1.

---

## Status at handover

| Item | State |
|------|-------|
| ESC 1 (PTE2 / FTM3_CH6, logical Pwm ch 0) | ✅ Works — arms and spins |
| ESC 2 firmware/signal chain (PTD3 / FTM3_CH5, logical Pwm ch 5) | ✅ Verified correct end-to-end |
| ESC 2 physically arming | ⏳ Pending — needs one-time throttle calibration (program added, not yet confirmed working) |

**Bottom line:** The board, pins, pin-mux, FTM config, and brushless driver are all proven correct. ESC 2 is a healthy, identical unit that was simply never throttle-calibrated. A one-time calibration routine is now in `main.c`; the user needs to run it and confirm.

---

## Pin / channel reference

| ESC | Pin | Phys pin | FTM hardware ch | Logical Pwm ch | Pin mux |
|-----|-----|----------|-----------------|----------------|---------|
| ESC 1 | PTE2 | 85 | FTM3_CH6 | 0 (has notification callback) | ALT4 |
| ESC 2 | PTD3 | 70 | FTM3_CH5 | 5 (slave, no callback) | **ALT2** |

- FTM3 PWM ≈ 41 Hz, period = 40000 ticks.
- Pulse widths (raw ticks): **1638 = 1 ms (min)**, **2457 = 1.5 ms (neutral)**, **3276 = 2 ms (max)**.
- Note: FTM3 is **ALT4 on PORTE** but **ALT2 on PORTD** — different ports, different ALT numbers. ALT2 for PTD3/FTM3_CH5 confirmed against the S32K144 datasheet.
- `main.c` drives both via `BrushlessInit(0U, 5U, 1638U, 2457U, 3276U)`.

---

## The key problem we solved (and why)

The S32DS **config tool would not reliably emit PTD3** into the generated pin-mux table
(`generate/src/Port_Ci_Port_Ip_VS_0_PBcfg.c`). Root cause: the Pins/Port code generator did
not run after PTD3 was added to the `.mex`, and clicking **Update Code** only regenerated the
Peripherals (Pwm) component. Hand-editing the generated files worked but got **wiped on any
regeneration**.

**Final, robust fix:** stop relying on the generator for this pin. `main.c` now muxes PTD3
directly in code, every boot:

```c
#include "Port_Ci_Port_Ip.h"
...
Port_Ci_Port_Ip_SetMuxModeSel(IP_PORTD, 3U, PORT_MUX_ALT2);   /* PTD3 -> FTM3_CH5 */
```

PTD3 is **not** in `g_pin_mux_InitConfigArr_VS_0`, so `Port_Init()` never touches it — this
one call is the sole owner of PTD3's mux. **Nothing the config tool does can break ESC 2's pin
again.** Do NOT need to re-add PTD3 in the Pins tool; the runtime call is authoritative.

---

## Files changed

| File | Change | Notes |
|------|--------|-------|
| `src/main.c` | Added `#include "Port_Ci_Port_Ip.h"` | For the mux call |
| `src/main.c` | Added `Port_Ci_Port_Ip_SetMuxModeSel(IP_PORTD, 3U, PORT_MUX_ALT2)` after `DriversInit()` | **Permanent** — keep this |
| `src/main.c` | Added **ONE-TIME ESC 2 CALIBRATION** block (clearly marked) | **Temporary** — delete after ESC 2 arms |
| `generate/src/Pwm_VS_0_PBcfg.c` | Channel-0 callback = `Brushless_Period_Finished` | Made permanent earlier via config tool's Notification field |

> The pin-mux is now handled in code, so the earlier hand-edits to
> `generate/src/Port_Ci_Port_Ip_VS_0_PBcfg.c` and `board/Port_Ci_Port_Ip_Cfg.h`
> are no longer needed and may have been reverted by regeneration — that's fine.

---

## Diagnostic facts established

- Build is clean and re-flashed (confirmed by user).
- Power-up behavior: **ESC 1 arms, ESC 2 does repeating beep-beep-beep.**
- **Swap test:** moving ESC 2's signal wire onto PTE2 (ESC 1's known-good pin) → ESC 2 **still beeps.**
  → Proves the fault is **ESC 2 itself**, not the MCU / pin / config.
- ESC 2 shares the **same battery/ground** as ESC 1 → ground is common, not the issue.
- ESC 2 is the **same module** as ESC 1; no manual available.
- `brushless.c` drives **both** channels (`Pwm_SetDutyCycle(Channel2=5, ...)` in `BrushlessInit` and `SetPwm`) → ESC 2 does get neutral + speed updates.
- FTM3 CH5 hardware config verified: `ChannelId 5`, `ChOutputEn TRUE`, present in output array.

**Conclusion:** ESC 2 needs a one-time throttle calibration (ESC 1 was calibrated; ESC 2 was not).

---

## Current pending task: run the ESC 2 calibration

A one-time calibration block is in `main.c` (between the PTD3 mux call and `BrushlessInit`).
It holds ESC 1 at neutral and walks ESC 2 through FULL → MIN → NEUTRAL.

**Procedure — KEEP ALL WHEELS OFF THE GROUND:**
1. Build + flash with the **ESC battery UNPLUGGED** (USB power only).
2. Board resets and drives ESC 2 to **FULL**. ESC 1 held at neutral.
3. **Immediately plug in the ESC battery** (within ~15 s).
   → ESC 2 sees full throttle at power-up → enters calibration (tones, then HIGH-point beep).
4. After 15 s code → **MIN** (LOW-point beep).
5. After 8 s code → **NEUTRAL** → ESC 2 arms (quiet).
6. Normal drive loop runs (both motors ramp).

**Expected:** ESC 2 goes tones → high beep → low beep → quiet/armed → spins.

**If ESC 2 spins at full instead of calibrating:** this ESC uses a different calibration
trigger — unplug immediately, and the sequence/timings (currently 15 s / 8 s) need adjusting.
Report exactly what ESC 2 does at each stage so the timings can be tuned.

**After ESC 2 arms reliably:** delete the marked calibration block in `main.c` (leave the
PTD3 mux call in place).

---

## Next steps

1. Run the calibration procedure above; confirm ESC 2 arms.
2. If timings are off, report ESC 2's behavior per stage and tune the `DelayMs` values.
3. Once both ESCs arm + spin: delete the calibration block, keep the PTD3 mux line.
4. Final two-motor verification (ramp up/down loop already in `main.c`).

## Reliable power-up (every run)
Board running first (it outputs neutral), **then** plug in the ESC battery. Continuous beeping
= ESC powered before a valid neutral was present. Wheels off the ground until verified.
