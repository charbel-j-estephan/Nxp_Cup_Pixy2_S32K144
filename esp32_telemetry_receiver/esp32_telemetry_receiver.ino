/* ==================================================================================================
 *  esp32_telemetry_receiver.ino  –  ESP32 I2C slave that receives S32K144 telemetry
 *
 *  The S32K144 is the I2C MASTER and writes one fixed 51-byte binary packet (~every
 *  20 ms) to this ESP32, which runs as an I2C SLAVE at address 0x42. The packet layout
 *  must match TelemetryPacket in the S32K firmware (src/telemetry.c) byte-for-byte.
 *  Both MCUs are little-endian, so we just memcpy the received bytes into the struct.
 *
 *  Wiring (3.3 V logic on both sides):
 *      S32K SDA  <-> ESP32 SDA (GPIO21 default below)
 *      S32K SCL  <-> ESP32 SCL (GPIO22 default below)
 *      S32K GND  <-> ESP32 GND        (common ground REQUIRED)
 *  The OLED/Pixy2 already provide bus pull-ups; do not add extra strong pull-ups.
 *
 *  Output: one CSV line per received packet on Serial @115200, matching the columns
 *      source,steer_deg,speed_ms,yaw_rads,motor_pwm_us,pixy_vector_x0,pixy_vector_x1,
 *      cruise_speed_ref,kp,ki,accel_lat_g,accel_long_g,accel_vert_g,gyro_yaw_degs,temp_degC
 * ================================================================================================== */

#include <Wire.h>
#include <string.h>

#define TELEM_SLAVE_ADDR   0x42      /* must match TELEMETRY_ESP32_ADDR on the S32K */
#define TELEM_SDA_PIN      21
#define TELEM_SCL_PIN      22
#define TELEM_SYNC         0xA5
#define TELEM_SOURCE_S32K  0x01

/* Wire format – keep IN LOCK-STEP with TelemetryPacket in the S32K firmware. */
#pragma pack(push, 1)
typedef struct {
  uint8_t  sync;             /* 0xA5 */
  uint8_t  source;           /* 0x01 = S32K */
  uint16_t seq;              /* frame counter */
  int16_t  steer_deg;
  float    speed_ms;
  float    yaw_rads;
  uint16_t motor_pwm_us;
  uint8_t  pixy_vector_x0;
  uint8_t  pixy_vector_x1;
  float    cruise_speed_ref;
  float    kp;
  float    ki;
  float    accel_lat_g;
  float    accel_long_g;
  float    accel_vert_g;
  float    gyro_yaw_degs;
  float    temp_degC;
  uint8_t  checksum;         /* XOR of every preceding byte */
} TelemetryPacket;
#pragma pack(pop)

static volatile TelemetryPacket g_pkt;
static volatile bool            g_have_packet = false;
static volatile uint32_t        g_bad_frames  = 0;

/* ISR context: copy the incoming bytes out fast, validate, flag for loop(). */
static void onI2cReceive(int numBytes) {
  uint8_t buf[sizeof(TelemetryPacket)];
  int n = 0;
  while (Wire.available() && n < (int)sizeof(buf)) {
    buf[n++] = (uint8_t)Wire.read();
  }
  /* drain any extra bytes so the next frame starts clean */
  while (Wire.available()) { (void)Wire.read(); }

  if (n != (int)sizeof(TelemetryPacket) || buf[0] != TELEM_SYNC) {
    g_bad_frames++;
    return;
  }
  uint8_t crc = 0;
  for (int i = 0; i < (int)sizeof(TelemetryPacket) - 1; i++) crc ^= buf[i];
  if (crc != buf[sizeof(TelemetryPacket) - 1]) {
    g_bad_frames++;
    return;
  }
  memcpy((void *)&g_pkt, buf, sizeof(TelemetryPacket));
  g_have_packet = true;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(TELEM_SLAVE_ADDR, TELEM_SDA_PIN, TELEM_SCL_PIN, 0);
  Wire.onReceive(onI2cReceive);

  Serial.println(F("source,steer_deg,speed_ms,yaw_rads,motor_pwm_us,"
                   "pixy_vector_x0,pixy_vector_x1,cruise_speed_ref,kp,ki,"
                   "accel_lat_g,accel_long_g,accel_vert_g,gyro_yaw_degs,temp_degC"));
}

void loop() {
  if (!g_have_packet) return;

  noInterrupts();
  TelemetryPacket p = *(TelemetryPacket *)&g_pkt;
  g_have_packet = false;
  interrupts();

  /* CSV line, S32K column order. Floats with enough precision for yaw/gains. */
  Serial.printf("S32K,%d,%.3f,%.4f,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f\n",
                p.steer_deg, p.speed_ms, p.yaw_rads, p.motor_pwm_us,
                p.pixy_vector_x0, p.pixy_vector_x1, p.cruise_speed_ref,
                p.kp, p.ki, p.accel_lat_g, p.accel_long_g, p.accel_vert_g,
                p.gyro_yaw_degs, p.temp_degC);
}
