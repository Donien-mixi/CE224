#pragma once
/* balance_turn.h — cua và bám hướng: phần KHAI BÁO + THÔNG SỐ.
 * Thân hàm nằm ở Core/Src/balance_turn.c.
 *
 * Lớp này đặt tốc độ quay w_ref và trần tốc độ cho lượt cua rồi giao lại cho quỹ
 * đạo dọc (balance_motion.c) và lõi R12. Nó không sinh lực, không cắt motor.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "balance_motion.h"

/* ---- Thông số chỉnh được ---- */

/* Tốc độ quay đặt (rad/s): quay tại chỗ và chéo, theo mốc tốc độ. */
#ifndef BALANCE_TURN_PURE_RATE
#define BALANCE_TURN_PURE_RATE (s->speed_preset?2.4f:1.3f)
#endif
#ifndef BALANCE_TURN_DIAGONAL_RATE
#define BALANCE_TURN_DIAGONAL_RATE (s->speed_preset?1.15f:.90f)
#endif

/* Chỉ dùng cho test: 1 = tính trần ngang theo tốc độ ĐO (luật cũ). */
#ifndef BALANCE_TURN_LATERAL_FROM_MEASURED
#define BALANCE_TURN_LATERAL_FROM_MEASURED 0
#endif

/* Gia tốc / giảm tốc yaw (rad/s²) và trần gia tốc ngang (m/s²). */
#ifndef BALANCE_TURN_ACCEL
#define BALANCE_TURN_ACCEL 4.5f
#endif
#ifndef BALANCE_TURN_DECEL
#define BALANCE_TURN_DECEL 8.0f
#endif
#ifndef BALANCE_TURN_LATERAL_ACCEL
#define BALANCE_TURN_LATERAL_ACCEL 0.90f
#endif

/* Trần tốc độ bánh ngoài (m/s) theo mốc 50 / 80 / 90. Đây là trần BÁNH, không
 * phải trần trục: bánh ngoài khi cua chạy v + |w|·HALF_TRACK. */
#ifndef BALANCE_TURN_WHEEL_ENV_50
#define BALANCE_TURN_WHEEL_ENV_50 0.80f
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_72
#define BALANCE_TURN_WHEEL_ENV_72 0.98f
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_80
#define BALANCE_TURN_WHEEL_ENV_80 1.05f
#endif

/* Ngân sách tốc độ khi cua: v ≤ BUDGET − |w|·HALF_TRACK
 * (0,87 − 1,15·0,131 = 0,719 m/s, tức 72 cm/s). Chỉ hạ, không nâng. */
#ifndef BALANCE_TURN_VNL_BUDGET
#define BALANCE_TURN_VNL_BUDGET 0.87f
#endif
#ifndef BALANCE_TURN_HALF_TRACK
#define BALANCE_TURN_HALF_TRACK 0.131f
#endif

/* Trôi tâm: đủ quyền tới KNEE (m), giảm tuyến tính trong SPAN, sàn FLOOR.
 * Chỉ cắt khi tâm trôi thật, không cắt vì nhích 2–3 cm/s của cú quay tại chỗ. */
#ifndef BALANCE_TURN_CENTER_KNEE
#define BALANCE_TURN_CENTER_KNEE 0.45f
#endif
#ifndef BALANCE_TURN_CENTER_SPAN
#define BALANCE_TURN_CENTER_SPAN 0.20f
#endif
#ifndef BALANCE_TURN_CENTER_FLOOR
#define BALANCE_TURN_CENTER_FLOOR 0.55f
#endif

/* Taper theo độ nghiêng: đủ quyền tới LEAN_FULL (°) tính từ tư thế trung tính,
 * giảm hết ở LEAN_FULL + SPAN. Dùng độ nghiêng vượt tư thế, không dùng pitch
 * tuyệt đối, để cua trái (thân ngả nhiều hơn) không bị cắt oan. */
#ifndef BALANCE_TURN_LEAN_FULL
#define BALANCE_TURN_LEAN_FULL 12.0f
#endif
#ifndef BALANCE_TURN_LEAN_SPAN
#define BALANCE_TURN_LEAN_SPAN 4.0f
#endif

/* Trần cua theo duty còn dư: bắt đầu hạ ở 92 %, hồi ở 86 %, sàn 72 %. */
#ifndef BALANCE_TURN_HEADROOM_HI
#define BALANCE_TURN_HEADROOM_HI 0.92f
#endif
#ifndef BALANCE_TURN_HEADROOM_LO
#define BALANCE_TURN_HEADROOM_LO 0.86f
#endif
#ifndef BALANCE_TURN_HEADROOM_FLOOR
#define BALANCE_TURN_HEADROOM_FLOOR 0.72f
#endif

/* ---- Trạng thái một lượt cua ---- */

/* psi_ref tính trong khung thân, yaw lấy từ encoder đã mở góc; không có cảm biến
 * heading toàn cục và không mô hình trượt ngang. */
typedef struct {float psi_ref,w_ref;float lean_ref;int8_t requested;bool used,release_required;uint16_t blocked_ticks,quiet;
 bool braking;uint16_t stop_ticks;uint8_t rate_mode;} BalanceTurn;

static inline void BalanceTurn_Clear(BalanceTurn *t){*t=(BalanceTurn){0};}

/* Có đang có "động tác" (lệnh dọc hoặc cua) để lớp trên cho phép nới REC1 không.
 * Dùng bởi balance_startup/robot_fsm khi quyết định ngưỡng nghiêng. */
static inline bool BalanceMotion8_Maneuver(const BalanceMotion *s,const BalanceTurn *t,uint32_t now){
 return s->ready&&!s->cancel_flags&&!s->release_required&&!t->release_required&&
    (uint32_t)(now-s->last_ms)<BALANCE_MOTION_STALE_MS&&
    (s->requested!=0||t->requested!=0||s->braking||t->braking);
}

/* ---- Hàm (thân ở balance_turn.c) ---- */

/* Tiến tới target tối đa step (rad/s) trong một nhịp. */
float BalanceTurn_Approach(float old,float target,float step);

/* Hệ số trần cua theo duty còn dư. b ≥ 0 đặt, b < 0 đọc. */
float BalanceTurn_headroom(float b);

/* Hệ số kế tiếp từ duty bánh; turning = đang cua. */
float BalanceTurn_headroom_next(float duty,bool turning);

/* Một nhịp đầy đủ: quỹ đạo dọc (motion) rồi cua. Gọi mỗi 5 ms khi đang BALANCING.
 * permission: 0 = cấm, 1 = bình thường, 2/3 = đang REC1. Trả 0 nếu trạng thái
 * hoặc kết quả không hữu hạn. */
int BalanceMotion8_Update(BalanceMotion *s,BalanceTurn *t,unsigned permission,uint32_t word,int turn,
 float position,float pitch,float gyro,float speed,float yaw,float yaw_rate,uint32_t now);
