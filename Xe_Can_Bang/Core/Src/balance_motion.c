/* balance_motion.c — quỹ đạo dọc: giữ mốc vị trí và tốc độ đặt cho vòng cân bằng.
 *
 * Nguyên tắc: đây là lớp QUỸ ĐẠO, không sinh lực. Nó chỉ đặt x_ref/v_ref để lõi
 * R12 bám theo; các chốt an toàn (góc, quãng, tốc độ, REC1) nằm ở lớp trên.
 * Toàn bộ thông số ở balance_motion.h để chỉnh một chỗ.
 * BalanceMotion_Update nằm trong header vì caller chèn hook governor qua macro.
 */
#include "balance_motion.h"

/* Bắt đầu phanh: neo mốc tại đúng chỗ đo và xoá mọi bằng chứng chặn đường.
 * Gọi khi nhả lệnh, mất lệnh, hoặc khi lớp trên yêu cầu dừng. */
void BalanceMotion_BeginBrake(BalanceMotion *s, float position, uint32_t now) {
    if (!s->braking) { s->cancel_origin = position; s->cancel_started_ms = now; }
    s->braking = true; s->stop_ticks = 0; s->x_ref = position; s->blocked_ticks = 0;
}

/* Bắt đầu phanh do bị đẩy (REC1) mà KHÔNG lấy lại mốc: xe phải quay về chỗ lệnh
 * cuối để lại, không phải chỗ bị đẩy. Gọi từ balance_turn.h khi vào REC1. */
void BalanceMotion_BeginBrakeHold(BalanceMotion *s, uint32_t now) {
    BalanceMotion_BeginBrake(s, s->x_ref, now);
    s->hold_anchor = true;
}

/* Huỷ quỹ đạo đang chạy và mở một cuộc "lấy lại" có hạn (quãng/ thời gian do lớp
 * trên kiểm). Lệnh mới chỉ được nhận sau khi xe yên thật. */
void BalanceMotion_Cancel(BalanceMotion *s, float position, float speed,
                          uint16_t reason, uint32_t now) {
    /* Chỉ chốt mốc huỷ một lần cho mỗi cước; vào lại REC1 không được nới hạn. */
    if ((!s->cancel_flags || s->ready) && !s->braking) {
        s->cancel_origin = position; s->cancel_started_ms = now;
    }
    s->x_ref = position;
    s->v_ref = fmaxf(-.8f, fminf(.8f, speed));
    s->ready = false; s->stable = 0;
    s->release_required = true; s->blocked_ticks = 0;
    s->cancel_flags = 64U | reason;
    s->braking = false; s->stop_ticks = 0;
}

/* Lý do cắt motor do quỹ đạo dọc (0 = không cắt).
 * 512 = NaN; 8 = quá quãng phanh cho phép; 4096 = quá hạn 4 s. */
uint16_t BalanceMotion_Fault(const BalanceMotion *s, float position, uint32_t now) {
    if (!s->braking && (!s->cancel_flags || s->ready)) return 0;
    if (!isfinite(position) || !isfinite(s->cancel_origin)) return 512U;
    /* Phanh thường (nhả lệnh) được 1,25 m; huỷ do chặn/lấy lại chỉ 1,0 m. */
    if (fabsf(position - s->cancel_origin) > (s->braking ? 1.25f : 1.0f)) return 8U;
    /* Phanh do bị đẩy không phải lệnh dừng nên không có hạn hoàn tất: người vẫn
     * đẩy thì trục không bao giờ yên đủ 200 ms và hạn 4 s sẽ cắt oan một xe đang
     * thăng bằng. Quãng 1,25 m ở trên và hạn REC1 vẫn chặn cuộc lấy lại. */
    if (s->braking && s->hold_anchor) return 0;
    return (uint32_t)(now - s->cancel_started_ms) >= 4000U ? 4096U : 0U;
}

/* Va đập cực mạnh: chỉ >900 °/s mới cắt ngay. Va vừa phải do lớp REC1 xử lý để
 * không cắt motor khi thân còn dựng. */
uint16_t BalanceMotion_Impact(const BalanceMotion *s, bool allow, float gyro, float norm2) {
    if (!allow || !s->used) return 0;
    if (!isfinite(gyro) || !isfinite(norm2)) return 512U;
    return fabsf(gyro) > 900.0f ? 8192U : 0U;
}

/* Trần tốc độ do đường đặt (cornering budget): BalanceMotion8_Update ghi giá trị
 * theo |w_ref|; 0 = không giới hạn. Chỉ hạ tham chiếu, không bao giờ nâng, và vẫn
 * đi qua ramp ACCEL/DECEL nên tốc độ đổi mượt thay vì bão hoà bánh.
 * b ≥ 0 để đặt, b < 0 để đọc. */
float BalanceMotion_corner_budget(float b) {
    static float budget = 0.0f;
    if (b >= 0.0f) budget = b;
    return budget;
}

/* Hệ số trần MAX 90 theo duty còn dư ở đường thẳng: bánh ≥96 % duty thì hạ tham
 * chiếu dần (sàn 86 % → 0,774 m/s), hồi lại khi ≤90 %. Pack tươi (0,90 chỉ cần
 * 88 % duty) không bị hạ nên vẫn đạt đúng mốc. Đang cua thì trần cua lo, hệ số
 * này không tác dụng. b ≥ 0 để đặt, b < 0 để đọc. */
float BalanceMotion_maxhead(float b) {
    static float scale = 1.0f;
    if (b >= 0.0f) scale = b;
    return scale;
}

/* Một nhịp cập nhật hệ số trên từ duty đo được; active = đang chạy MAX và không
 * cua. Trả về hệ số mới (đã kẹp trong [FLOOR, 1]). */
float BalanceMotion_maxhead_next(float duty, bool active) {
    float sc = BalanceMotion_maxhead(-1.0f);
    if (!active) return 1.0f;
    if (duty >= BALANCE_MOTION_MAXHEAD_HI) { if (sc > BALANCE_MOTION_MAXHEAD_FLOOR) sc -= 0.004f; }
    else if (duty <= BALANCE_MOTION_MAXHEAD_LO) { if (sc < 1.0f) sc += 0.002f; }
    return sc;
}
