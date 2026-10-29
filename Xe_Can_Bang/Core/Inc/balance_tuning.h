#pragma once
/* RT4-T1 test calibration. Units are included in names; positive yaw is left.
 * Keep state machines and feedback gains out of this configuration file.
 * Changes require rebuilding STM32 and the native simulation libraries. */
#define BAL_TUNE_FORWARD_NORMAL_MPS 0.50f
#define BAL_TUNE_FORWARD_FAST_MPS 0.72f
#define BAL_TUNE_REVERSE_NORMAL_MPS 0.55f
#define BAL_TUNE_REVERSE_FAST_MPS 0.72f

/* Acceleration and braking, m/s^2. */
#define BAL_TUNE_ACCEL_LOW 0.30f
#define BAL_TUNE_ACCEL_HIGH 0.15f
#define BAL_TUNE_BRAKE 0.60f
#define BAL_TUNE_REVERSE_BRAKE 0.45f

/* Pure turns and curves, rad/s. Scale only pure left turns. */
#define BAL_TUNE_PURE_NORMAL_RADPS 1.20f
#define BAL_TUNE_PURE_FAST_RADPS 2.20f
#define BAL_TUNE_CURVE_NORMAL_RADPS 0.80f
#define BAL_TUNE_CURVE_FAST_RADPS 0.55f
#define BAL_TUNE_LEFT_PURE_RATE_SCALE 1.10f

/* Predict overspeed in either direction before the normal 0.8m/s cutoff.
 * All three thresholds AND outward speed error are required; REC1 must be
 * primed. REC1 still cancels the gesture and requires a new press. */
#define BAL_TUNE_RESERVE_REFERENCE_MPS 0.40f
#define BAL_TUNE_RESERVE_SPEED_MPS 0.65f
#define BAL_TUNE_RESERVE_ERROR_MPS 0.08f
