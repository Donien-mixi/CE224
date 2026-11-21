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

/* ---- Thông số chỉnh được ---- */

/* Tốc độ đặt theo ba mốc 50 / 80 / 90 cm/s (chỉ dùng trong BalanceMotion_Update). */
#ifndef BALANCE_MOTION_SPEED
#define BALANCE_MOTION_SPEED (s->speed_preset==2? .90f:s->speed_preset? .80f:.50f)
#endif

/* Lệnh quá tuổi này (ms) thì coi là mất kết nối và phanh. */
#ifndef BALANCE_MOTION_STALE_MS
#define BALANCE_MOTION_STALE_MS 500U
#endif

/* Gia tốc tăng tốc (m/s²): mốc cao tăng nhanh hơn, và giảm nửa khi đã quá 0,5 m/s. */
#ifndef BALANCE_MOTION_ACCEL
#define BALANCE_MOTION_ACCEL (s->speed_preset?(fabsf(old)>.50f?.40f:.70f):(fabsf(old)>.50f?.30f:.60f))
#endif

/* Giảm tốc khi nhả lệnh (m/s²). */
#ifndef BALANCE_MOTION_DECEL
#define BALANCE_MOTION_DECEL .90f
#endif

/* Giảm tốc khi đảo chiều (m/s²) — nhanh hơn nhả lệnh để không vọt. */
#ifndef BALANCE_MOTION_REVERSE_DECEL
#define BALANCE_MOTION_REVERSE_DECEL 1.0f
#endif

/* Nhả dần mốc khi đứng yên (m/s) và phạm vi được phép nhả (m). */
#ifndef BALANCE_MOTION_HOLD_RELAX
#define BALANCE_MOTION_HOLD_RELAX .02f
#endif
#ifndef BALANCE_MOTION_HOLD_SPAN
#define BALANCE_MOTION_HOLD_SPAN .15f
#endif

/* Số nhịp 5 ms giữ lệnh cũ khi nút gửi neutral: đổi hướng không bị phanh giữa. */
#ifndef BALANCE_MOTION_RELEASE_HOLD_TICKS
#define BALANCE_MOTION_RELEASE_HOLD_TICKS 24U
#endif

/* Chỉ dùng cho test: 1 = luật cũ (cắt tốc độ khi lệch mốc quá 20 cm). */
#ifndef BALANCE_MOTION_LEAD_CUT
#define BALANCE_MOTION_LEAD_CUT 0
#endif

/* Trần MAX theo duty còn dư: bắt đầu hạ ở 96 %, hồi ở 90 %, sàn 86 %. */
#ifndef BALANCE_MOTION_MAXHEAD_HI
#define BALANCE_MOTION_MAXHEAD_HI 0.96f
#endif
#ifndef BALANCE_MOTION_MAXHEAD_LO
#define BALANCE_MOTION_MAXHEAD_LO 0.90f
#endif
#ifndef BALANCE_MOTION_MAXHEAD_FLOOR
#define BALANCE_MOTION_MAXHEAD_FLOOR 0.86f
#endif

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
