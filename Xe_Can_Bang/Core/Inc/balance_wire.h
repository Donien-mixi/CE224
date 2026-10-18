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
    uint8_t controller; // 7 or 12; protocol v6 sync0x5ea5, never inferred from PWM
    uint8_t capture; // phase:0 normal,1 startup,2 disturbance recovery; 104-byte motion layout
    float distance_limit; // allocated once on ARM, hard ceiling1m
    float x_ref,v_ref;
    uint32_t drive_epoch;
    uint16_t motion_flags;
    int16_t drive_direction;
    uint16_t crc;
} BalanceFrame;
#ifdef __cplusplus
static_assert(sizeof(BalanceFrame)==104,"balance wire layout");
#else
_Static_assert(sizeof(BalanceFrame)==104,"balance wire layout");
#endif
