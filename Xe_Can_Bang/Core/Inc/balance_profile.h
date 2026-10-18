#pragma once
/* Final R12 STATE4_OBS14 baseline. Legacy PID constants below support
 * diagnostic/library code only; they are not a selectable balance controller.
 * Raw test data must never use the following sensor corrections. */
#define BALANCE_BUILD "BAL-20261018-R12-SPEED2-8DIR"
#define BALANCE_CONTROLLER_R12 12U
#define BALANCE_DEFAULT_CONTROLLER BALANCE_CONTROLLER_R12
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
#define BALANCE_ARM_ANGLE 60.0f // fixed R12 startup gate
#define BALANCE_ARM_ANGLE_R12 60.0f // operator reports successful 60deg starts; frozen S60X law
#define BALANCE_ARM_ANGLE_FOR(profile) ((profile)==BALANCE_CONTROLLER_R12?BALANCE_ARM_ANGLE_R12:0.0f)
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
#define BALANCE_CAPTURE_SPEED 1.00f // only during bounded startup capture
#define BALANCE_LARGE_CAPTURE_ANGLE 48.0f // preserve S48T speed guard through48deg
#define BALANCE_LARGE_CAPTURE_SPEED 1.10f // only active capture started above48deg
#define BALANCE_HEARTBEAT_MS 1000U

#define BALANCE_EXTREME_CAPTURE_ANGLE 58.0f
#define BALANCE_EXTREME_CAPTURE_SPEED 1.20f
#define BALANCE_CAPTURE_TIMEOUT_MS 3000U
#define BALANCE_EXTREME_CAPTURE_TIMEOUT_MS 4000U
