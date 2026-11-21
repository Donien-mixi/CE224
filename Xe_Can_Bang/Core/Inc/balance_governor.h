#pragma once
/* balance_governor.h — trần tốc độ/lực: khai báo + state.
 * Thân hàm ở Core/Src/balance_governor.c; thông số ở tuning.h (nhóm 4).
 *
 * Governor là bộ hãm phản hồi: khi duty cạn, thân ngả nhiều hoặc bám trễ, nó hạ
 * gia tốc tham chiếu (không nâng) để xe còn dự trữ cho vòng cân bằng. Nó không
 * đổi luật lực R12 và không sửa số đo.
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "tuning.h"

/* Feedback reference shaper, NOT an invariant-set / formally certified RG.
 * No gains, measurements or operator commands are changed. Called at200Hz. */
typedef struct {
    float acceleration,duty,speed,equilibrium,rest_sum;
    uint16_t cool,saturated,rest_dwell,rest_samples;
    bool equilibrium_ready;
    bool pressure,limited;
} BalanceGovernor;

void  BalanceGovernor_Reset(BalanceGovernor *g);
void  BalanceGovernor_ResetKeepEquilibrium(BalanceGovernor *g);
void  BalanceGovernor_ObserveRest(BalanceGovernor *g,bool eligible,float pitch,float gyro,
                                  float speed,float yaw_rate);
void  BalanceGovernor_Observe(BalanceGovernor *g,int left,int right,float speed);
void  BalanceGovernor_Anchor(const BalanceGovernor *g,float *reference,float position,int direction);
float BalanceGovernor_Step(BalanceGovernor *g,float old,float target,float rate,int wanted,
                           float pitch,float gyro);
