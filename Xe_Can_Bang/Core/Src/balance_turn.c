/* balance_turn.c — cua và bám hướng: đặt tốc độ quay và trần tốc độ khi cua.
 *
 * Nguyên tắc: đây là lớp QUỸ ĐẠO NGANG, không sinh lực. Nó chỉ đặt w_ref/psi_ref,
 * hạ trần tốc độ dọc khi cua, và quyết định có huỷ lượt cua (kẹt quay) hay không.
 * Toàn bộ thông số ở balance_turn.h để chỉnh một chỗ.
 */
#include "balance_turn.h"

/* Tiến tới target tối đa step (rad/s) trong một nhịp. */
float BalanceTurn_Approach(float old, float target, float step) {
    return old + fmaxf(-step, fminf(step, target - old));
}

/* Hệ số trần cua theo duty còn dư (headroom governor): bánh ≥92 % duty thì hạ
 * dần, hồi khi ≤86 %, sàn 72 %. Chỉ áp dụng khi đang cua để đường thẳng không bị
 * chậm theo. b ≥ 0 để đặt, b < 0 để đọc. */
float BalanceTurn_headroom(float b) {
    static float scale = 1.0f;
    if (b >= 0.0f) scale = b;
    return scale;
}

/* Một nhịp cập nhật hệ số trên từ duty bánh; turning = đang cua. */
float BalanceTurn_headroom_next(float duty, bool turning) {
    float sc = BalanceTurn_headroom(-1.0f);
    if (!turning) return 1.0f;
    if (duty >= BALANCE_TURN_HEADROOM_HI) { if (sc > BALANCE_TURN_HEADROOM_FLOOR) sc -= 0.004f; }
    else if (duty <= BALANCE_TURN_HEADROOM_LO) { if (sc < 1.0f) sc += 0.002f; }
    return sc;
}

/* Một nhịp đầy đủ: chạy quỹ đạo dọc trước (motion), rồi cua.
 * Gọi mỗi 5 ms từ vòng 200 Hz khi đang BALANCING.
 * permission: 0 = cấm, 1 = bình thường, 2/3 = đang REC1. */
int BalanceMotion8_Update(BalanceMotion *s, BalanceTurn *t, unsigned permission, uint32_t word, int turn,
    float position, float pitch, float gyro, float speed, float yaw, float yaw_rate, uint32_t now) {
    if (permission > 3U) return 0;
    bool allow = permission == 1U || permission == 2U;
    bool recovering = permission == 2U || permission == 3U;
    /* REC1 phải phanh giống nhau dù đang giữ nút hay đã nhả. Nhưng KHÔNG lấy lại
     * mốc: xe phải quay về chỗ lệnh cuối để lại, không phải chỗ bị đẩy. */
    if (recovering && !s->recovery_active && s->used && !s->cancel_flags) {
        BalanceMotion_BeginBrakeHold(s, now);
    }
    if (recovering) s->recovery_limited = true;
    s->recovery_active = recovering;
    /* Làm mềm sau REC1 có hạn 1,5 s: giữ trần 0,50 m/s rồi trả lại mốc đã chọn dù
     * vẫn đang giữ nút. */
    if (recovering) s->post_rec_ticks = 0U;
    else if (s->recovery_limited) {
        if (s->post_rec_ticks < 60000U) s->post_rec_ticks++;
        if (s->post_rec_ticks >= 300U) { s->recovery_limited = false; s->post_rec_ticks = 0U; }
    } else s->post_rec_ticks = 0U;
    if (turn < -1 || turn > 1 || !isfinite(yaw_rate) || !isfinite(yaw)
        || !isfinite(t->psi_ref) || !isfinite(t->w_ref)) return 0;
    uint32_t last = word != s->last_word ? now : s->last_ms;
    bool fresh = (word >> 2) > 0U && (uint32_t)(now - last) < BALANCE_MOTION_STALE_MS;
    bool neutral = (word & 3U) == 0U && turn == 0;
    bool linear_neutral = (word & 3U) == 0U;
    if (permission == 1U && fresh && s->ready && !s->release_required && !t->release_required && !s->cancel_flags &&
        ((!linear_neutral && (int)(word & 3U) != (s->requested > 0 ? 1 : s->requested < 0 ? 2 : 0)) ||
         (turn != 0 && turn != t->requested))) s->recovery_limited = false;
    /* Quay tại chỗ bắt đầu từ chỗ đo hiện tại và chỉ chốt MỘT lần. Bám trục mỗi
     * nhịp sẽ xoá hồi tiếp vị trí dọc (một bánh đứng, bánh kia vẽ cung). */
    if (allow && fresh && turn && linear_neutral && s->ready && !s->release_required &&
        !t->release_required && (t->requested == 0 || s->requested != 0)) {
        s->x_ref = position; s->blocked_ticks = 0;
        if (fabsf(s->v_ref) < .01f && fabsf(speed) < .06f) { s->braking = false; s->stop_ticks = 0; }
    }
    /* Nhả cua giữa lúc còn đang dịch chuyển: phanh quỹ đạo dọc tại chỗ đo. */
    if (allow && fresh && neutral && t->requested != 0 && !s->cancel_flags) {
        BalanceMotion_BeginBrake(s, position, now);
    }
    if (t->used && (!fresh || (s->cancel_flags && !s->ready))) t->release_required = true;
    if (fresh && neutral) t->release_required = false;
    /* Khi cua đang giữ nhả, quỹ đạo dọc bị cấm chạy (chỉ giữ thăng bằng). */
    if (!BalanceMotion_Update(s, allow && !t->release_required, word, position, pitch, gyro, speed, now)) return 0;
    bool enabled = allow && s->ready && !s->release_required && !t->release_required && fresh;
    int wanted = enabled ? turn : 0;
    /* Lệnh lái là tốc độ quay đang giữ, không phải tổng góc đã tích. Nhả thì phanh
     * tốc độ quay rồi chốt mốc heading MỚI. */
    if (wanted) { t->braking = false; t->stop_ticks = 0; t->psi_ref = yaw; }
    else if (t->used && t->requested != 0) { t->braking = true; t->stop_ticks = 0; t->psi_ref = yaw; }
    /* Kẹt quay: có lệnh mà bánh không quay suốt 200 nhịp (1 s) thì huỷ cả hai quỹ
     * đạo qua cơ chế huỷ có hạn sẵn có, không đẩy mãi một mô-men. */
    if (wanted && fabsf(t->w_ref) > .35f && fabsf(yaw_rate) < .08f) {
        if (t->blocked_ticks < 200U) t->blocked_ticks++;
    } else t->blocked_ticks = 0;
    if (t->blocked_ticks >= 200U) {
        BalanceMotion_Cancel(s, position, speed, 128U, now);
        s->flags = (s->flags & ~1U) | s->cancel_flags | 16U;
        t->release_required = true; t->blocked_ticks = 0; wanted = 0;
    }
    if (wanted) { t->used = true; s->used = true; s->cancel_flags = 0; s->flags &= (uint16_t)~(64U | 128U); s->flags |= 32U; }
    /* Huỷ lệnh thì bỏ luôn quỹ đạo yaw. Chỉ bị giữ (inhibit) thì giữ mốc heading và
     * thôi xin tốc độ quay, để nút đang giữ chạy tiếp được khi hết giữ. */
    if (t->used && s->cancel_flags) { t->psi_ref = yaw; t->braking = true; }
    float requested_rate = (word & 3U) ? BALANCE_TURN_DIAGONAL_RATE : BALANCE_TURN_PURE_RATE;
    float max_rate = requested_rate;
    if (s->recovery_limited) max_rate = fminf(max_rate, s->recovery_active ? .40f : .80f);
    float envelope = s->speed_preset == 2 ? BALANCE_TURN_WHEEL_ENV_80 :
        s->speed_preset ? BALANCE_TURN_WHEEL_ENV_72 : BALANCE_TURN_WHEEL_ENV_50;
    /* Dung sai 10 cm để độ vọt của vòng kín không bị tính là vượt tốc độ, còn
     * vượt thật vẫn bị chặn. */
    float guard = fmaxf(fabsf(s->v_ref), fabsf(speed) - .10f);
    float wheel_cap = fmaxf(.10f, 2.0f * (envelope - guard) / .262f);
    /* Trần ngang tính theo tốc độ LỆNH, không theo tốc độ đo: nếu theo đo thì
     * trần nhảy qua lại quanh 1,15 rad/s mỗi khi |v| dao động 0,76–0,82. */
#if BALANCE_TURN_LATERAL_FROM_MEASURED
    float lateral_cap = fabsf(speed) > .1f ? BALANCE_TURN_LATERAL_ACCEL / fabsf(speed) : max_rate;
#else
    float lateral_cap = guard > .1f ? BALANCE_TURN_LATERAL_ACCEL / guard : max_rate;
#endif
    max_rate = fminf(max_rate, fminf(wheel_cap, lateral_cap));
    /* Tốc độ quay đặt cũng bị nhân bởi hệ số duty còn dư: bánh yếu chạm trần trước
     * bộ giới hạn, nên cua bão hoà sẽ quay chậm lại thay vì giật. */
    float target = max_rate * BalanceTurn_headroom(-1.0f) * wanted;
    float lead = wanted * (t->psi_ref - yaw);
    float tracking = fmaxf(0, fminf(1, (.50f - lead) / .25f));
    float balance = fmaxf(0, fminf(1, (BALANCE_TURN_LEAN_FULL - fabsf(pitch - t->lean_ref)) / BALANCE_TURN_LEAN_SPAN));
    /* Trôi tâm nhiều thì giảm quyền cua, nhưng phải rộng tay: cú quay tại chỗ tự
     * trôi 15–17 cm mới cân bằng, và chỉ trượt thật (>20 cm trong 1 s) mới bị cắt. */
    float center = linear_neutral ? fmaxf(BALANCE_TURN_CENTER_FLOOR,
        fminf(1, (BALANCE_TURN_CENTER_KNEE - fabsf(s->x_ref - position)) / BALANCE_TURN_CENTER_SPAN)) : 1.0f;
    target *= tracking * balance * center;
    if (wanted && (max_rate < requested_rate - .001f || tracking < .999f || balance < .999f || center < .999f)) s->flags |= 512U;
    float old = t->w_ref;
    float acceleration = (old * target < 0 || fabsf(target) < fabsf(old)) ? BALANCE_TURN_DECEL : BALANCE_TURN_ACCEL;
    /* Neutral bỏ lệnh quay ngay; giảm tốc thật do hồi tiếp tốc độ quay đảm nhiệm,
     * không phải bằng cách chạy nốt ramp góc đã tích. */
    t->w_ref = wanted ? BalanceTurn_Approach(old, target, acceleration * .005f) : 0;
    if (fabsf(t->w_ref) > .0001f) { s->flags |= 2U; if (target == 0) s->flags |= 4U; }
    if (t->used && (wanted || t->braking)) t->psi_ref = yaw;
    if (t->braking) {
        /* Chỉ bám yaw tối đa 0,6 s sau khi nhả rồi đóng băng mốc: truyền động lệch
         * có thể giữ tốc độ quay dư mãi, và bám lâu sẽ trôi mốc suốt đoạn thẳng. */
        if (t->stop_ticks < 60000U) t->stop_ticks++;
        bool stopped = fabsf(yaw_rate) < .08f && fabsf(t->w_ref) < .01f;
        if (stopped || t->stop_ticks >= 120U) {
            t->braking = false; t->w_ref = 0; t->psi_ref = yaw; t->stop_ticks = 0;
        }
    }
    t->rate_mode = wanted ? 1U : t->braking ? 2U : 0U;
    if (t->rate_mode) s->flags |= 2048U;
    if (t->braking) s->flags |= 4U;
    /* Chặn cứng độ lệch heading đang xin. Không bao giờ ghi đè cảm biến/trạng thái. */
    if (t->used) t->psi_ref = yaw + fmaxf(-.60f, fminf(.60f, t->psi_ref - yaw));
    if (t->release_required) s->flags |= 16U;
    /* Kết thúc cước huỷ chỉ khi xe đã yên thật, kể cả khi không có gói neutral nào
     * tới. Giữ cả hai cổng lệnh đóng: không bao giờ chạy tiếp lệnh cũ. */
    if (allow && s->cancel_flags && !s->ready && fabsf(pitch) <= 6 && fabsf(gyro) < 25 &&
        fabsf(speed) < .10f && fabsf(yaw_rate) < .30f && fabsf(s->v_ref) < .01f && fabsf(t->w_ref) < .01f) {
        if (t->quiet < 40U) t->quiet++;
    } else t->quiet = 0;
    if (t->quiet >= 40U) {
        /* 200 ms yên đã đủ xác nhận hồi phục; vẫn phải có neutral mới trước khi
         * lệnh MỚI được chạy. */
        s->cancel_flags = 0; s->cancel_started_ms = 0; s->ready = true; s->stable = 100;
        s->x_ref = position; s->v_ref = 0; s->braking = false; s->stop_ticks = 0;
        s->release_required = true; t->release_required = true;
        t->psi_ref = yaw; t->w_ref = 0; t->braking = false; t->stop_ticks = 0; t->quiet = 0;
        t->rate_mode = 0;
        s->flags = (s->flags & ~(1U | 2U | 4U | 64U | 128U | 1024U | 2048U)) | 16U;
    }
    t->requested = (int8_t)(fresh ? turn : 0);
    /* Đặt trần tốc độ do đường cho nhịp sau (5 ms). Nhân thêm hệ số duty còn dư để
     * bánh không bị ghim 100 % khi cua. Chỉ hạ, không nâng. */
    BalanceMotion_corner_budget(fabsf(t->w_ref) > 0.30f
        ? BalanceTurn_headroom(-1.0f) * fmaxf(0.35f, BALANCE_TURN_VNL_BUDGET - fabsf(t->w_ref) * BALANCE_TURN_HALF_TRACK)
        : 0.0f);
    return isfinite(t->psi_ref) && isfinite(t->w_ref);
}
