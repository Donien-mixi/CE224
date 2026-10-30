// Exercise production FSM/parser/filter/PID and unchanged R12 law with host hardware mocks.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/r12_host');fs.mkdirSync(out,{recursive:true});
let mocks=fs.readFileSync(path.join(__dirname,'test_hardware_boot.cjs'),'utf8').match(/const mocks = `([\s\S]*?)`;/)[1].replaceAll('\r','');
mocks=mocks.replace('#define RAD_TO_DEG 57.2957795f','').replace(/typedef struct \{ int state;[^\n]+Robot_Data_t;/,'#include "robot_fsm.h"')
 .replace(/typedef struct \{ int emergency_stop[^\n]+ESP32_Command_t;/,'#include "esp32_comm.h"')
 .replace(/enum \{ ROBOT_STATE_INIT[^\n]+\n/,'').replace('LED_PATTERN_FAST_BLINK, SysTick_IRQn','LED_PATTERN_FAST_BLINK,LED_PATTERN_HEARTBEAT, SysTick_IRQn')
 .replace('  if(n==42)','  if(n==sizeof(BalanceFrame)){const BalanceFrame *f=p;assert(f->sync==0x60a5);assert(f->controller==12);wire_frame=*f;return true;}\n  if(n==42)')
 .replace('    else { assert(!strncmp(m,"$TINFO,3,",9));assert(commas==21); }','    else if(strncmp(m,"$BINFO,",7)) { assert(!strncmp(m,"$TINFO,3,",9));assert(commas==21); }');
mocks=mocks.replace('static double encoder_l,encoder_r;','static double encoder_l,encoder_r;\nstatic float startup_pitch;static bool startup_hold,impact_mock;');
mocks=mocks.replace('  return sensor_ok;\n}',`  if(startup_hold){float raw=(BALANCE_RAW_ZERO_DEG-startup_pitch)*DEG_TO_RAD;i->ax=BALANCE_ACC_BX+BALANCE_ACC_SX*sinf(raw);i->az=BALANCE_ACC_BZ+BALANCE_ACC_SZ*cosf(raw);}
  if(impact_mock){i->gy=570;i->ax=2.7f;i->ay=0;i->az=0;}
  return sensor_ok;\n}`);
fs.writeFileSync(path.join(out,'main.h'),'#pragma once\n');
for(const name of ['robot_fsm.h','esp32_comm.h'])fs.writeFileSync(path.join(out,name),fs.readFileSync(path.join(root,'Xe_Can_Bang/Core/Inc',name),'utf8').replace('#include "main.h"','#include <stdint.h>'));
const source=fs.readFileSync(path.join(root,'Xe_Can_Bang/Core/Src/robot_fsm.c'),'utf8').replace(/^#include .+$/gm,'');
const comm=fs.readFileSync(path.join(root,'Xe_Can_Bang/Core/Src/esp32_comm.c'),'utf8');
const parser=comm.slice(comm.indexOf('static void Parse_Packet'),comm.indexOf('void ESP32_Comm_Process')).replaceAll('s_command','command');
const pre=String.raw`
#include "balance_wire.h"
static BalanceFrame wire_frame;
#include "balance_profile.h"
#include "balance_startup.h"
#include "balance_recovery.h"
#include "balance_motion.h"
#include "balance_turn.h"
#include "drive8_protocol.h"
#include "phase2_motion_control.h"
#include "balance_assist.h"
#include "phase1_catalog_control.h"
#include "pid.h"
#include "filter.h"
`;
const stubs=String.raw`
#define MOTOR_MAX_PWM 2499
#define METERS_PER_TICK (0.204203522f/BALANCE_PPR)
static bool cal_ok=true;
bool BMI160_CalibrationOK(void){return cal_ok;}
void Motor_SetDuty(int l,int r){assert(abs(l)<=2499&&abs(r)<=2499);motor_l=l;motor_r=r;}
void Motor_Init(void){Motor_Stop();}
void Motor_SelfTest(void){}
void BuzzerLED_Init(void){} void Buzzer_On(void){}
void Encoder_Init(void){Encoder_Reset();}
void ESP32_Comm_Init(void){} void ESP32_Comm_FlushRx(void){}
bool BMI160_Init(void){return sensor_ok;}
void BMI160_Calibrate_Gyro(int n){assert(n==500);mock_time+=5000;mock_dwt.CYCCNT+=500000000U;}
static int htim4;
void HAL_TIM_Base_Stop_IT(int *p){(void)p;}
void HAL_TIM_Base_Start_IT(int *p){(void)p;}
#define __HAL_TIM_SET_COUNTER(a,b) ((void)(a),(void)(b))
`;
const checks=String.raw`
static void tick(void){mock_time+=5;mock_dwt.CYCCNT+=500000;command.last_cmd_time=mock_time;Robot_ControlLoop_200Hz();Robot_BalanceService();}
static void stand(void){Motor_Stop();s_robot.state=ROBOT_STATE_STANDBY;s_robot.fault=0;command.emergency_stop=0;Filter_Init(0);for(int i=0;i<45;i++)tick();assert(s_robot.ready);}
static void stop(void){Parse_Packet("$STOP");tick();assert(!motor_l&&!motor_r&&s_robot.state!=3);}
static void arm(unsigned profile){char msg[32];snprintf(msg,sizeof(msg),"$ARM,%u",profile);Parse_Packet(msg);tick();assert(s_robot.state==3&&!motor_l&&!motor_r);}
int main(void){
 Robot_Init();assert(bal_controller==12&&Filter_GetAlpha()==.998f);stand();
 // Batched/rapid commands cannot overlap calibration with a motor action.
 Parse_Packet("$ARM,12,20");assert(command.arm_request);
 Parse_Packet("$CALIB");assert(command.trigger_calib&&!command.arm_request);
 Parse_Packet("$PULSE,1,1");Parse_Packet("$ARM,12,21");assert(!command.pulse_request&&!command.arm_request);
 Parse_Packet("$DIAG");assert(!command.diag_request);
 Robot_RecalibrateIMU();assert(!s_robot.ready&&!s_robot.motion_ready&&!command.drive_epoch&&!command.trigger_calib);
 for(int i=0;i<39;i++){tick();assert(!s_robot.ready&&!motor_l&&!motor_r);}
 tick();assert(s_robot.ready);
 Parse_Packet("$PULSE,1,1");Parse_Packet("$ARM,12,22");assert(command.pulse_request&&!command.arm_request);
 Parse_Packet("$STOP");tick();assert(!command.pulse_request&&!command.arm_request&&!motor_l&&!motor_r);
 stand();
 for(const char **p=(const char*[]){"$PROFILE,7","$PROFILE,12","$PROFILE,8","$PROFILE,12x","$PROFILE,7,12","$ARM","$ARM,7","$ARM,8","$ARM,12x",NULL};*p;p++){Parse_Packet(*p);assert(!command.profile_request&&!command.arm_request);}
 command.profile_request=7;tick();assert(bal_controller==12&&!command.profile_request&&s_robot.state!=3);
 arm(12);
 // R12 integration in the real FSM must exactly match frozen native law inputs.
 float expectedState[CAT_N+3];memcpy(expectedState,bal_catalog,sizeof(expectedState));
 tick();int32_t l,r;Encoder_GetHardwareTicks(&l,&r);BMI160_Data_t imu;BMI160_Read_All(&imu);
 float in[7]={s_robot.pitch*DEG_TO_RAD,s_robot.gyro_rate*DEG_TO_RAD,PITCH_ACC_SIGN*Filter_CalibratedRawPitch(imu.ax,imu.az)*DEG_TO_RAD,
   .5f*((float)l-(float)r)*METERS_PER_TICK,s_robot.v_left,s_robot.v_right,(-(float)r-(float)l)*METERS_PER_TICK},o[4];
 assert(cat_step(in,expectedState,o));assert(motor_l==(int)o[0]&&motor_r==(int)o[1]);
 for(unsigned i=0;i<CAT_N+3;i++)assert(fabsf(expectedState[i]-bal_catalog[i])<1e-6f);
 Parse_Packet("$PROFILE,7");assert(!command.profile_request);assert(bal_controller==12);
 stop();for(unsigned i=0;i<CAT_N+3;i++)assert(bal_catalog[i]==0);
 // Legacy coefficient selection cannot change the fixed R12 filter.
 Filter_SelectController(7);assert(Filter_GetAlpha()==.998f);
 stand();
 assert(sizeof(BalanceFrame)==122);
 // Verify 200ms dwell and +/-10 R12 vs original +/-3 R7 with held IMU tilt.
 const float angles[]={-60.1f,-59.5f,-3.1f,-2.9f,2.9f,3.1f,59.5f,60.1f};
 for(unsigned mode=12;mode<=12;mode++){startup_hold=false;stop();
  for(unsigned ai=0;ai<8;ai++){startup_pitch=angles[ai];startup_hold=true;Motor_Stop();s_robot.state=ROBOT_STATE_STANDBY;s_robot.fault=0;bal_still=0;Filter_Init(-startup_pitch);
   for(unsigned j=0;j<39;j++){tick();assert(!s_robot.ready);}tick();
   bool allowed=fabsf(startup_pitch)<=BALANCE_ARM_ANGLE_FOR(mode);assert((bool)s_robot.ready==allowed);
   char msg[32];snprintf(msg,sizeof(msg),"$ARM,%u",mode);Parse_Packet(msg);tick();assert((s_robot.state==3)==allowed);assert(!motor_l&&!motor_r);stop();
  }
 }
 // Actual recalibration path must retain observed tilt instead of declaring it0.
 startup_hold=true;startup_pitch=9;Robot_RecalibrateIMU();assert(fabsf(Filter_GetAngle()-9)<.001f);
 startup_hold=false;stand();
 BalanceStartup probe;
 for(unsigned profile=7;profile<=12;profile+=5){for(int angle=-100;angle<=100;angle++){
   float budget=BalanceStartup_Budget(profile,(float)angle);assert(budget>=.3f&&budget<=1);
   if(profile==7)assert(budget==.3f);
 }}
 BalanceStartup_Begin(&probe,12,35,1000);assert(probe.active&&fabsf(probe.budget-.65f)<1e-6f);
 for(int i=0;i<39;i++)assert(BalanceStartup_Update(&probe,7,0,.1f,1000+i*5)==0&&probe.active);
 assert(!BalanceStartup_Update(&probe,7,0,.1f,1195)&&!probe.active&&probe.angle_limit==25);
 BalanceStartup_Begin(&probe,12,-35,1000);assert(BalanceStartup_Update(&probe,-20,0,0,3999)==0);assert(BalanceStartup_Update(&probe,-20,0,0,4000)==2048);
 BalanceStartup_Begin(&probe,7,2,0);assert(!probe.active&&probe.budget==.3f);
 BalanceStartup_Begin(&probe,12,10,0);assert(!probe.active&&probe.budget==.3f);
 // S60X: capture headroom depends on the angle at ARM and ends on STOP/quiet.
 for(int sign=-1;sign<=1;sign+=2){
  BalanceStartup_Begin(&probe,12,sign*48.0f,0);assert(BalanceStartup_SpeedLimit(&probe)==1.0f);
  BalanceStartup_Begin(&probe,12,sign*48.001f,0);assert(BalanceStartup_SpeedLimit(&probe)==1.10f);
  BalanceStartup_Begin(&probe,12,sign*50.0f,0);assert(BalanceStartup_SpeedLimit(&probe)==1.10f);
  for(int i=0;i<40;i++)BalanceStartup_Update(&probe,0,0,0,i*5);
  assert(!probe.active&&BalanceStartup_SpeedLimit(&probe)==.8f);
  BalanceStartup_Begin(&probe,12,sign*50.0f,0);BalanceStartup_Stop(&probe);assert(BalanceStartup_SpeedLimit(&probe)==.8f);
 }
 BalanceStartup_Begin(&probe,7,2,0);assert(BalanceStartup_SpeedLimit(&probe)==.8f);
 // S60X gate only. Keep the existing budget, tilt and3s capture supervisor.
 for(int sign=-1;sign<=1;sign+=2){
  BalanceStartup_Begin(&probe,12,sign*58.0f,0);
  assert(probe.active&&fabsf(probe.budget-.972f)<1e-6f&&probe.angle_limit==63.0f);
  assert(BalanceStartup_SpeedLimit(&probe)==1.10f);
  assert(BalanceStartup_Update(&probe,sign*58,0,0,3000)==2048);
 }
 // S60X: unchanged policies through58deg; extended thresholds only above58deg.
 for(int sign=-1;sign<=1;sign+=2){
  BalanceStartup_Begin(&probe,12,sign*58.0f,0);assert(BalanceStartup_TimeoutMs(&probe)==3000&&BalanceStartup_SpeedLimit(&probe)==1.10f);
  BalanceStartup_Begin(&probe,12,sign*58.001f,0);assert(BalanceStartup_TimeoutMs(&probe)==4000&&BalanceStartup_SpeedLimit(&probe)==1.20f);
  BalanceStartup_Begin(&probe,12,sign*60.0f,0);assert(probe.budget==1.0f&&probe.angle_limit==65.0f);
  assert(BalanceStartup_Update(&probe,sign*60,0,0,3999)==0);
  assert(BalanceStartup_Update(&probe,sign*60,0,0,4000)==2048);
  BalanceStartup_Begin(&probe,12,sign*60.0f,0);
  for(int i=0;i<40;i++)BalanceStartup_Update(&probe,0,0,0,3100+i*5);
  assert(!probe.active&&BalanceStartup_SpeedLimit(&probe)==.8f);
 }
 for(int angle=-58;angle<=58;angle++){
  BalanceStartup_Begin(&probe,12,(float)angle,0);
  float oldLimit=probe.active?(abs(angle)>48?1.10f:1.00f):.80f;
  assert(BalanceStartup_SpeedLimit(&probe)==oldLimit&&BalanceStartup_TimeoutMs(&probe)==3000);
 }
 // Real FSM at high-angle ARM uses allocated budget and then brakes past it.
 startup_hold=false;stop();startup_hold=true;startup_pitch=34;Filter_Init(-34);bal_still=0;
 for(int i=0;i<45;i++){tick();}
 arm(12);assert(bal_startup.active&&bal_startup.budget>.63f);
 s_robot.x_actual=bal_startup.budget+.01f;tick();assert(s_robot.fault&8);assert(!motor_l&&!motor_r&&!bal_startup.active);
 // Only bounded capture can use the extra speed headroom. No permanent raise.
 startup_hold=false;stand();startup_hold=true;startup_pitch=34;Filter_Init(-34);bal_still=0;
 for(int i=0;i<45;i++){tick();}
 arm(12);motor_l=motor_r=4500;tick();assert(s_robot.state==3&&bal_startup.active);
 bal_startup.active=false;bal_startup.angle_limit=25;startup_hold=false;Filter_Init(0);motor_l=motor_r=4500;tick();assert(s_robot.fault&16);assert(!motor_l&&!motor_r);
 stand();startup_hold=true;startup_pitch=34;Filter_Init(-34);bal_still=0;
 for(int i=0;i<45;i++){tick();}
 arm(12);bal_startup.started_ms=mock_time-3000;tick();assert(s_robot.fault&2048);assert(!motor_l&&!motor_r);
 startup_hold=false;stand();
 // Actual FSM: 1.05m/s still cuts a34deg start; accepts it above48deg, but cuts1.11m/s.
 startup_hold=false;stand();startup_hold=true;startup_pitch=34;Filter_Init(-34);bal_still=0;
 for(int i=0;i<45;i++)tick();
 arm(12);motor_l=motor_r=5250;tick();assert(s_robot.fault&16);assert(!motor_l&&!motor_r);
 startup_hold=false;stand();startup_hold=true;startup_pitch=49.5f;Filter_Init(-49.5f);bal_still=0;
 for(int i=0;i<45;i++)tick();
 arm(12);motor_l=motor_r=5250;tick();assert(s_robot.state==3&&bal_startup.active);
 motor_l=motor_r=5550;tick();assert(s_robot.fault&16);assert(!motor_l&&!motor_r);
 startup_hold=false;stand();
 // Actual FSM: >58deg accepts1.15m/s, still cuts1.22m/s and4s timeout.
 startup_hold=false;stand();startup_hold=true;startup_pitch=59.5f;Filter_Init(-59.5f);bal_still=0;
 for(int i=0;i<45;i++)tick();
 arm(12);motor_l=motor_r=5750;tick();assert(s_robot.state==3&&bal_startup.active);
 motor_l=motor_r=6100;tick();assert(s_robot.fault&16);assert(!motor_l&&!motor_r);
 startup_hold=false;stand();startup_hold=true;startup_pitch=59.5f;Filter_Init(-59.5f);bal_still=0;
 for(int i=0;i<45;i++)tick();
 arm(12);bal_startup.started_ms=mock_time-3100;tick();assert(s_robot.state==3&&!(s_robot.fault&2048));
 bal_startup.started_ms=mock_time-4000;tick();assert(s_robot.fault&2048);assert(!motor_l&&!motor_r);
 startup_hold=false;stand();
 // All original failure cuts remain active under either law.
 for(unsigned mode=12;mode<=12;mode++){stop();stand();arm(mode);command.last_cmd_time=mock_time-(BALANCE_HEARTBEAT_MS+100);mock_time+=5;mock_dwt.CYCCNT+=500000;Robot_ControlLoop_200Hz();assert(s_robot.fault&2);assert(!motor_l&&!motor_r);
   stand();arm(mode);s_robot.x_actual=.31f;tick();assert(s_robot.fault&8);assert(!motor_l&&!motor_r);
   stand();arm(mode);Filter_Init(-35);tick();assert(s_robot.fault&4);assert(!motor_l&&!motor_r);
 }
 // Production recovery path: same controller, no encoder/anchor reset.
 startup_hold=false;stop();stand();arm(12);bal_recovery.primed=true;
 startup_hold=true;startup_pitch=34;Filter_Init(-34);tick();
 assert(s_robot.state==3&&bal_recovery.active&&!bal_startup.active&&bal_startup.budget==.3f);
 s_robot.x_actual=.5f;tick();assert(s_robot.state==3&&s_robot.x_actual>.49f&&bal_recovery.active);
 bal_recovery.started_ms=mock_time-4000;tick();assert((s_robot.fault&4096)&&!motor_l&&!motor_r&&!bal_recovery.active);
 tick();assert(s_robot.state!=3&&!motor_l&&!motor_r); // no automatic re-arm
 startup_hold=false;stand();arm(12);bal_recovery.primed=true;
 startup_hold=true;startup_pitch=51;Filter_Init(-51);tick();assert((s_robot.fault&4)&&!motor_l&&!motor_r);
 startup_hold=false;stand();arm(12);bal_recovery.primed=true;
 startup_hold=true;startup_pitch=34;Filter_Init(-34);tick();s_robot.x_actual=1.01f;tick();assert((s_robot.fault&8)&&!motor_l&&!motor_r);
 startup_hold=false;stand();arm(12);bal_recovery.primed=true;
 startup_hold=true;startup_pitch=34;Filter_Init(-34);tick();
 Parse_Packet("$STOP");tick();assert(!motor_l&&!motor_r&&!bal_recovery.active);
 // Production ARM epoch and atomic, monotonic motion mailbox.
 startup_hold=false;stand();Parse_Packet("$ARM,12,17");tick();assert(s_robot.state==3&&command.drive_epoch==17);
 bal_motion.ready=true;bal_motion.flags=1;s_robot.motion_ready=1;
 Parse_Packet("$MOVE8,1,0,16,1,0");assert(!command.drive_word);
 Parse_Packet("$MOVE8,1,0,17,1,0");assert(command.drive_word==5);tick();assert(bal_motion.used&&bal_motion.v_ref>0&&s_robot.state==3);
 s_robot.x_actual=bal_motion.x_ref=2.0f;tick();assert(s_robot.state==3&&!s_robot.fault); // continuous travel; error stays bounded
 Parse_Packet("$MOVE8,-1,0,17,1,0");assert(command.drive_word==5); // duplicate cannot reverse
 Parse_Packet("$MOVE8,0,0,17,2,0");assert(command.drive_word==8);tick();tick();assert(bal_motion.v_ref==0);
 Parse_Packet("$MOVE8,1,0,17,4294967297,0");assert(command.drive_word==8);
 Parse_Packet("$MOVE8,1,0,17,3,2");assert(command.drive_word==8);
 Parse_Packet("$MOVE8,1,0,17,3,01");assert(command.drive_word==8);
 Parse_Packet("$MOVE8,1,0,17,3,1");assert(command.drive_word==13&&command.drive_speed_preset==1);
 Parse_Packet("$MOVE8,0,1,17,4,0");tick();assert(command.drive_turn==1&&bal_turn.used&&bal_turn.w_ref>0);
 for(int i=0;i<4;i++){tick();}
 assert(wire_frame.drive_sequence==4&&wire_frame.drive_rx_ms==command.drive_rx_ms);
 uint32_t motion_rx=command.drive_rx_ms;mock_time++;Parse_Packet("$PING");assert(command.drive_rx_ms==motion_rx);
 Parse_Packet("$MOVE8,0,-1,17,4,0");assert(command.drive_turn==1); // replay cannot change turn
 Parse_Packet("$STOP");Parse_Packet("$MOVE8,1,0,17,3,0");tick();assert(!command.drive_epoch&&!command.drive_word&&!bal_motion.used&&!motor_l&&!motor_r);
 assert(!command.drive_turn&&!bal_turn.used&&bal_turn.w_ref==0);
 stand();arm(12);s_robot.x_actual=bal_motion.x_ref=2;bal_motion.used=true;impact_mock=true;tick();
 assert(s_robot.state!=3&&s_robot.fault==8192&&!motor_l&&!motor_r&&!command.drive_speed_preset);
 impact_mock=false;tick();assert(s_robot.state!=3&&!motor_l&&!motor_r); // no automatic re-arm
 stand();arm(12);bal_motion.used=true;bal_motion.cancel_flags=64;bal_motion.cancel_origin=0;bal_motion.cancel_started_ms=mock_time;
 s_robot.x_actual=bal_motion.x_ref=1.01f;tick();assert(s_robot.fault==8&&!motor_l&&!motor_r); // moving xref cannot erase the abort-distance guard
 stand();arm(12);bal_motion.used=true;bal_motion.cancel_flags=64;bal_motion.cancel_origin=0;bal_motion.cancel_started_ms=mock_time-4000;
 tick();assert(s_robot.fault==4096&&!motor_l&&!motor_r); // no indefinite cancellation/recovery
 puts("PASS R12 FSM and motion: frozen zero-reference law; startup60/recovery retained; epoch/seq/STOP fences; no automatic ARM; reference advances only after a valid held command.");
 return 0;
}
`;
// C11 has no range for: use simple string list loop.
const file=path.join(out,'dual.c'),exe=path.join(out,'dual.exe');
fs.writeFileSync(file,pre+mocks+stubs+source+parser+checks);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+out,'-I'+path.join(root,'Xe_Can_Bang/Core/Inc'),file,path.join(root,'Xe_Can_Bang/Core/Src/pid.c'),path.join(root,'Xe_Can_Bang/Core/Src/filter.c'),'-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));r=cp.spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr||String(r.error));process.stdout.write(r.stdout);
for(const name of ['phase1_catalog_control.h','phase1_catalog_params.h'])assert.deepEqual(fs.readFileSync(path.join(root,'Xe_Can_Bang/Core/Inc',name)),fs.readFileSync(path.join(root,'Simulation/experiments/motor_datasheet',name)));
assert.deepEqual(fs.readFileSync(path.join(root,'Xe_Can_Bang/Core/Inc/balance_wire.h')),fs.readFileSync(path.join(root,'ESP32_S3_Gateway/balance_wire.h')));
console.log('PASS frozen R12 headers identical to simulation; both board UART headers identical.');
