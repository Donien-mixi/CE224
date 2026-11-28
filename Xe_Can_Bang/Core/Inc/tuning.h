#pragma once
/* tuning.h — TOÀN BỘ thông số chỉnh được của xe, một chỗ.
 *
 * Quy ước:
 *  - Giữ nguyên tên macro và #ifndef để host test / mô phỏng override bằng -D.
 *  - Mỗi dòng: #define TÊN giá_trị  // đơn vị — để làm gì.
 *  - Dòng nào ghi "test:" chỉ dùng cho test/mô phỏng, không đổi khi chạy xe.
 *  - Chốt cấp hệ, build ID và luật lực R12 đóng băng nằm ở balance_profile.h
 *    (cố ý tách riêng: sửa nhầm sẽ đổi an toàn hoặc luật cân bằng).
 *  - Ba macro dùng biến của module gọi (s, old) được ghi rõ trong chú thích.
 */

/* ---- 1. Tốc độ và ramp dọc (balance_motion) ---- */
#ifndef BALANCE_MOTION_SPEED
#define BALANCE_MOTION_SPEED (s->speed_preset==2? .90f:s->speed_preset? .80f:.50f)  // tốc độ đặt 50/80/90 cm/s theo mốc (dùng biến s của module gọi)
#endif
#ifndef BALANCE_MOTION_STALE_MS
#define BALANCE_MOTION_STALE_MS 500U  // lệnh quá tuổi này thì coi là mất kết nối và phanh (ms)
#endif
#ifndef BALANCE_MOTION_ACCEL
#define BALANCE_MOTION_ACCEL (s->speed_preset?(fabsf(old)>.50f?.40f:.70f):(fabsf(old)>.50f?.30f:.60f))  // gia tốc tăng tốc (m/s²); dùng s và old của module gọi
#endif
#ifndef BALANCE_MOTION_DECEL
#define BALANCE_MOTION_DECEL 1.20f  // giảm tốc khi nhả lệnh (m/s²) — 28/11: 0.90 -> 1.20 để bớt trôi
#endif
#ifndef BALANCE_MOTION_REVERSE_DECEL
#define BALANCE_MOTION_REVERSE_DECEL 1.80f  // giảm tốc khi đảo chiều (m/s²) — 28/11: 1.0 -> 1.8 để đảo chiều nhanh
#endif
#ifndef BALANCE_MOTION_HOLD_RELAX
#define BALANCE_MOTION_HOLD_RELAX .02f  // nhả mốc khi đứng yên (m/s)
#endif
#ifndef BALANCE_MOTION_HOLD_SPAN
#define BALANCE_MOTION_HOLD_SPAN .15f  // phạm vi được phép nhả mốc (m)
#endif
#ifndef BALANCE_MOTION_RELEASE_HOLD_TICKS
#define BALANCE_MOTION_RELEASE_HOLD_TICKS 8U  // số nhịp 5 ms giữ lệnh cũ khi neutral (8 = 40 ms) — 28/11: 24 -> 8
#endif
#ifndef BALANCE_MOTION_LEAD_CUT
#define BALANCE_MOTION_LEAD_CUT 0  // test: 1 = luật cũ cắt tốc độ khi lệch mốc quá 20 cm
#endif
#ifndef BALANCE_MOTION_MAXHEAD_HI
#define BALANCE_MOTION_MAXHEAD_HI 0.96f  // trần MAX: bắt đầu hạ khi bánh ≥ mức này (duty)
#endif
#ifndef BALANCE_MOTION_MAXHEAD_LO
#define BALANCE_MOTION_MAXHEAD_LO 0.90f  // trần MAX: hồi lại khi bánh ≤ mức này (duty)
#endif
#ifndef BALANCE_MOTION_MAXHEAD_FLOOR
#define BALANCE_MOTION_MAXHEAD_FLOOR 0.86f  // trần MAX: sàn hệ số (0,86 → 0,774 m/s)
#endif

/* ---- 2. Cua và bám hướng (balance_turn) ---- */
#ifndef BALANCE_TURN_PURE_RATE
#define BALANCE_TURN_PURE_RATE (s->speed_preset?2.4f:1.3f)  // tốc độ quay tại chỗ (rad/s)
#endif
#ifndef BALANCE_TURN_DIAGONAL_RATE
#define BALANCE_TURN_DIAGONAL_RATE (s->speed_preset?1.15f:.90f)  // tốc độ quay khi chéo (rad/s)
#endif
#ifndef BALANCE_TURN_ACCEL
#define BALANCE_TURN_ACCEL 4.5f  // gia tốc yaw (rad/s²)
#endif
#ifndef BALANCE_TURN_DECEL
#define BALANCE_TURN_DECEL 8.0f  // giảm tốc yaw (rad/s²)
#endif
#ifndef BALANCE_TURN_LATERAL_ACCEL
#define BALANCE_TURN_LATERAL_ACCEL 0.90f  // trần gia tốc ngang (m/s²)
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_50
#define BALANCE_TURN_WHEEL_ENV_50 0.80f  // trần tốc độ bánh ngoài ở mốc 50 (m/s)
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_72
#define BALANCE_TURN_WHEEL_ENV_72 0.98f  // trần tốc độ bánh ngoài ở mốc 80 (m/s)
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_80
#define BALANCE_TURN_WHEEL_ENV_80 1.05f  // trần tốc độ bánh ngoài ở mốc 90 (m/s)
#endif
#ifndef BALANCE_TURN_VNL_BUDGET
#define BALANCE_TURN_VNL_BUDGET 0.87f  // ngân sách tốc độ khi cua (m/s)
#endif
#ifndef BALANCE_TURN_HALF_TRACK
#define BALANCE_TURN_HALF_TRACK 0.131f  // nửa vết bánh (m) — dùng cho trần bánh ngoài
#endif
#ifndef BALANCE_TURN_CENTER_KNEE
#define BALANCE_TURN_CENTER_KNEE 0.45f  // trôi tâm: còn đủ quyền tới mức này (m)
#endif
#ifndef BALANCE_TURN_CENTER_SPAN
#define BALANCE_TURN_CENTER_SPAN 0.20f  // trôi tâm: giảm hết trong khoảng này (m)
#endif
#ifndef BALANCE_TURN_CENTER_FLOOR
#define BALANCE_TURN_CENTER_FLOOR 0.55f  // trôi tâm: sàn hệ số
#endif
#ifndef BALANCE_TURN_LEAN_FULL
#define BALANCE_TURN_LEAN_FULL 12.0f  // taper nghiêng: đủ quyền tới mức này (°)
#endif
#ifndef BALANCE_TURN_LEAN_SPAN
#define BALANCE_TURN_LEAN_SPAN 4.0f  // taper nghiêng: giảm hết sau mức này (°)
#endif
#ifndef BALANCE_TURN_HEADROOM_HI
#define BALANCE_TURN_HEADROOM_HI 0.92f  // trần cua: bắt đầu hạ ở duty này
#endif
#ifndef BALANCE_TURN_HEADROOM_LO
#define BALANCE_TURN_HEADROOM_LO 0.86f  // trần cua: hồi lại ở duty này
#endif
#ifndef BALANCE_TURN_HEADROOM_FLOOR
#define BALANCE_TURN_HEADROOM_FLOOR 0.72f  // trần cua: sàn hệ số
#endif
#ifndef BALANCE_TURN_LATERAL_FROM_MEASURED
#define BALANCE_TURN_LATERAL_FROM_MEASURED 0  // test: 1 = tính trần ngang theo tốc độ ĐO (luật cũ)
#endif

/* ---- 3. Lấy lại sau va đập — REC1 (balance_recovery) ---- */
#define BALANCE_RECOVERY_LIVE_COMMANDS 1  // cờ đánh dấu cho test/mô phỏng (firmware luôn dùng lệnh trực tiếp)
#ifndef BALANCE_RECOVERY_GYRO_DPS
#define BALANCE_RECOVERY_GYRO_DPS 150.0f  // ngưỡng gyro để coi là bị đập (°/s)
#endif
#ifndef BALANCE_RECOVERY_TILT_MANEUVER
#define BALANCE_RECOVERY_TILT_MANEUVER 16.0f  // ngưỡng nghiêng khi ĐANG giữ lệnh (°)
#endif
#ifndef BALANCE_RECOVERY_TILT_IDLE
#define BALANCE_RECOVERY_TILT_IDLE 8.0f  // ngưỡng nghiêng khi KHÔNG có lệnh (°)
#endif
#ifndef BALANCE_RECOVERY_TIMEOUT_MS
#define BALANCE_RECOVERY_TIMEOUT_MS 8000U  // hạn một cuộc REC1 (ms)
#endif
#ifndef BALANCE_RECOVERY_OVERSPEED_TICKS
#define BALANCE_RECOVERY_OVERSPEED_TICKS 20U  // số nhịp vượt trần liên tiếp (20 = 100 ms)
#endif
#ifndef BALANCE_RECOVERY_OVERSPEED_SLACK
#define BALANCE_RECOVERY_OVERSPEED_SLACK 0.0f  // test: hạ ngưỡng vượt tốc xuống dưới trần (m/s)
#endif
#ifndef BALANCE_RECOVERY_IMMEDIATE_MANEUVER
#define BALANCE_RECOVERY_IMMEDIATE_MANEUVER 0  // test: 1 = vào REC1 ngay khi có lệnh (luật cũ, chưa cần 60 ms bằng chứng)
#endif

/* ---- 4. Governor — trần tốc độ/lực (balance_governor) ---- */
#define BALANCE_GOVERNOR_EQUILIBRIUM 1  // cờ đánh dấu cho test/mô phỏng (firmware luôn học tư thế trung tính)
#ifndef BALANCE_GOV_LEAN_DEG
#define BALANCE_GOV_LEAN_DEG 10.0f  // ngưỡng nghiêng để ép governor (°)
#endif
#ifndef BALANCE_GOV_LEAN_LAUNCH_DEG
#define BALANCE_GOV_LEAN_LAUNCH_DEG 13.0f  // ngưỡng nghiêng khi xuất phát (°)
#endif
#ifndef BALANCE_GOV_CLEAR_DEG
#define BALANCE_GOV_CLEAR_DEG 5.0f  // nghiêng dưới mức này thì nhả pressure (°)
#endif
#ifndef BALANCE_GOV_CLEAR_GYRO
#define BALANCE_GOV_CLEAR_GYRO 30.0f  // gyro dưới mức này thì nhả pressure (°/s)
#endif
#ifndef BALANCE_GOV_LAUNCH_GYRO
#define BALANCE_GOV_LAUNCH_GYRO 45.0f  // gyro nhả pressure khi xuất phát (°/s)
#endif
#ifndef BALANCE_GOV_CRAWL
#define BALANCE_GOV_CRAWL 0.06f  // sàn bò khi pressure (m/s²)
#endif
#ifndef BALANCE_GOV_BRAKE_CAP
#define BALANCE_GOV_BRAKE_CAP .85f  // trần giảm tốc riêng của governor (m/s²)
#endif
#ifndef BALANCE_GOV_REST_DWELL
#define BALANCE_GOV_REST_DWELL 100U  // số nhịp chờ trước khi học tư thế (100 = 0,5 s)
#endif
#ifndef BALANCE_GOV_REST_SAMPLES
#define BALANCE_GOV_REST_SAMPLES 60U  // số mẫu để chốt tư thế trung tính
#endif
#ifndef BALANCE_GOV_ANCHOR_DUTY
#define BALANCE_GOV_ANCHOR_DUTY 0.95f  // chỉ neo mốc khi duty > mức này
#endif

/* ---- 5. Giám sát tốc độ (balance_interlock) ---- */
#ifndef BALANCE_SPEED_TRIP_TICKS
#define BALANCE_SPEED_TRIP_TICKS 40U  // số nhịp vượt trần liên tiếp để cắt (40 = 200 ms)
#endif
#ifndef BALANCE_SPEED_OVERRUN_MPS
#define BALANCE_SPEED_OVERRUN_MPS 1.45f  // vượt mức này thì cắt ngay (m/s)
#endif
#ifndef BALANCE_SPEED_TRACK_MARGIN_MPS
#define BALANCE_SPEED_TRACK_MARGIN_MPS 0.12f  // trần = max(envelope, |v_ref| + mức này)
#endif

/* ---- 6. Giới hạn chạy và bù cơ khí (balance_profile / balance_trim) ---- */
#ifndef BALANCE_STOP_SPEED
#define BALANCE_STOP_SPEED 0.90f  // trần tốc độ thường = mốc cao nhất (m/s)
#endif
#ifndef BALANCE_TRIM_LEFT_PWM
#define BALANCE_TRIM_LEFT_PWM 110.0f  // bù lệch bánh trái (PWM, cộng sau phân bổ R12)
#endif
