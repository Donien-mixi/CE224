#pragma once
/* R12 hardware trial: frozen STATE4_OBS14 law in phase1_catalog_control.h.
 * BALANCE_KP/KD/KI below are legacy defaults, NOT the active R12 controller.
 * Raw test data must never use the following sensor corrections. */
#define BALANCE_BUILD "BAL-20261015-R12"
#define BALANCE_PPR 898.225f
#define BALANCE_KP_ANGLE 48.0f
#define BALANCE_KD_ANGLE 3.6f
#define BALANCE_KP_VELOCITY 32.0f
#define BALANCE_KI_VELOCITY 0.5f
#define BALANCE_KP_POSITION 0.3f
#define BALANCE_DB_L_POS 160
#define BALANCE_DB_L_NEG 200
#define BALANCE_DB_R_POS 200
#define BALANCE_DB_R_NEG 140
#define BALANCE_ACC_BX (-0.0416836098593f)
#define BALANCE_ACC_BY (-0.105884356548f)
#define BALANCE_ACC_BZ 0.004738195334f
#define BALANCE_ACC_SX 0.997272837552f
#define BALANCE_ACC_SY 1.00758977736f
#define BALANCE_ACC_SZ 1.00929388014f
#define BALANCE_RAW_ZERO_DEG 2.16694264915f
/* Trial interlocks, separate from the simulated controller and acceptance bands. */
#define BALANCE_ARM_ANGLE 3.0f
#define BALANCE_ARM_GYRO 6.0f
#define BALANCE_ARM_WHEEL_SPEED 0.05f
#define BALANCE_ARM_STILL_CYCLES 40U // 200ms at200Hz; request still checked on STM32
#define BALANCE_INTEGRAL_DELAY_MS 3000U
#define BALANCE_INTEGRAL_LIMIT 2.0f
#define BALANCE_INTEGRAL_GATE_ANGLE 5.0f
#define BALANCE_INTEGRAL_GATE_SPEED 0.15f
#define BALANCE_HEADING_KP 500.0f  // PWM / m of right-minus-left wheel travel
#define BALANCE_HEADING_KD 40.0f   // PWM / (m/s)
#define BALANCE_HEADING_BETA 0.95f
#define BALANCE_HEADING_LIMIT 8.0f // smaller differential intervention while validating capture
#define BALANCE_STOP_ANGLE 25.0f
#define BALANCE_STOP_DISTANCE 0.30f
#define BALANCE_STOP_SPEED 0.80f
#define BALANCE_HEARTBEAT_MS 1000U
