#pragma once
#include <stdint.h>
/* Little endian IEEE754 float; CRC16 CCITT FFFF over all except crc.
 * Decimated at 100Hz from 200Hz control. IMU ODR100Hz (not synchronised).
 * PWM is the output of this cycle; raw sensors and ticks precede that output.
 * ticks_r is the original hardware sign, before direction correction. */
typedef struct __attribute__((packed)) {
    uint16_t sync;
    uint32_t sequence, t_ms;
    uint16_t dt_us, execution_us;
    int16_t gx, gy, gz, ax, ay, az;
    int32_t ticks_l, ticks_r;
    int16_t pwm_l, pwm_r;
    float pitch, gyro, v_l, v_r, x, v_cmd, theta_cmd, pwm_base, integral;
    uint16_t fault, dropped;
    uint8_t state, ready;
    uint16_t crc;
} BalanceFrame;
#ifdef __cplusplus
static_assert(sizeof(BalanceFrame)==82,"balance wire layout");
#else
_Static_assert(sizeof(BalanceFrame)==82,"balance wire layout");
#endif
