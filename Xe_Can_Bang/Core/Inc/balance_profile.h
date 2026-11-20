#pragma once
/* Final R12 STATE4_OBS14 baseline. Legacy PID constants below support
 * diagnostic/library code only; they are not a selectable balance controller.
 * Raw test data must never use the following sensor corrections. */
#define BALANCE_BUILD "BAL-20261120-R12-8DIR-RT42-RF"
/* RT29: the disturbance brake (REC1 entry, hold_anchor) has no4s completion
 * deadline. Log RT28 (12)(3): three auto-cuts, all fault4096, at t=11.52/85.50/
 *156.48s, each exactly4s after a REC1 entry, each on an upright vehicle
 * (|pitch|<=1.1deg, |v|<=0.22mps) - the brake could not complete because the
 * operator kept pushing. The commanded-release brake keeps its4s/1.25m envelope.
 * RT28: a disturbance recovery must not re-capture the position anchor. Logs
 * RT27 (10)(2)/(11)(2) show REC1 entry moving x_ref+0.219->+0.110 and
 * -0.967->-0.943 in one tick, so a front push "returned" to the pushed spot.
 * REC1 entry now brakes with a held anchor (BalanceMotion_BeginBrakeHold) and the
 * brake-completion/RT19 re-anchor window is skipped while that hold is set; a new
 * gesture and a commanded stop still capture their own points.
 * RT27: push handling is the RT21-ANTISTALL design again. RT25/RT26 added a
 * catch that set v_ref:=measured speed and a drive-home assist; the RT26 logs
 * show what that did: with a neutral command v_ref followed the body to-0.87m/s,
 * so the law kept driving the vehicle in the direction it was already rolling
 * (5.3N LESS braking than the plain references) and the vehicle ran away and
 * fell. Both blocks are removed; the anchor and the velocity reference stay
 * where the drive command put them, exactly as in RT21. Push recovery is REC1
 * (50deg/1.2mps/+0.8m travel) plus two later guards that only remove spurious
 * motor cuts: the RT23 200ms speed-interlock debounce and REC1 keeping its
 * priming after a settled exit.
 * RT26: RT25 moved x_ref onto the body while catching, so every push relocated
 * the anchor (logs (5)(3)/(6)(1): x_ref dragged3.66m/4.56m and the vehicle ended
 * wherever the push left it). The catch now holds the anchor it captured at
 * entry, triggers only above80dps/20ms or12deg of lean, REC1's travel allowance
 * past the entry offset is0.35m instead of0.8m, and a bounded drive-home
 * reference (<=0.30m/s,4cm deadband) brings the vehicle back to the anchor when
 * nothing is commanding. RT25: push catch. Under a real push (pitch rate>=60dps for20ms, or a lean of
 *10deg beyond the learned posture) the supervisor hands the frozen R12 law a
 * reference that follows the body (x_ref:=position, v_ref:=measured speed) and
 * drops the yaw request, so only the lean/rate terms act. Logs RT25(3)(4) show
 * the old references asking +1.5N FORWARD while the body fell backwards at
 *0.93m/s, and -9.6N once the references follow the body. REC1 also keeps its
 * priming after a settled exit (a push0.14s after an episode could not re-arm
 * it). No R12 gain, observer, allocator, motion or turn law changed. */
/* RT23: fleet-wide interlock changes only (no R12 gain/observer/allocator edit):
 *  - speed interlock: 200ms debounce, ceiling follows the command(+0.12mps),
 *    hard runaway bound1.45mps. A transient axle overspeed during a balance
 *    catch no longer cuts both motors (that was the fall in every RT22 log).
 *  - turn scaling: wheel cap uses the commanded speed and a per-wheel envelope;
 *    centre-drift taper knee45cm/floor55%. Left and right now hold the same
 *    requested yaw rate. */
#ifndef BALANCE_SPEED_TRIP_TICKS
#define BALANCE_SPEED_TRIP_TICKS 40U /* 200ms at200Hz */
#endif
#ifndef BALANCE_SPEED_OVERRUN_MPS
#define BALANCE_SPEED_OVERRUN_MPS 1.45f
#endif
#ifndef BALANCE_SPEED_TRACK_MARGIN_MPS
#define BALANCE_SPEED_TRACK_MARGIN_MPS 0.12f
#endif
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
/* RT32: bo toc do chot 50/80/90 cm/s. Tran thuong = muc cao nhat 0.90.
 * 0.90 can duty0.879@12.2V /0.900@11.9V /0.955@11.2V (tien, do tu log);
 * duoi ~11.9V governor se ghìm, nen chi chay MAX khi pin >=12.0V (do bang tay). */
#ifndef BALANCE_STOP_SPEED
#define BALANCE_STOP_SPEED 0.90f
#endif
/* RT32: bu lech banh trai. Do tren log xe (RT30 0.86 va RT31 0.90): de cung toc
 * do, banh trai can them ~98 PWM khi tien, ~160 khi lui, ~166 khi quay tai cho
 * (trung binh ~110 PWM =4,4% thang). Day la bu mo-men hang so dat SAU bo phan bo
 * R12 (khong sua gain/quan sat/phan bo), co kep +-MOTOR_MAX_PWM. */
#ifndef BALANCE_TRIM_LEFT_PWM
#define BALANCE_TRIM_LEFT_PWM 110.0f
#endif
#define BALANCE_CAPTURE_SPEED 1.00f // only during bounded startup capture
#define BALANCE_LARGE_CAPTURE_ANGLE 48.0f // preserve S48T speed guard through48deg
#define BALANCE_LARGE_CAPTURE_SPEED 1.10f // only active capture started above48deg
/* The car keeps balancing when the browser is closed: the gateway sends its own
 * heartbeat while balancing, and this 10 s window covers a gateway reboot.
 * Losing the browser only expires the 350 ms move lease (brake + hold), it does
 * not cut the motor. Explicit STOP and the hard protections are unchanged. */
#define BALANCE_HEARTBEAT_MS 10000U

#define BALANCE_EXTREME_CAPTURE_ANGLE 58.0f
#define BALANCE_EXTREME_CAPTURE_SPEED 1.20f
#define BALANCE_CAPTURE_TIMEOUT_MS 3000U
#define BALANCE_EXTREME_CAPTURE_TIMEOUT_MS 4000U
