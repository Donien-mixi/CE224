#pragma once
#include "balance_motion.h"
/* RT23 SYM2: the left turn lost rate while the right turn held it. Two scaling
 * bugs, not a gain change: (1) the wheel-speed cap used the measured axle speed
 * against the0.80 AXLE ceiling, so preset72 diagonals collapsed to0.10-0.46rad
 * per second and jittered; it now uses the commanded speed and a real per-wheel
 * envelope (50:0.80, 72:0.98, 80:1.05). (2) the centre-drift taper counted the
 *2-3cm/s creep of a normal pivot, which cut the left turn to62-74% once the
 * drift passed12cm; the knee moves from22cm to45cm with a55% floor. Pure-turn
 * and diagonal rates, yaw accel, lateral ceiling, lean taper, blockage detector,
 * heading freeze and the R12 force law are unchanged. */
/* RT22: sharper steering. The operator reported the car turning too gently,
 * especially while translating: a0.55rad/s diagonal at0.8m/s is a1.5m radius.
 * Pure-turn rate is raised slightly and the diagonal rate is now close to the
 * pure rate; the0.4m/s^2 lateral ceiling was the real binding cap at speed, so
 * it moves to0.9 (yaw1.12 at0.8m/s, radius0.7m). Yaw acceleration4.5rad/s^2
 * reaches the requested rate in ~0.5s instead of0.8s. The wheel-speed cap,
 * pitch-balance taper, centre drift taper, blockage detector and the0.6s
 * post-release heading freeze are unchanged. */
#ifndef BALANCE_TURN_PURE_RATE
#define BALANCE_TURN_PURE_RATE (s->speed_preset?2.4f:1.3f)
#endif
#ifndef BALANCE_TURN_LATERAL_FROM_MEASURED
#define BALANCE_TURN_LATERAL_FROM_MEASURED 0
#endif
#ifndef BALANCE_TURN_DIAGONAL_RATE
#define BALANCE_TURN_DIAGONAL_RATE (s->speed_preset?1.15f:.90f)
#endif
#ifndef BALANCE_TURN_ACCEL
#define BALANCE_TURN_ACCEL 4.5f
#endif
#ifndef BALANCE_TURN_DECEL
#define BALANCE_TURN_DECEL 8.0f
#endif
#ifndef BALANCE_TURN_LATERAL_ACCEL
#define BALANCE_TURN_LATERAL_ACCEL 0.90f
#endif
/* RT23: the speed the wheel cap is evaluated against is the COMMANDED axle
 * speed, not the measured one, and the72 wheel envelope is0.98 rather than the
 *0.80 axle ceiling. Evidence: log RT23-8DIR preset72 diagonal held0.51-0.69
 *rad/s against1.15 requested (flag512 63-100% of the gesture) while the same
 * gesture at80 reached1.15. The old rule used the measured0.72-0.79, so the cap
 * was2*(0.80-0.77)/0.262=0.23rad/s and jittered with the loop's own overshoot.
 * A wheel envelope is not an axle ceiling: at preset72 a full diagonal puts the
 * outside wheel at0.72+1.15*0.131=0.87, which the hardware already ran. */
#ifndef BALANCE_TURN_WHEEL_ENV_50
#define BALANCE_TURN_WHEEL_ENV_50 0.80f
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_72
/* RT32: preset 1 la 0.80m/s (truoc la0.72) nhung tran banh GIU NGUYEN 0.98:
 * mot tran banh khong phai tran truc, va o0.80 mot cu cheo1.15rad/s can banh
 * ngoai0.95m/s - con duoi0.98. Nang len nua se chi lam lenh vuot qua VNL
 * (0.92-0.96m/s o pin yeu) ma khong tao duoc momen. */
#define BALANCE_TURN_WHEEL_ENV_72 0.98f
#endif
/* RT34: tran toc do do duong. Banh ngoai khi cua chay v + |w|*0.131; dong co chi
 * toi ~0.95m/s o pack 11.7V, nen o MAX 90 mot cu cua1.15rad/s can banh ngoai
 * 1.05m/s = bat kha thi -> banh bao hoa, yaw vot 37% roi moi hoi (log 38). Vi vay
 * khi dang cua, ha tham chieu toc do con BUDGET - |w|*HALF_TRACK.
 * RT36 (18/11): BUDGET 0.95 -> 0.90. Nguoi van hanh yeu cau cua o 70-75cm/s va
 * hai chieu bang nhau; log fwd_turn cho thay cua 1.15rad/s dat 0.80 thi banh
 * ngoai 0.95 = sat tran, con 0.90-0.151=0.749 thi con du. */
#ifndef BALANCE_TURN_VNL_BUDGET
#define BALANCE_TURN_VNL_BUDGET 0.90f
#endif
#ifndef BALANCE_TURN_HALF_TRACK
#define BALANCE_TURN_HALF_TRACK 0.131f
#endif
#ifndef BALANCE_TURN_WHEEL_ENV_80
#define BALANCE_TURN_WHEEL_ENV_80 1.05f
#endif
/* RT23 centre-drift backstop: full authority until45cm, 55% floor at65cm. */
#ifndef BALANCE_TURN_CENTER_KNEE
#define BALANCE_TURN_CENTER_KNEE 0.45f
#endif
#ifndef BALANCE_TURN_CENTER_SPAN
#define BALANCE_TURN_CENTER_SPAN 0.20f
#endif
#ifndef BALANCE_TURN_CENTER_FLOOR
#define BALANCE_TURN_CENTER_FLOOR 0.55f
#endif
/* RT24: the lean taper must act on the lean BEYOND the learned neutral posture,
 * not on absolute pitch. Log RT23-oncar (BINFO equilibrium+1.074deg, qualified):
 * a full2.40rad/s pivot rides at pitch+6.5deg turning left and -2.2deg turning
 * right (see note), so the old (12-|pitch|)/6 rule cut the LEFT turn to
 *0.65-0.91 of the request (58% of left-turn samples sat below2.38rad/s) while
 * the right turn held2.40 (18%). Full authority now up to8deg of lean, linear
 * cut from8 to12deg, zero beyond. The absolute-pitch tests (REC1 quiet, abort
 * completion) are unchanged: they ask "is the body upright", not "is it turning". */
#ifndef BALANCE_TURN_LEAN_FULL
#define BALANCE_TURN_LEAN_FULL 12.0f
#endif
#ifndef BALANCE_TURN_LEAN_SPAN
#define BALANCE_TURN_LEAN_SPAN 4.0f
#endif
/* Extends SPEED2 motion gating; keeps measurements in the physical ARM frame.
 * Encoder-derived yaw is unwrapped. No roll/slip or global heading sensor. */
typedef struct {float psi_ref,w_ref;float lean_ref;int8_t requested;bool used,release_required;uint16_t blocked_ticks,quiet;
 bool braking;uint16_t stop_ticks;uint8_t rate_mode;} BalanceTurn;
static inline void BalanceTurn_Clear(BalanceTurn *t){*t=(BalanceTurn){0};}
static inline bool BalanceMotion8_Maneuver(const BalanceMotion *s,const BalanceTurn *t,uint32_t now){
 return s->ready&&!s->cancel_flags&&!s->release_required&&!t->release_required&&
    (uint32_t)(now-s->last_ms)<350U&&
    (s->requested!=0||t->requested!=0||s->braking||t->braking);
}
static inline float BalanceTurn_Approach(float old,float target,float step){return old+fmaxf(-step,fminf(step,target-old));}
static inline int BalanceMotion8_Update(BalanceMotion *s,BalanceTurn *t,unsigned permission,uint32_t word,int turn,
 float position,float pitch,float gyro,float speed,float yaw,float yaw_rate,uint32_t now){
 if(permission>3U)return 0;
 bool allow=permission==1U||permission==2U;
 bool recovering=permission==2U||permission==3U;
 if(recovering&&!s->recovery_active&&s->used&&!s->cancel_flags){
  /* REC1 must brake identically whether a finger is held or released.
   * Otherwise a held press follows the axle forever and erases position
   * feedback, while a neutral freezes its braking anchor once vref is zero.
   * RT28: but the anchor itself must NOT be re-captured here. Logs RT27 (10)(2)
   * and (11)(2): a push while standing (no drive command) starts REC1 and this
   * call moved x_ref by10-11 cm in ONE tick, so the vehicle then returned to the
   * pushed position instead of its own. A front push triggers REC1 later (bigger
   * displacement) than a rear push, which is why only that direction looked wrong. */
  BalanceMotion_BeginBrakeHold(s,now);
 }
 if(recovering)s->recovery_limited=true;
 s->recovery_active=recovering;
 /* RT20: bound the post-REC1 softening. The old clear required permission==1 and
  * a direction/turn change, so a press made while REC1 was still active consumed
  * the new-gesture evidence and the vehicle stayed capped at0.50m/s for the rest
  * of the run (log L2: 7s at0.50 with preset80 after a190dps bump). Keep the cap
  * for1.5s after recovery ends, then restore the selected preset even while the
  * same press is still held. */
 if(recovering)s->post_rec_ticks=0U;
 else if(s->recovery_limited){
  if(s->post_rec_ticks<60000U)s->post_rec_ticks++;
  if(s->post_rec_ticks>=300U){s->recovery_limited=false;s->post_rec_ticks=0U;}
 }else s->post_rec_ticks=0U;
 if(turn<-1||turn>1||!isfinite(yaw_rate)||!isfinite(yaw)||!isfinite(t->psi_ref)||!isfinite(t->w_ref))return 0;
 uint32_t last=word!=s->last_word?now:s->last_ms;
 bool fresh=(word>>2)>0U&&(uint32_t)(now-last)<350U;
 bool neutral=(word&3U)==0U&&turn==0;
 bool linear_neutral=(word&3U)==0U;
 if(permission==1U&&fresh&&s->ready&&!s->release_required&&!t->release_required&&!s->cancel_flags&&
    ((!linear_neutral&&(int)(word&3U)!=(s->requested>0?1:s->requested<0?2:0))||
     (turn!=0&&turn!=t->requested)))s->recovery_limited=false;
 /* Capture ONCE when a pure-turn gesture starts. Following the axle every
  * tick erased longitudinal position feedback (log25: left wheel stopped,
  * right wheel drove an arc). Keep the R12 balance loop and a fixed center. */
 if(allow&&fresh&&turn&&linear_neutral&&s->ready&&!s->release_required&&
    !t->release_required&&(t->requested==0||s->requested!=0)){
  s->x_ref=position;s->blocked_ticks=0;
  if(fabsf(s->v_ref)<.01f&&fabsf(speed)<.06f){s->braking=false;s->stop_ticks=0;}
 }
 if(allow&&fresh&&neutral&&t->requested!=0&&!s->cancel_flags){
  BalanceMotion_BeginBrake(s,position,now);
 }
 if(t->used&&(!fresh||(s->cancel_flags&&!s->ready)))t->release_required=true;
 if(fresh&&neutral)t->release_required=false;
 if(!BalanceMotion_Update(s,allow&&!t->release_required,word,position,pitch,gyro,speed,now))return 0;
 bool enabled=allow&&s->ready&&!s->release_required&&!t->release_required&&fresh;
 int wanted=enabled?turn:0;
 /* Held steering commands angular velocity, never a stored total angle.
  * On release brake angular velocity and then freeze a NEW heading anchor. */
 if(wanted){t->braking=false;t->stop_ticks=0;t->psi_ref=yaw;}
 else if(t->used&&t->requested!=0){t->braking=true;t->stop_ticks=0;t->psi_ref=yaw;}
 /* A blocked turn must not keep applying torque indefinitely. Cancel both
  * trajectories through the existing bounded recovery/release mechanism. */
 if(wanted&&fabsf(t->w_ref)>.35f&&fabsf(yaw_rate)<.08f){
  if(t->blocked_ticks<200U)t->blocked_ticks++;
 }else t->blocked_ticks=0;
 if(t->blocked_ticks>=200U){
  BalanceMotion_Cancel(s,position,speed,128U,now);
  s->flags=(s->flags&~1U)|s->cancel_flags|16U;
  t->release_required=true;t->blocked_ticks=0;wanted=0;
 }
 if(wanted){t->used=true;s->used=true;s->cancel_flags=0;s->flags&=(uint16_t)~(64U|128U);s->flags|=32U;}
 /* A cancelled gesture also abandons the yaw trajectory. A plain inhibit keeps
  * the heading anchor and simply stops asking for rate, so the held press can
  * continue when the inhibit ends. */
 if(t->used&&s->cancel_flags){t->psi_ref=yaw;t->braking=true;}
 float requested_rate=(word&3U)?BALANCE_TURN_DIAGONAL_RATE:BALANCE_TURN_PURE_RATE;
 float max_rate=requested_rate;
 if(s->recovery_limited)max_rate=fminf(max_rate,s->recovery_active?.40f:.80f);
 float envelope=s->speed_preset==2?BALANCE_TURN_WHEEL_ENV_80:
     s->speed_preset?BALANCE_TURN_WHEEL_ENV_72:BALANCE_TURN_WHEEL_ENV_50;
 /* A10cm tracking allowance keeps the measured loop overshoot out of the cap
  * while a real overspeed still counts. */
 float guard=fmaxf(fabsf(s->v_ref),fabsf(speed)-.10f);
 float wheel_cap=fmaxf(.10f,2.0f*(envelope-guard)/.262f);
 /* RT36: the lateral ceiling must follow the COMMANDED speed. Using the measured
  * axle made the cap cross1.15rad/s as |v| moved0.76-0.82, so the reverse right
  * turn flickered512 and w_ref1.09-1.15 at50Hz (log rev_turn857-866s:87% of
  * samples limited, w_ref never settled). Same rule as the wheel cap: a
  * command-derived limit, not a measured one. The old rule is kept only for the
  * host-test probe. */
#if BALANCE_TURN_LATERAL_FROM_MEASURED
 float lateral_cap=fabsf(speed)>.1f?BALANCE_TURN_LATERAL_ACCEL/fabsf(speed):max_rate;
#else
 float lateral_cap=guard>.1f?BALANCE_TURN_LATERAL_ACCEL/guard:max_rate;
#endif
 max_rate=fminf(max_rate,fminf(wheel_cap,lateral_cap));
 float target=max_rate*wanted;
 float lead=wanted*(t->psi_ref-yaw);
 float tracking=fmaxf(0,fminf(1,(.50f-lead)/.25f));
 float balance=fmaxf(0,fminf(1,(BALANCE_TURN_LEAN_FULL-fabsf(pitch-t->lean_ref))/BALANCE_TURN_LEAN_SPAN));
 /* A drifting turn center calls for less turn authority, not a false linear
  * blockage cancellation (log26). R12 position feedback keeps its fixed anchor;
  * ordinary/recovery position budgets still provide the independent bound.
  * RT8: log27 left turns sat at~11cm centre drift, so the old16cm knee halved
  * the rate on more than half of the gesture. RT23: the same drift is not a
  * walk-away. A pure pivot leans the body (measured pitch+4.8..+5.9deg turning
  * left, -2.6..-4.2 right), the R12 force loop needs that lean, and the axle
  * creeps2-3cm/s until the position term balances it at15-17cm (log RT23-8DIR
  * 80: drift0.108->0.165 over2s then flat). With the22cm knee that steady
  * creep cut the left turn to center0.74 and the rate faded2.40->1.43rad/s
  * (flag512 77-86%) while right turns held2.40 - the left/right asymmetry the
  * operator reported. The knee moves to45cm with a55% floor, so only a genuine
  * slide (>20cm of new drift in under a second) still reduces the rate. */
 float center=linear_neutral?fmaxf(BALANCE_TURN_CENTER_FLOOR,
     fminf(1,(BALANCE_TURN_CENTER_KNEE-fabsf(s->x_ref-position))/BALANCE_TURN_CENTER_SPAN)):1.0f;
 target*=tracking*balance*center;
 if(wanted&&(max_rate<requested_rate-.001f||tracking<.999f||balance<.999f||center<.999f))s->flags|=512U;
 float old=t->w_ref;
 float acceleration=(old*target<0||fabsf(target)<fabsf(old))?BALANCE_TURN_DECEL:BALANCE_TURN_ACCEL;
 /* Neutral removes the commanded turn now. Physical deceleration is provided
  * by bounded yaw-rate feedback, not by finishing a stored angular ramp. */
 t->w_ref=wanted?BalanceTurn_Approach(old,target,acceleration*.005f):0;
 if(fabsf(t->w_ref)>.0001f){s->flags|=2U;if(target==0)s->flags|=4U;}
 if(t->used&&(wanted||t->braking))t->psi_ref=yaw;
 if(t->braking){
  /* RT19: bound the post-turn follow window. The old rule followed the measured
   * yaw until the rate stayed below0.08rad/s for100ms, with no upper bound. An
   * asymmetric drivetrain can hold a residual rate above that forever, which
   * left the heading anchor chasing the body for entire straight runs
   * (log(13)(2): +59deg in4.9s with dir=+1 and turn=0, flag2048 set100%).
   * Follow for at most0.6s, then freeze the anchor so heading feedback acts. */
  if(t->stop_ticks<60000U)t->stop_ticks++;
  bool stopped=fabsf(yaw_rate)<.08f&&fabsf(t->w_ref)<.01f;
  if(stopped||t->stop_ticks>=120U){
   t->braking=false;t->w_ref=0;t->psi_ref=yaw;t->stop_ticks=0;
  }
 }
 t->rate_mode=wanted?1U:t->braking?2U:0U;
 if(t->rate_mode)s->flags|=2048U;
 if(t->braking)s->flags|=4U;
 /* Hard bound on requested yaw lead. Never overwrite the sensor/state. */
 if(t->used)t->psi_ref=yaw+fmaxf(-.60f,fminf(.60f,t->psi_ref-yaw));
 if(t->release_required)s->flags|=16U;
 /* End an abort envelope only after physically settling, even if no neutral
  * packet arrives. Keep both command gates closed: never resume stale motion.
  * Merely waiting or receiving PING cannot end an unsettled abort. */
 /* RT16: a fresh neutral clears release_required earlier in this function.
  * It must NOT disable physical completion of the independent abort timer.
  * Use the same6deg quiet angle as REC1; do not require return to an abandoned
  * position. Encoder yaw remains bounded by actual wheel quietness. */
 if(allow&&s->cancel_flags&&!s->ready&&fabsf(pitch)<=6&&fabsf(gyro)<25&&
    fabsf(speed)<.10f&&fabsf(yaw_rate)<.30f&&fabsf(s->v_ref)<.01f&&fabsf(t->w_ref)<.01f){
  if(t->quiet<40U)t->quiet++;
 }else t->quiet=0;
 if(t->quiet>=40U){
  /* This200ms dwell already confirms physical recovery. Do not demand a
   * second500ms dwell: log10 stayed locked5s under harmless gyro spikes.
   * Still require a fresh neutral before any NEW command can run. */
  s->cancel_flags=0;s->cancel_started_ms=0;s->ready=true;s->stable=100;
  s->x_ref=position;s->v_ref=0;s->braking=false;s->stop_ticks=0;
  s->release_required=true;t->release_required=true;
  t->psi_ref=yaw;t->w_ref=0;t->braking=false;t->stop_ticks=0;t->quiet=0;
  t->rate_mode=0;
  s->flags=(s->flags&~(1U|2U|4U|64U|128U|1024U|2048U))|16U;
 }
 t->requested=(int8_t)(fresh?turn:0);
 /* RT34: dat tran toc do do duong cho tick sau (5ms). Chi ha, khong nang. */
 BalanceMotion_corner_budget(fabsf(t->w_ref)>0.30f
   ? fmaxf(0.35f,BALANCE_TURN_VNL_BUDGET-fabsf(t->w_ref)*BALANCE_TURN_HALF_TRACK)
   : 0.0f);
 return isfinite(t->psi_ref)&&isfinite(t->w_ref);
}
