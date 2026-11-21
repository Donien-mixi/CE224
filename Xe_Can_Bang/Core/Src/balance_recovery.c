/* balance_recovery.c — lớp REC1: phát hiện bị đẩy/nhiễu và tự lấy lại thăng bằng.
 *
 * Nguyên tắc: đây là lớp GIÁM SÁT. Nó không sửa luật lực R12, chỉ quyết định
 * "có vào REC1 hay không" và "cho phép xe chạy tiếp hay phải đứng lại".
 * Toàn bộ ngưỡng nằm ở balance_recovery.h để chỉnh một chỗ.
 */
#include "balance_recovery.h"
/* ---- Cửa probe cho host test (mặc định tắt, xem tuning.h) ---- */
/* 1 = vào REC1 ngay khi có lệnh (luật cũ, chưa cần 60 ms bằng chứng). */
#define TWIP_PROBE_IMMEDIATE_MANEUVER BALANCE_RECOVERY_IMMEDIATE_MANEUVER

/* Vào REC1 hay không, xét theo 4 đường:
 *   - nghiêng quá ngưỡng (có lệnh: 16°, không lệnh: 8°)
 *   - gyro ≥ 150 °/s (bị đập)
 *   - sai bám tốc độ ≥ 0,35 m/s (trượt bánh/đẩy mạnh)
 * Khi đang giữ lệnh, bằng chứng phải duy trì 60 ms (12 nhịp 200 Hz) để một cú
 * phanh bình thường không bị coi là mất cân bằng. Khi không có lệnh thì vào ngay.
 */
uint16_t BalanceRecovery_UpdateMode(BalanceRecovery *s, bool allow,
    float pitch, float gyro, float speed, float position, float base_budget,
    uint32_t now, bool maneuver) {
    if (!allow) { BalanceRecovery_Clear(s); return 0; }
    if (!isfinite(pitch) || !isfinite(gyro) || !isfinite(speed)
        || !isfinite(position) || !isfinite(base_budget)) return 512U;

    /* ---- chưa trong REC1: chờ đủ điều kiện rồi bật ---- */
    if (!s->active) {
        /* "primed" = xe đã đứng yên đủ lâu (100 nhịp) để coi là mốc an toàn */
        bool stable = fabsf(pitch) <= 8.0f && fabsf(gyro) < 25.0f && fabsf(speed) < .15f;
        if (stable) { if (s->stable < 100) s->stable++; } else s->stable = 0;
        if (s->stable >= 100) s->primed = true;

        float tilt_trigger = maneuver ? BALANCE_RECOVERY_TILT_MANEUVER : BALANCE_RECOVERY_TILT_IDLE;
        bool suspect = fabsf(pitch) >= tilt_trigger || fabsf(speed) >= .35f;
        bool gyro_spike = fabsf(gyro) >= BALANCE_RECOVERY_GYRO_DPS;

        /* đếm bằng chứng liên tục (12 nhịp = 60 ms) chỉ khi đang có lệnh */
        if (s->primed && maneuver && (suspect || gyro_spike)) { if (s->quiet < 12U) s->quiet++; }
        else s->quiet = 0;

#if TWIP_PROBE_IMMEDIATE_MANEUVER
        bool trigger = s->primed && (gyro_spike || suspect);           /* probe test cũ */
#else
        bool trigger = s->primed && (!maneuver ? (gyro_spike || suspect)
                                              : ((suspect || gyro_spike) && s->quiet >= 12U));
#endif
        if (trigger) {
            s->active = true; s->primed = false; s->quiet = 0; s->started_ms = now;
            if (s->episodes < 65535) s->episodes++;
            /* cho phép đi thêm 0,8 m tính từ chỗ bị đẩy */
            s->distance_limit = fminf(1.0f, fmaxf(base_budget, fabsf(position) + .80f));
        }
    }
    if (!s->active) return 0;

    /* ---- đang trong REC1: ra khi đã yên 300 ms ---- */
    bool quiet = fabsf(pitch) <= 6.0f && fabsf(gyro) < 25.0f
                 && fabsf(speed) < .15f && fabsf(position) <= base_budget;
    if (quiet) s->quiet++; else s->quiet = 0;
    if (s->quiet >= 60) { s->active = false; s->stable = 0; s->quiet = 0; s->primed = true; return 0; }

    /* hạn 8 s: đủ dài cho một lần bị giữ tay, vẫn chặn cuộc lấy lại không bao giờ yên */
    return (uint32_t)(now - s->started_ms) >= BALANCE_RECOVERY_TIMEOUT_MS ? 4096U : 0;
}

/* Bản không có tham số "maneuver" (dùng cho test và cho chỗ không có lệnh). */
uint16_t BalanceRecovery_Update(BalanceRecovery *s, bool allow,
    float pitch, float gyro, float speed, float position, float base_budget, uint32_t now) {
    return BalanceRecovery_UpdateMode(s, allow, pitch, gyro, speed, position,
                                      base_budget, now, false);
}

/* Va đập thật: gyro > 250 °/s kèm gia tốc bất thường (|a| quá lớn hoặc quá nhỏ). */
void BalanceRecovery_Impact(BalanceRecovery *s, bool allow, bool moved,
    float gyro, float norm2, float position, float budget, uint32_t now) {
    if (!allow || !moved || s->active || !isfinite(gyro) || !isfinite(norm2)
        || !isfinite(position) || !isfinite(budget)) return;
    if (fabsf(gyro) > 250.0f && (norm2 > 4.0f || norm2 < .16f)) {
        s->active = true; s->primed = false; s->quiet = 0; s->started_ms = now;
        if (s->episodes < 65535U) s->episodes++;
        s->distance_limit = fminf(1.0f, fmaxf(budget, fabsf(position) + .80f));
    }
}

/* Chặn trước khi interlock cắt motor: chỉ khi bánh THẬT SỰ vượt trần của
 * interlock (max(envelope, |v_ref|+0,12)) và cùng chiều xe đang chạy, duy trì
 * 100 ms. Nhờ vậy REC1 (nới lên 1,20 m/s) bật trước khi motor bị cắt. */
bool BalanceRecovery_OverSpeed(BalanceRecovery *s, bool allow, float speed,
    float reference, float position, float budget, float envelope, uint32_t now) {
    if (!isfinite(speed) || !isfinite(reference) || !isfinite(position)
        || !isfinite(budget) || !isfinite(envelope)) { s->overspeed = 0U; return false; }
    if (!allow || s->active) { s->overspeed = 0U; return false; }

    float ceiling = BalanceInterlock_Ceiling(envelope, reference) - BALANCE_RECOVERY_OVERSPEED_SLACK;
    float error = speed - reference;
    if (fabsf(speed) > ceiling && speed * error > 0.0f) {
        if (s->overspeed < 60000U) s->overspeed++;
    } else s->overspeed = 0U;
    if (s->overspeed < BALANCE_RECOVERY_OVERSPEED_TICKS) return false;

    s->overspeed = 0U; s->active = true; s->primed = false; s->quiet = 0; s->started_ms = now;
    if (s->episodes < 65535U) s->episodes++;
    s->distance_limit = fminf(1.0f, fmaxf(budget, fabsf(position) + .80f));
    return true;
}

/* Cho phép quỹ đạo chạy tiếp trong REC1 hay không:
 * 1 = không có REC1 (chạy bình thường), 2 = REC1 nhưng xe đã yên (chạy chậm),
 * 3 = REC1 và xe chưa yên (không cho chạy, chỉ giữ thăng bằng). */
unsigned BalanceRecovery_DriveMode(const BalanceRecovery *s, bool eligible,
    float pitch, float gyro, float speed, float reference) {
    if (!eligible || !isfinite(pitch) || !isfinite(gyro) || !isfinite(speed)
        || !isfinite(reference)) return 0;
    if (!s->active) return 1;
    return fabsf(pitch) <= 6.0f && fabsf(gyro) <= 25.0f
           && fabsf(speed) <= .35f && fabsf(reference) <= .35f ? 2U : 3U;
}

/* Ba giới hạn nới rộng khi REC1 đang bật (góc/ tốc độ/ quãng). */
float BalanceRecovery_Angle(const BalanceRecovery *s, float ordinary) {
    return s->active ? fmaxf(ordinary, 50.0f) : ordinary;
}
float BalanceRecovery_Speed(const BalanceRecovery *s, float ordinary) {
    return s->active ? fmaxf(ordinary, 1.20f) : ordinary;
}
float BalanceRecovery_Distance(const BalanceRecovery *s, float ordinary) {
    return s->active ? fmaxf(ordinary, s->distance_limit) : ordinary;
}
