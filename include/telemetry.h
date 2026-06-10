/*==================================================================================================
 * telemetry.h  –  Binary telemetry stream to an ESP32 over I2C
 *
 * The S32K144 is the I2C master (the same bus as the OLED/Pixy2) and the ESP32
 * is an I2C slave at TELEMETRY_ESP32_ADDR. Once per control tick (~20 ms) the
 * S32K writes one fixed-layout binary packet (see telemetry.c, TelemetryPacket)
 * to the ESP32. The ESP32 receives it in its Wire.onReceive() handler and
 * memcpy's the bytes into an identical packed struct.
 *
 * Usage:
 *     TelemetryInit();                       // once, after DriversInit()
 *     ...
 *     TelemetryState.steer_deg     = ...;    // fill the live fields you have
 *     TelemetryState.motor_pwm_us  = ...;
 *     TelemetrySend();                       // push one packet to the ESP32
 *
 * Fields the firmware does not compute yet (speed, yaw, IMU, temperature, PID
 * gains, …) simply stay 0 until the relevant sensor/control code writes them.
 ==================================================================================================*/
#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Std_Types.h"   /* sint16, uint16, uint8 */

/* ESP32 I2C slave address, 7-bit. Must match Wire.begin(0x42) on the ESP32. */
#define TELEMETRY_ESP32_ADDR   (0x42U)

/**
 * @brief Live telemetry inputs, in the units the ESP32 logger expects.
 *
 * Control- and sensor-side code writes these wherever it produces a value;
 * TelemetrySend() snapshots the whole struct into the binary I2C packet.
 */
typedef struct
{
    sint16 steer_deg;          /* steering angle, degrees  (-40..+40)        */
    uint16 motor_pwm_us;       /* ESC pulse width, microseconds (~1000..2000)*/
    uint8  pixy_vector_x0;     /* line-vector tail X pixel (0..78, centre 39) */
    uint8  pixy_vector_x1;     /* line-vector head X pixel (0..78, centre 39) */
    float  speed_ms;           /* ground speed, m/s                          */
    float  yaw_rads;           /* heading rate, rad/s                        */
    float  cruise_speed_ref;   /* target/cruise speed, m/s                   */
    float  kp;                 /* speed-loop proportional gain               */
    float  ki;                 /* speed-loop integral gain                   */
    float  accel_lat_g;        /* lateral acceleration, g                    */
    float  accel_long_g;       /* longitudinal acceleration, g               */
    float  accel_vert_g;       /* vertical acceleration, g                   */
    float  gyro_yaw_degs;      /* yaw rate, deg/s                            */
    float  temp_degC;          /* temperature, °C                            */
} TelemetryData;

/** Live telemetry state. Write its fields, then call TelemetrySend(). */
extern volatile TelemetryData TelemetryState;

/**
 * @brief Prepare telemetry over I2C. Call once, after DriversInit() (which
 *        already brings up the I2C master). Records the bus channel and resets
 *        the packet sequence counter.
 */
void TelemetryInit(void);

/**
 * @brief Snapshot TelemetryState into the binary packet and transmit it
 *        (blocking) to the ESP32 at TELEMETRY_ESP32_ADDR. Call every ~20 ms.
 */
void TelemetrySend(void);

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_H */
