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

/* Thông số chỉnh được: xem tuning.h (nhóm 2). */

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
