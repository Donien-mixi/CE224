/* balance_motion.c — quỹ đạo dọc: giữ mốc vị trí và tốc độ đặt cho vòng cân bằng.
 *
 * Nguyên tắc: đây là lớp QUỸ ĐẠO, không sinh lực. Nó chỉ đặt x_ref/v_ref để lõi
 * R12 bám theo; các chốt an toàn (góc, quãng, tốc độ, REC1) nằm ở lớp trên.
 * Toàn bộ thông số ở balance_motion.h để chỉnh một chỗ.
 * BalanceMotion_Update nằm trong header vì caller chèn hook governor qua macro.
 */
#include "balance_motion.h"
/* ---- Cửa probe cho host test (mặc định tắt, xem tuning.h) ---- */
/* 1 = luật cũ: cắt tốc độ khi lệch mốc quá 20 cm. Chỉ dùng để chứng minh test
 * bắt được lỗi cũ; không bao giờ bật trên xe. */
#define TWIP_PROBE_LEAD_CUT BALANCE_MOTION_LEAD_CUT

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

/* ---- Bản mặc định yếu của hook governor ---- */
/* Nơi không cài hook thì quỹ đạo tự ramp tuyến tính và không neo vị trí — đúng
 * hành vi của nhánh #else cũ. Định nghĩa mạnh ở robot_fsm.c / test / mô phỏng sẽ
 * thay hai hàm này khi liên kết. */
__attribute__((weak)) float BalanceMotion_FilterHook(float old, float target, float rate,
                                                     int wanted, float pitch, float gyro) {
    (void)wanted; (void)pitch; (void)gyro;
    return old + fmaxf(-rate * .005f, fminf(rate * .005f, target - old));
}

__attribute__((weak)) void BalanceMotion_AnchorHook(float *ref, float position, int wanted) {
    (void)ref; (void)position; (void)wanted;
}

__attribute__((weak)) int BalanceMotion_GovernorActive(void) {
    return 0;
}

/* Một nhịp quỹ đạo dọc: đọc lệnh, quyết định phanh/chạy, cập nhật mốc và v_ref.
 * Gọi từ BalanceMotion8_Update (balance_turn.c) mỗi 5 ms khi đang BALANCING.
 * Trả 0 nếu trạng thái/kết quả không hữu hạn. */
int BalanceMotion_Update(BalanceMotion *s,bool allow,uint32_t word,
    float position,float pitch,float gyro,float speed,uint32_t now){
    if(!isfinite(position)||!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed))return 0;
    if(word!=s->last_word){s->last_word=word;s->last_ms=now;}
    unsigned code=word&3U;if(code>2U)return 0;
    int dir=code==1?1:code==2?-1:0;
    bool fresh=(word>>2)>0U&&(uint32_t)(now-s->last_ms)<BALANCE_MOTION_STALE_MS;
    if(!fresh&&dir!=0)s->release_required=true;
    /* Giữ (startup/recovery) chỉ tạm dừng quỹ đạo: mốc bám thân và v_ref về 0,
     * nên vẫn giữ nút thì sau khi hết giữ sẽ chạy tiếp. */
    s->inhibited=!allow;
    /* Lệnh mới bắt đầu từ vị trí hiện tại, không từ mốc của lệnh trước.
     * Neutral bỏ nợ vị trí ngay; chỉ vận tốc được giảm dần. */
    if(allow&&fresh&&s->ready&&!s->release_required&&dir!=0&&dir!=s->requested){
        s->x_ref=position;s->braking=false;s->hold_anchor=false;s->stop_ticks=0;s->blocked_ticks=0;
        /* Nhấn lại bắt đầu từ vận tốc đo được, không từ ramp dở của lệnh trước,
         * để đảo chiều không bị bước nhảy lực. Có governor thì governor tự ramp
         * từ v_ref hiện tại (xem BalanceMotion_GovernorActive). */
        if(!BalanceMotion_GovernorActive())
            s->v_ref=fmaxf(s->speed_preset==2?-1.05f:-.8f,fminf(s->speed_preset==2?1.05f:.8f,speed));
    }
    /* Neutral vài gói liên tiếp là đổi nút, không phải dừng (xem hằng số trên).
     * Lệnh quá tuổi vẫn phanh ngay. held_dir dùng thay requested vì requested đã
     * về 0 ngay gói neutral đầu. */
    bool neutral_now=fresh&&dir==0;
    bool stale_now=!fresh&&dir!=0;
    if(s->used&&!s->cancel_flags&&s->held_dir!=0&&(neutral_now||stale_now)){
        if(stale_now){s->release_ticks=0;s->held_dir=0;BalanceMotion_BeginBrake(s,position,now);}
        else{
            if(s->release_ticks<60000U)s->release_ticks++;
            if(s->release_ticks>=BALANCE_MOTION_RELEASE_HOLD_TICKS){
                s->held_dir=0;BalanceMotion_BeginBrake(s,position,now);
            }
        }
    }else if(fresh&&dir!=0)s->release_ticks=0;
    if(dir!=0)s->held_dir=(int8_t)dir;
    /* Bám encoder để phát hiện chặn đường, không phải đo tường. Bánh quay tại
     * chỗ có thể lọt; rung IMU mạnh đã có chốt riêng. */
    float error=s->x_ref-position;
    /* Ngân sách chạy trước 20 cm, bằng chứng chặn 30 cm: độ trễ đảo chiều không
     * bị coi là chặn vĩnh viễn, còn tường thật vẫn huỷ được. */
    bool lead=dir!=0&&s->used&&!s->cancel_flags&&!s->braking&&dir*error>.30f;
    /* Trục đứng yên có thể lệch mốc vài cm. Tốc độ thấp chỉ là bằng chứng chặn
     * khi đang có lệnh dịch chuyển; vượt mốc 30 cm hoặc overshoot khi quay tại
     * chỗ không tính. */
    /* Ngưỡng bằng chứng theo độ lớn tham chiếu: dưới 0,15 m/s cần 1,5 s; dưới
     * 0,30 m/s cần 1 s; trên đó 300 ms. Quá 30 cm thì 100 ms ở mọi mức. */
    bool stuck=dir!=0&&s->used&&!s->cancel_flags&&!s->braking&&
        dir*error>.06f&&fabsf(speed)<.05f;
    if(allow&&(lead||stuck)){
        s->blocked_ticks+=(uint16_t)(lead?3:1);
        unsigned evidence=lead||fabsf(s->v_ref)>=.30f?60U:dir*s->v_ref>=.15f?200U:300U;
        if(s->blocked_ticks>=evidence)BalanceMotion_Cancel(s,position,speed,128U,now);
    }else s->blocked_ticks=0;
    if(fresh&&dir==0)s->release_required=false;
    // Đang bị giữ: quỹ đạo chỉ chờ trên vị trí đo.
    if(s->cancel_flags&&!s->ready&&(!allow||fabsf(s->v_ref)>.0001f))s->x_ref=position;
    if(allow&&!s->release_required&&!s->cancel_flags){
        /* Lần chạy đầu tiên chỉ cần xe yên thật, không cần quay về vị trí ARM.
         * Không đổi mốc giữ hay lực khi đứng yên: lệnh mới tự lấy mốc của nó. */
        bool anchor_ok=s->used?fabsf(position-s->x_ref)<.08f:dir==0;
        bool quiet=fabsf(pitch)<=5&&fabsf(gyro)<25&&fabsf(speed)<(s->cancel_flags?.10f:.15f)&&fabsf(s->v_ref)<.01f&&anchor_ok;
        if(quiet){if(s->stable<100U)s->stable++;}else s->stable=0;
        if(s->stable>=100U)s->ready=true;
    }
    int wanted=allow&&s->ready&&!s->release_required&&fresh?dir:0;
    /* Trong cửa sổ đổi nút vẫn chạy hướng cũ, nên đảo chiều đi qua 0 thay vì
     * khởi động lại. */
    if(wanted==0&&allow&&s->ready&&!s->release_required&&neutral_now&&
       s->release_ticks>0&&s->release_ticks<BALANCE_MOTION_RELEASE_HOLD_TICKS)
        wanted=s->held_dir;
    if(wanted!=0&&s->braking){s->x_ref=position;s->braking=false;s->hold_anchor=false;s->stop_ticks=0;}
    if(wanted!=0)s->cancel_flags=0;
    bool limited=wanted*(s->x_ref-position)>.20f;
    float speed_limit=s->recovery_limited?fminf(BALANCE_MOTION_SPEED,s->recovery_active?.25f:.50f):BALANCE_MOTION_SPEED;
    /* Ở MAX: bánh đã sát trần duty thì pack không giao nổi 0,90 — nhắm thấp hơn
     * thay vì bão hoà và mất trục. */
    if(s->speed_preset==2)speed_limit*=BalanceMotion_maxhead(-1.0f);
#if TWIP_PROBE_LEAD_CUT
    float target=limited?0:speed_limit*wanted;
#else
    float target=speed_limit*wanted;
    if(limited){
        /* Trục đang chạy mà chậm hơn mốc 20 cm thì neo mốc tại trục và giữ lệnh
         * (luật cũ cắt về 0 rồi ramp lại = khựng). Trục đứng thì vẫn cắt để bộ
         * phát hiện chặn còn tác dụng. */
        if(fabsf(speed)>=.30f)s->x_ref=position+wanted*.20f;
        else target=0;
    }
#endif
    /* Trần do cua: bánh ngoài cần v + |w|·0,131 mà motor chỉ tới ~0,95 m/s, nên
     * ở MAX mà cua gắt thì bánh bão hoà 100 % và yaw vọt. Chỉ hạ, thông qua ramp
     * nên tốc độ đổi mượt. */
    float cb=BalanceMotion_corner_budget(-1.0f);
    if(cb>0.0f){if(target>cb)target=cb;else if(target<-cb)target=-cb;}
    if(wanted!=0)s->used=true;
    float old=s->v_ref;
    float rate=old*target<0?BALANCE_MOTION_REVERSE_DECEL:
        fabsf(target)<fabsf(old)?BALANCE_MOTION_DECEL:BALANCE_MOTION_ACCEL;
    if(s->recovery_limited&&old*target>=0&&fabsf(target)>fabsf(old))rate=fminf(rate,.30f);
    s->v_ref=BalanceMotion_FilterHook(old,target,rate,wanted,pitch,gyro);
    if(s->used&&!s->cancel_flags){
        if(s->braking){
            /* Bỏ đích cũ ngay, rồi thôi dời mốc khi vận tốc đã về 0. Bám trục mãi
             * sẽ vô hiệu hoá hồi tiếp vị trí khi IMU/COM lệch nhẹ. */
            if(fabsf(old)>.0001f&&!s->hold_anchor)s->x_ref=position;
            /* Lượng tử encoder và bias để lại vận tốc dư nhỏ. Chốt mốc mới trước
             * khi đòi đúng 0 m/s; vòng vị trí không đổi sẽ khử nốt trôi. */
            bool stopped=fabsf(speed)<.06f&&fabsf(s->v_ref)<.01f&&fabsf(pitch)<=6&&fabsf(gyro)<25;
            if(stopped){if(s->stop_ticks<20U)s->stop_ticks++;}else s->stop_ticks=0;
            if(s->stop_ticks>=20U){s->braking=false;s->v_ref=0;if(!s->hold_anchor)s->x_ref=position;s->hold_anchor=false;}
        }else if(allow){
            /* Xe còn lăn vài cm sau khi phanh im; chốt mốc ngay lúc đó để lại sai
             * số vị trí vĩnh viễn nên lực đẩy liên tục mà xe không nhích. Chốt
             * lại một lần, 200 ms sau khi phanh im, chỉ trong cửa sổ 20..40 nhịp. */
            if(!s->hold_anchor&&s->stop_ticks>=20U&&s->stop_ticks<40U&&fabsf(s->v_ref)<.0001f&&
               fabsf(speed)<.02f&&fabsf(pitch)<=6&&fabsf(gyro)<20){
                s->stop_ticks++;
                if(s->stop_ticks>=40U)s->x_ref=position;
            }
            /* Đứng tựa vào ma sát: sau 3 s yên thật, mốc trôi về vị trí đo 2 cm/s
             * để vòng vị trí thôi đè lên tường ma sát. Không làm khi đang chạy,
             * đang phanh, đang huỷ hay trong REC1; giới hạn 15 cm. */
            if(!s->braking&&!s->cancel_flags&&!s->recovery_active&&!s->recovery_limited&&
               fabsf(s->v_ref)<.005f&&fabsf(speed)<.02f){
                if(s->settle_ticks<60000U)s->settle_ticks++;
            }else s->settle_ticks=0;
            if(s->settle_ticks>=600U){
                float err=s->x_ref-position;
                if(fabsf(err)<=BALANCE_MOTION_HOLD_SPAN){
                    float step=BALANCE_MOTION_HOLD_RELAX*.005f;
                    s->x_ref=fabsf(err)<=step?position:s->x_ref-copysignf(step,err);
                }
            }
            s->x_ref+=.5f*(old+s->v_ref)*.005f;
        }
        else if(!s->recovery_active)s->x_ref=position;
    }
    if(allow&&wanted&&!s->braking&&!s->cancel_flags)
        BalanceMotion_AnchorHook(&s->x_ref,position,wanted);
    s->requested=(int8_t)(fresh?dir:0);
    s->flags=(s->ready&&!s->release_required?1U:0U)|(fabsf(s->v_ref)>.0001f?2U:0U)|
      (target==0&&fabsf(s->v_ref)>.0001f?4U:0U)|(limited?8U:0U)|
      (s->release_required?16U:0U)|(s->used?32U:0U)|s->cancel_flags|(s->speed_preset?256U:0U)|(s->braking?1028U:0U)|(s->recovery_limited?32768U:0U);
    return isfinite(s->x_ref)&&isfinite(s->v_ref);
}

