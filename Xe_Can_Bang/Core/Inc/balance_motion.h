#pragma once
/* balance_motion.h — quỹ đạo dọc: phần KHAI BÁO + THÔNG SỐ.
 * Thân hàm nằm ở Core/Src/balance_motion.c.
 *
 * Lớp này chỉ đặt x_ref/v_ref (và các cờ) cho lõi R12 bám theo. Nó không sinh lực
 * và không cắt motor; mọi chốt an toàn nằm ở lớp trên. Hook governor do caller
 * cài qua hai hàm weak ở cuối tệp.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "tuning.h"

/* Thông số chỉnh được: xem tuning.h (nhóm 1). */

/* ---- Trạng thái quỹ đạo dọc ---- */

/* Chạy ở 200 Hz. Mốc/vận tốc tính trong khung thân xe; một quỹ đạo bị bỏ phải
 * không kéo xe đang bị chặn quay lại đích cũ. */
typedef struct {
    float x_ref,v_ref;
    uint32_t last_word,last_ms;
    uint16_t stable,flags;
    bool ready,used,release_required;
    int8_t limit_direction,requested;
    uint16_t blocked_ticks,cancel_flags;
    bool inhibited,recovery_limited,recovery_active;
    uint8_t speed_preset;
    float cancel_origin;
    uint32_t cancel_started_ms;
    bool braking;
    bool hold_anchor; /* đang phanh do bị đẩy: giữ mốc cũ */
    uint16_t stop_ticks;
    uint16_t post_rec_ticks; /* đếm nhịp làm mềm sau REC1 */
    uint16_t settle_ticks;   /* đếm nhịp đứng yên trước khi nhả mốc */
    uint16_t release_ticks;  /* số gói neutral đã chịu được trước khi phanh */
    int8_t held_dir;         /* hướng lệnh cuối, giữ qua khe neutral */
} BalanceMotion;

static inline void BalanceMotion_Clear(BalanceMotion *s) { *s = (BalanceMotion){0}; }

/* ---- Hàm (thân ở balance_motion.c) ---- */

/* Bắt đầu phanh tại chỗ đo. */
void     BalanceMotion_BeginBrake(BalanceMotion *s, float position, uint32_t now);

/* Bắt đầu phanh do bị đẩy: giữ nguyên mốc để xe quay về đúng chỗ cũ. */
void     BalanceMotion_BeginBrakeHold(BalanceMotion *s, uint32_t now);

/* Huỷ quỹ đạo hiện tại; lệnh mới phải chờ xe yên. */
void     BalanceMotion_Cancel(BalanceMotion *s, float position, float speed,
                              uint16_t reason, uint32_t now);

/* Mã cắt do quỹ đạo dọc: 512 NaN, 8 quá quãng, 4096 quá hạn. */
uint16_t BalanceMotion_Fault(const BalanceMotion *s, float position, uint32_t now);

/* Va đập rất mạnh (>900 °/s) → 8192 (cắt motor); dưới ngưỡng → 0. */
uint16_t BalanceMotion_Impact(const BalanceMotion *s, bool allow, float gyro, float norm2);

/* Trần tốc độ do cua đặt (0 = không giới hạn). b ≥ 0 đặt, b < 0 đọc. */
float    BalanceMotion_corner_budget(float b);

/* Hệ số trần MAX theo duty còn dư. b ≥ 0 đặt, b < 0 đọc. */
float    BalanceMotion_maxhead(float b);

/* Hệ số kế tiếp từ duty bánh trong một nhịp; active = có đang chạy MAX. */
float    BalanceMotion_maxhead_next(float duty, bool active);

/* Một nhịp quỹ đạo dọc. Trả 0 nếu trạng thái/kết quả không hữu hạn.
 * Thân hàm ở balance_motion.c. */
int BalanceMotion_Update(BalanceMotion *s, bool allow, uint32_t word, float position,
                         float pitch, float gyro, float speed, uint32_t now);

/* ---- Hook governor (do caller cài) ---- */
/* robot_fsm.c, các host test và mô phỏng cài hai hàm này để chèn bộ trần tốc
 * độ/lực của governor vào ramp. balance_motion.c có bản mặc định yếu (weak): nơi
 * không cài thì ramp tuyến tính / không neo, đúng như trước đây khi không define
 * macro tương ứng. Định nghĩa mạnh ở nơi khác sẽ thay bản yếu khi liên kết. */
float BalanceMotion_FilterHook(float old, float target, float rate, int wanted,
                               float pitch, float gyro);
void  BalanceMotion_AnchorHook(float *ref, float position, int wanted);
/* 1 = caller đã cài bộ lọc governor. Khi đó lệnh mới KHÔNG reset v_ref về vận tốc
 * đo (governor tự ramp từ v_ref hiện tại); mặc định 0 = ramp tuyến tính. */
int   BalanceMotion_GovernorActive(void);
