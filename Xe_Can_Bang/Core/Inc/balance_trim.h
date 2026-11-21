#pragma once
#include <math.h>
#include "balance_profile.h"
/* Bù lệch cơ khí bánh trái bằng mô-men hằng số theo dấu quay.
 *
 * Đo trên xe: để giữ cùng tốc độ, bánh trái cần thêm ~98 PWM khi tiến, ~160 khi
 * lùi, ~166 khi quay tại chỗ (trung bình ~110 PWM = 4,4 % thang). Nguyên nhân là
 * cơ khí (motor/hộp giảm tốc/ma sát).
 *
 * Đặt SAU bộ phân bổ R12, trước khi ghi PWM — không sửa gain, observer, phân bổ,
 * khung truyền hay giới hạn an toàn nào; kẹp ±max_pwm. Bánh phải không đổi.
 *
 * Trung thực: bù này làm hai bánh cùng tốc độ ở cùng duty, giúp vòng giữ hướng
 * bớt phải giữ offset lớn; nó KHÔNG làm bánh trái khoẻ thêm, nên tốc độ quay tối
 * đa khi bánh trái là bánh ngoài vẫn bị giới hạn bởi vật lý. */
static inline float BalanceTrim_Left(float pwm, float amount, float max_pwm){
    if(!isfinite(pwm))return pwm;
    if(!isfinite(amount)||amount<=0.0f)return pwm;
    if(max_pwm<=0.0f)return pwm;
    if(pwm>0.5f)return fminf(pwm+amount,max_pwm);
    if(pwm<-0.5f)return fmaxf(pwm-amount,-max_pwm);
    return pwm;
}
