/*==================================================================================================
 * telemetry.c  –  Binary telemetry stream to an ESP32 over I2C  (see telemetry.h)
 *
 * The S32K144 acts as the I2C master (re-using the OLED/Pixy2 bus, channel
 * I2cConf_I2cChannel_Display_Channel) and writes one fixed-layout binary packet
 * per tick to the ESP32 slave at TELEMETRY_ESP32_ADDR. Using the AUTOSAR I2c
 * driver's blocking I2c_SyncTransmit() keeps the call site trivial.
 *
 * Wire format – TelemetryPacket – is byte-packed (no padding) and little-endian
 * (both Cortex-M4 and ESP32 are little-endian), so the ESP32 receive handler can
 * memcpy the bytes straight into an identical packed struct. A leading sync byte
 * + sequence counter and a trailing XOR checksum let the ESP32 frame and verify
 * each packet.
 ==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#include "telemetry.h"
#include "CDD_I2c.h"   /* I2c_SyncTransmit(), I2c_RequestType, I2C_SEND_DATA, channel id */

/*------------------------------------------------------------------ wire format */
#define TELEMETRY_SYNC      (0xA5U)   /* frame-start marker            */
#define TELEMETRY_SOURCE    (0x01U)   /* 0x01 = "S32K" data source     */

/*
 * On-the-wire packet. Keep this layout in lock-step with the struct the ESP32
 * uses to decode it. __attribute__((packed)) removes all padding so the byte
 * offsets are identical on both sides.  Total size = 51 bytes.
 */
typedef struct __attribute__((packed))
{
    uint8  sync;             /* 0xA5                                    */
    uint8  source;           /* 0x01 = S32K                             */
    uint16 seq;              /* frame counter (wraps at 65535)          */
    sint16 steer_deg;
    float  speed_ms;
    float  yaw_rads;
    uint16 motor_pwm_us;
    uint8  pixy_vector_x0;
    uint8  pixy_vector_x1;
    float  cruise_speed_ref;
    float  kp;
    float  ki;
    float  accel_lat_g;
    float  accel_long_g;
    float  accel_vert_g;
    float  gyro_yaw_degs;
    float  temp_degC;
    uint8  checksum;         /* XOR of every preceding byte             */
} TelemetryPacket;

/*------------------------------------------------------------------ module state */
volatile TelemetryData TelemetryState = {0};

static uint8  TelemetryI2cChannel;
static uint16 TelemetrySeq;

/*------------------------------------------------------------------ public API */
void TelemetryInit(void)
{
    /* The I2C master is already configured by DriversInit(); we only record the
     * channel and reset the sequence counter. The ESP32 must be wired on the
     * same SDA/SCL as the OLED/Pixy2 and run as an I2C slave at the address
     * TELEMETRY_ESP32_ADDR. */
    TelemetryI2cChannel = (uint8)I2cConf_I2cChannel_Display_Channel;
    TelemetrySeq        = 0U;
}

void TelemetrySend(void)
{
    TelemetryPacket  pkt;
    I2c_RequestType  req;
    const uint8     *raw = (const uint8 *)&pkt;
    uint8            crc = 0U;
    uint16           i;

    /* snapshot the live state into the wire packet */
    pkt.sync             = TELEMETRY_SYNC;
    pkt.source           = TELEMETRY_SOURCE;
    pkt.seq              = TelemetrySeq;
    pkt.steer_deg        = TelemetryState.steer_deg;
    pkt.speed_ms         = TelemetryState.speed_ms;
    pkt.yaw_rads         = TelemetryState.yaw_rads;
    pkt.motor_pwm_us     = TelemetryState.motor_pwm_us;
    pkt.pixy_vector_x0   = TelemetryState.pixy_vector_x0;
    pkt.pixy_vector_x1   = TelemetryState.pixy_vector_x1;
    pkt.cruise_speed_ref = TelemetryState.cruise_speed_ref;
    pkt.kp               = TelemetryState.kp;
    pkt.ki               = TelemetryState.ki;
    pkt.accel_lat_g      = TelemetryState.accel_lat_g;
    pkt.accel_long_g     = TelemetryState.accel_long_g;
    pkt.accel_vert_g     = TelemetryState.accel_vert_g;
    pkt.gyro_yaw_degs    = TelemetryState.gyro_yaw_degs;
    pkt.temp_degC        = TelemetryState.temp_degC;
    pkt.checksum         = 0U;

    /* XOR checksum over every byte except the checksum field (the last byte) */
    for (i = 0U; i < (uint16)(sizeof(pkt) - 1U); i++)
    {
        crc ^= raw[i];
    }
    pkt.checksum = crc;

    /* Blocking master write of the whole packet to the ESP32. I2c_SyncTransmit
     * returns (with an error) on NACK/timeout, so a missing ESP32 can't hang
     * the control loop. */
    req.SlaveAddress         = (I2c_AddressType)TELEMETRY_ESP32_ADDR;
    req.BitsSlaveAddressSize = FALSE;          /* 7-bit address          */
    req.HighSpeedMode        = FALSE;
    req.ExpectNack           = FALSE;
    req.RepeatedStart        = FALSE;
    req.BufferSize           = (uint16)sizeof(pkt);
    req.DataDirection        = I2C_SEND_DATA;
    req.DataBuffer           = (I2c_DataType *)&pkt;

    (void)I2c_SyncTransmit(TelemetryI2cChannel, &req);

    TelemetrySeq++;
}

#ifdef __cplusplus
}
#endif
