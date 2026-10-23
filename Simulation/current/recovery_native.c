#include "balance_recovery.h"
#define API __declspec(dllexport)
API void twip_push_guard(double *in,double *state,double *out) {
    BalanceRecovery s={.started_ms=(uint32_t)state[0],.stable=(uint16_t)state[1],
        .quiet=(uint16_t)state[2],.episodes=(uint16_t)state[3],.primed=state[4]!=0,
        .active=state[5]!=0,.distance_limit=(float)state[6]};
    BalanceRecovery_Reserve(&s,in[0]!=0,in[7],in[8],in[4],in[5],(uint32_t)in[6]);
    uint16_t fault=BalanceRecovery_UpdateMode(&s,in[0]!=0,(float)in[1],(float)in[2],
        (float)in[3],(float)in[4],(float)in[5],(uint32_t)in[6],in[9]!=0);
    out[0]=s.started_ms;out[1]=s.stable;out[2]=s.quiet;out[3]=s.episodes;
    out[4]=s.primed;out[5]=s.active;out[6]=s.distance_limit;
    out[7]=BalanceRecovery_Angle(&s,25);out[8]=BalanceRecovery_Speed(&s,.8f);
    out[9]=BalanceRecovery_Distance(&s,(float)in[5]);out[10]=fault;
}
