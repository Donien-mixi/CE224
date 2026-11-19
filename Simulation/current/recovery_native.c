#include "balance_recovery.h"
#define API __declspec(dllexport)
/* The100ms overspeed evidence lives in BalanceRecovery.overspeed in the firmware.
 * The Scilab state vector has no free slot, so the harness keeps it in a static
 * that survives the per-tick call. */
static uint16_t harness_overspeed;
API void twip_push_guard(double *in,double *state,double *out) {
    BalanceRecovery s={.started_ms=(uint32_t)state[0],.stable=(uint16_t)state[1],
        .quiet=(uint16_t)state[2],.episodes=(uint16_t)state[3],.primed=state[4]!=0,
        .overspeed=harness_overspeed,
        .active=state[5]!=0,.distance_limit=(float)state[6]};
#if defined(TWIP_RECOVERY_EXTRA) && defined(BALANCE_RECOVERY_LIVE_COMMANDS)
    BalanceRecovery_Impact(&s,in[0]!=0,in[11]!=0,in[2],in[10],in[4],in[5],(uint32_t)in[6]);
    BalanceRecovery_OverSpeed(&s,in[0]!=0,in[7],in[8],in[4],in[5],TWIP_RUNTIME_SPEED_CEILING,(uint32_t)in[6]);
#endif
    harness_overspeed=s.overspeed;
    uint16_t fault=BalanceRecovery_UpdateMode(&s,in[0]!=0,(float)in[1],(float)in[2],
        (float)in[3],(float)in[4],(float)in[5],(uint32_t)in[6],in[9]!=0);
    out[0]=s.started_ms;out[1]=s.stable;out[2]=s.quiet;out[3]=s.episodes;
    out[4]=s.primed;out[5]=s.active;out[6]=s.distance_limit;
    out[7]=BalanceRecovery_Angle(&s,25);out[8]=BalanceRecovery_Speed(&s,.8f);
    out[9]=BalanceRecovery_Distance(&s,(float)in[5]);out[10]=fault;
}
API void twip_recovery_drive_mode(double *in,double *state,double *out){
    BalanceRecovery s={.active=state[5]!=0};
#ifdef BALANCE_RECOVERY_LIVE_COMMANDS
    out[0]=BalanceRecovery_DriveMode(&s,in[0]!=0,in[1],in[2],in[3],in[4]);
#else
    out[0]=in[0]!=0&&!s.active?1:0;
#endif
}
