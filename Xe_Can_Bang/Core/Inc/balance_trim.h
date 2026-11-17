#pragma once
#include <math.h>
#include "balance_profile.h"
/* RT32 — bù lệch cơ khí bánh trái.
 *
 * Đo trên log xe thật (RT30 0,86 m/s và RT31 0,90 m/s): để giữ cùng một tốc độ,
 * bánh trái cần thêm ~98 PWM khi tiến, ~160 PWM khi lùi, ~166 PWM khi quay tại
 * chỗ (trung bình ~110 PWM = 4,4 % thang). Nguyên nhân là cơ khí (motor/hộp giảm
 * tốc/ma sát), nên bù bằng một lượng mô-men hằng số theo dấu quay của bánh.
 *
 * Vị trí đặt: SAU bộ phân bổ R12, trước khi ghi PWM — không sửa gain, observer,
 * phân bổ lực, khung truyền hay bất kỳ giới hạn an toàn nào. Kẹp ±max_pwm để
 * không bao giờ vượt thang duty. Bánh phải không đổi.
 *
 * Lưu ý trung thực: bù này làm hai bánh cùng tốc độ ở cùng duty (đúng mô-men),
 * giúp vòng giữ hướng bớt phải giữ một offset lớn; nó KHÔNG làm bánh trái khoẻ
 * thêm, nên tốc độ quay tối đa khi bánh trái là bánh ngoài (rẽ phải khi tiến)
 * vẫn bị giới hạn bởi vật lý. */
static inline float BalanceTrim_Left(float pwm, float amount, float max_pwm){
    if(!isfinite(pwm))return pwm;
    if(!isfinite(amount)||amount<=0.0f)return pwm;
    if(max_pwm<=0.0f)return pwm;
    if(pwm>0.5f)return fminf(pwm+amount,max_pwm);
    if(pwm<-0.5f)return fmaxf(pwm-amount,-max_pwm);
    return pwm;
}
