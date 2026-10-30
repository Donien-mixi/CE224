// Runs the actual diagnostic C implementation with deterministic hardware mocks.
// No hardware access and no flashing. Generated executable stays in .build/.
const fs = require('node:fs');
const path = require('node:path');
const cp = require('node:child_process');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const root = path.resolve(__dirname, '..');
const source = fs.readFileSync(path.join(root, 'Xe_Can_Bang/Core/Src/robot_fsm.c'), 'utf8');
const diagnostic = source.slice(source.indexOf('#define TEST_QUEUE_SIZE'), source.indexOf('void Robot_Init(void)')).replace('static bool bal_config_ok;','').replace('static void BalanceStart_Reset(void);','');
const outDir = path.join(root, '.build/hardware_host');
fs.mkdirSync(outDir, {recursive: true});
const mocks = `
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "hardware_test.h"
#include "wheel_measurement.h"
typedef struct { int state; float pitch,gyro_rate,x_actual,x_target,v_actual,v_left,v_right,v_target,steer_cmd; int16_t pwm_left,pwm_right; float batt_voltage; uint32_t loop_count; } Robot_Data_t;
typedef struct { int emergency_stop,bench_test,trigger_calib; float v_target,steer_cmd; } ESP32_Command_t;
typedef struct { float gx,gy,gz,ax,ay,az; } BMI160_Data_t;
typedef struct { int16_t gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw; } BMI160_RawData_t;
enum { ROBOT_STATE_INIT, ROBOT_STATE_DIAGNOSTIC=8, ROBOT_STATE_TEST_DONE, ROBOT_STATE_TEST_ABORTED };
enum { LED_PATTERN_SLOW_BLINK,LED_PATTERN_ALARM,LED_PATTERN_FAST_BLINK, SysTick_IRQn,TIM4_IRQn };
#define RAD_TO_DEG 57.2957795f
#define CoreDebug_DEMCR_TRCENA_Msk 1
#define DWT_CTRL_CYCCNTENA_Msk 1
#define __DMB() ((void)0)
static struct { uint32_t CYCCNT, CTRL; } mock_dwt;
static struct { uint32_t DEMCR; } mock_debug;
#define DWT (&mock_dwt)
#define CoreDebug (&mock_debug)
static uint32_t SystemCoreClock=100000000, mock_time;
static int motor_l,motor_r,sensor_ok=1,config_ok=1,encoder_ok=1,moving=0,encoder_threshold=300,inverted=0,tilted=0;
static double encoder_l,encoder_r;
static ESP32_Command_t command;
uint32_t HAL_GetTick(void) { return mock_time; }
void HAL_NVIC_SetPriority(int a,int b,int c) { (void)a;(void)b;(void)c; }
void LED_SetPattern(int x) { (void)x; }
void Buzzer_BeepAsync(int x) { (void)x; }
void Motor_Stop(void) { motor_l=motor_r=0; }
static unsigned coast_calls;
void Motor_Coast(void) { motor_l=motor_r=0;coast_calls++; }
void Motor_SetDuty(int l,int r);
ESP32_Command_t *ESP32_Comm_GetCommand(void) { return &command; }
bool BMI160_GetConfig(uint8_t cfg[6]) { uint8_t good[6]={104,209,40,8,40,0}; memcpy(cfg,good,6); if(!config_ok)cfg[3]=3;return sensor_ok; }
void BMI160_GetBias(float b[3]) { b[0]=.1f;b[1]=-.2f;b[2]=.3f; }
bool BMI160_Read_All(BMI160_Data_t *i) {
  *i=(BMI160_Data_t){0,moving==2?130:(moving?5:0),0,inverted?((mock_time/5)%2?.001f:-.001f):0,0,inverted?-1:1};
  if(tilted) { i->ax=.173648f;i->az=.984808f; }
  return sensor_ok;
}
void BMI160_GetLastRaw(BMI160_RawData_t *r) { *r=(BMI160_RawData_t){2,-3,5,0,0,4096}; }
void Encoder_Update(float dt) {
  if(encoder_ok) {
    if(abs(motor_l)>encoder_threshold) encoder_l += (motor_l>0?1:-1)*(abs(motor_l)-encoder_threshold)*2*dt;
    if(abs(motor_r)>encoder_threshold) encoder_r -= (motor_r>0?1:-1)*(abs(motor_r)-encoder_threshold)*2*dt;
  }
}
void Encoder_Reset(void) { encoder_l=encoder_r=0; }
void Encoder_GetHardwareTicks(int32_t *l,int32_t *r) { *l=(int32_t)encoder_l;*r=(int32_t)encoder_r; }
float Encoder_GetLeftVelocity(void) { return motor_l*.0002f; }
float Encoder_GetRightVelocity(void) { return motor_r*.0002f; }
float Encoder_GetAverageVelocity(void) { return (motor_l+motor_r)*.0001f; }
static unsigned sent_frames,sent_meta,wire_job_id,wire_job_frames;
static void (*wire_hook)(void);
bool ESP32_Comm_SendBytes(const void *p,uint16_t n) {
  if(wire_hook){void (*hook)(void)=wire_hook;wire_hook=NULL;hook();}
  if(n==42) { const HardwareTestFrame *f=p; assert(f->sync==0x5aa5); sent_frames++;if(f->stage==13)wire_job_frames++; }
  else { const char *m=p; unsigned commas=0;for(unsigned i=0;i<n;i++)if(m[i]==',')commas++;
    if(!strncmp(m,"$TSTOP,",7))assert(commas==10);
    else if(!strncmp(m,"$TJ,",4)) {
      assert(commas==16);unsigned ver,id,kind,phase,dur,count;int pwm,ref;
      assert(sscanf(m,"$TJ,%u,%u,%u,%u,%u,%d,%d,%u",&ver,&id,&kind,&phase,&dur,&pwm,&ref,&count)==8);
      if(id!=wire_job_id){wire_job_id=id;wire_job_frames=0;}
      if(phase>=2)assert(count==wire_job_frames); /* final marker follows every attempted/sent row */
    }
    else { assert(!strncmp(m,"$TINFO,3,",9));assert(commas==21); }
    sent_meta++;
  }
  return true;
}
`;
const checks = `
static unsigned step_seen[2],manual_pulses[2][8],wheel_pulses[2][2][10];
void Motor_SetDuty(int l,int r) {
  if(manual_phase==1 && (manual_job.kind==5 || manual_job.kind==6)) {
    int values[2]={l,r},previous[2]={motor_l,motor_r};
    int levels[4]={600,1000,1400,1800};
    for(unsigned w=0;w<2;w++)if(values[w]&&!previous[w])
      for(unsigned i=0;i<4;i++)if(abs(values[w])==levels[i])manual_pulses[w][i+(values[w]<0?4:0)]++;
  }
  if(manual_phase==1 && manual_job.kind>=12) {
    const int levels[10]={120,160,180,200,220,260,320,400,300,600};
    int v[2]={l,r},prev[2]={motor_l,motor_r};
    for(unsigned w=0;w<2;w++)if(v[w]&&!prev[w])
      for(unsigned k=0;k<10;k++)if(abs(v[w])==levels[k])wheel_pulses[w][v[w]<0][k]++;
    for(unsigned w=0;w<2;w++)assert(!v[w] || !prev[w] || v[w]*prev[w]>0); /* never instant reversal */
  }
  motor_l=l;motor_r=r;assert(abs(l)<=1800 && abs(r)<=1800);
  if(test_stage==7 || test_stage==8) {
    unsigned wheel=test_stage-7;int pwm=wheel?r:l;
    int levels[4]={600,1000,1400,1800};
    for(unsigned i=0;i<4;i++)if(abs(pwm)==levels[i])step_seen[wheel]|=1U<<(i+(pwm<0?4:0));
    assert(wheel ? l==0 : r==0);
  }
}
static void advance(unsigned ms) { mock_time+=ms;DWT->CYCCNT+=ms*100000; TestSample(); HardwareTest_Service(); }
static void reset_test(void) {
  memset(&s_robot,0,sizeof(s_robot));memset(&command,0,sizeof(command));
  test_head=test_tail=test_stage=test_flags=test_locked=0;test_dropped=0;
  test_seq=test_last_meta_ms=0;memset(test_deadband,0,sizeof(test_deadband));
  mock_time=0;sensor_ok=config_ok=encoder_ok=1;moving=inverted=tilted=0;encoder_threshold=300;Encoder_Reset();Motor_Stop();
  step_seen[0]=step_seen[1]=0;
  memset(wheel_pulses,0,sizeof(wheel_pulses));
  TestBegin(true);
}
static unsigned token=100;
static void start_manual(int kind,int duration,int pwm,int tag) {
  reset_test();TestSetStage(11,mock_time);
  char request[96];snprintf(request,sizeof(request),"$TEST,%u,%d,%d,%d,0,%d",++token,kind,duration,pwm,tag);
  assert(HardwareTest_Command(request) && manual_pending);
  advance(5);assert(manual_phase==1 && test_stage==13);
}
static void finish_manual(void) {
  for(unsigned i=0;i<15000 && manual_phase==1;i++) {
    HardwareTest_Command("$TPING");advance(5);
  }
  assert(manual_phase==2 && test_stage==11 && !motor_l && !motor_r);
  for(unsigned i=0;i<100;i++)advance(5);
  unsigned frozen=test_seq;for(unsigned i=0;i<300;i++)advance(5);assert(test_seq==frozen);
  assert(test_tail==test_head && !test_dropped && manual_announced);
}
static void finish_in_transmit(void){ManualFinish(false,mock_time);}
int main(void) {
  assert(TestCRC((const uint8_t*)"123456789",9)==0x29b1);
  reset_test();
  for(unsigned i=0;i<16000 && test_stage<11;i++) advance(5);
  assert(test_stage==11 && s_robot.state==ROBOT_STATE_TEST_DONE && !motor_l && !motor_r);
  for(unsigned i=0;i<4;i++)assert(test_deadband[i]>=300 && test_deadband[i]<=480);
  command.bench_test=1;command.v_target=1;advance(5);
  assert(HardwareTest_IsLocked() && !motor_l && !motor_r && command.bench_test==0);
  assert(test_dropped==0 && sent_frames>5000 && sent_meta>10);
  assert(test_visited==0x0fff && test_abort_from==255); /* Every nominal stage, no aborted stage */
  assert(step_seen[0]==255 && step_seen[1]==255); /* eight commanded levels per wheel */
  unsigned before_tail=sent_frames;tilted=1;
  for(unsigned i=0;i<400;i++)advance(5);
  assert(sent_frames>before_tail+300 && test_stage==11 && !motor_l && !motor_r);
  /* IMU tilt recording continues after done without restarting motor commands. */
  reset_test();encoder_threshold=1750;
  while(test_stage<7) advance(5);
  assert(test_stage==7 && test_flags==0);
  for(unsigned i=0;i<4;i++)assert(test_deadband[i]==1800); /* regression: final plateau must be evaluated */
  encoder_threshold=300;
  while(test_stage<11)advance(5);
  assert(test_stage==11 && test_visited==0x0fff);
  reset_test();inverted=1;
  for(unsigned i=0;i<16000 && test_stage<11;i++)advance(5);
  assert(test_stage==11 && test_flags==0); /* tiny noise crossing +/-180 is not real chassis motion */
  reset_test();encoder_ok=0;for(unsigned i=0;i<5000 && test_stage<11;i++)advance(5);
  assert(test_stage==12 && (test_flags&16) && !motor_l && !motor_r);
  assert(test_abort_from==3 && test_visited==0x100f);
  reset_test();sensor_ok=0;advance(5);assert(test_stage==12 && (test_flags&8));
  unsigned terminal_time=test_stage_ms;advance(5);assert(test_stage_ms==terminal_time);
  reset_test();command.emergency_stop=1;advance(5);assert(test_stage==12 && (test_flags&4));
  reset_test();while(test_stage<3)advance(5);tilted=1;
  for(unsigned i=0;i<20;i++)advance(5);
  assert(test_stage==3); /* 100 ms transient accel tilt */
  tilted=0;advance(5);tilted=1;
  for(unsigned i=0;i<52;i++)advance(5);
  assert(test_stage==12 && (test_flags&32) && test_abort_from==3 && !motor_l && !motor_r);
  assert(test_stop.stage==3 && test_stop.reason==1 && test_stop.delta>9.9f);
  tilted=0;advance(5);assert(test_stop.delta>9.9f); /* preserve peak after braking */
  command.emergency_stop=1;advance(5);assert(!(test_flags&4));
  reset_test();while(test_stage<7)advance(5);encoder_ok=0;
  for(unsigned i=0;i<145 && test_stage<11;i++)advance(5);
  assert(test_stage==12 && test_abort_from==7 && (test_flags&16));
  reset_test();while(test_stage<9)advance(5);encoder_ok=0;
  for(unsigned i=0;i<145 && test_stage<11;i++)advance(5);
  assert(test_stage==12 && test_abort_from==9 && (test_flags&16));
  reset_test();while(test_stage<10)advance(5);moving=1;
  for(unsigned i=0;i<605 && test_stage<11;i++)advance(5);
  assert(test_stage==12 && test_abort_from==10 && (test_flags&128));
  reset_test();while(test_stage<3)advance(5);moving=2;advance(5);
  assert(test_stage==12 && test_stop.reason==2 && test_stop.gy==130 && !motor_l && !motor_r);
  reset_test();advance(5);advance(5);advance(8);assert(test_stage==12 && (test_flags&2));
  reset_test();moving=1;for(unsigned i=0;i<3100 && test_stage<11;i++)advance(5);
  assert(test_stage==12 && (test_flags&128));
  reset_test();config_ok=0;TestBegin(true);assert(test_stage==12 && (test_flags&64));
  assert(test_abort_from==255 && test_visited==0x1000);
  assert(HardwareTest_Command("$TEST,100,5,1000,600,0,0") && !manual_pending); /* bad sensor range cannot reopen */
  for(int kind=1;kind<=4;kind++) {start_manual(kind,1000,600,0);finish_manual();}
  assert(coast_calls>100);
  start_manual(1,1000,600,0);moving=1;finish_manual();assert(test_flags&128);
  start_manual(2,1000,600,0);tilted=1;finish_manual();assert(!(test_flags&32));
  start_manual(3,1000,600,0);moving=2;finish_manual(); /* intentional tilt is not motor motion */
  memset(manual_pulses,0,sizeof(manual_pulses));
  for(int kind=5;kind<=6;kind++) {start_manual(kind,48000,600,0);finish_manual();}
  for(unsigned w=0;w<2;w++)for(unsigned i=0;i<8;i++)assert(manual_pulses[w][i]==3);
  start_manual(7,36000,600,0);finish_manual();
  start_manual(8,300,-600,2);finish_manual();
  start_manual(9,200,800,2);finish_manual();
  start_manual(11,300,-600,0);unsigned coast_before=coast_calls;finish_manual();assert(coast_calls>coast_before+500);
  /* All twelve prepared jobs execute the real ISR/service implementation. */
  for(int kind=12;kind<=14;kind++)for(int wheel=0;wheel<3;wheel++)for(int dir=0;dir<(kind==13?2:1);dir++) {
    int duration=kind==12?18000:kind==13?36000:12900;
    start_manual(kind,duration,kind==12?600:kind==13?(dir?-400:400):300,wheel);
    unsigned start=test_start_ms;finish_manual();
    assert(manual_leg_ms==start+5000 && !test_flags && test_seq<14000);
    for(unsigned w=0;w<2;w++)for(unsigned d=0;d<2;d++)for(unsigned k=0;k<10;k++) {
      unsigned expected=0;
      bool chosen=wheel==2 || w==(unsigned)wheel;
      if(chosen && ((kind==12 && k==9)||(kind==14 && k==8)||(kind==13 && k<8 && d==(unsigned)dir)))expected=3;
      assert(wheel_pulses[w][d][k]==expected);
    }
  }
  start_manual(13,36000,400,2);encoder_ok=0;finish_manual();assert(!test_flags); /* no movement at low PWM is valid data */
  start_manual(12,18000,600,2);encoder_ok=0;
  while(manual_phase==1){HardwareTest_Command("$TPING");advance(5);}
  assert(manual_phase==3 && (test_flags&16) && !motor_l && !motor_r);
  start_manual(14,12900,300,2);command.emergency_stop=1;advance(5);assert(manual_phase==3 && (test_flags&4));
  reset_test();TestSetStage(11,mock_time);HardwareTest_Command("$TEST,123,13,36000,400,0,2");
  command.emergency_stop=1;advance(5);assert(!manual_pending && manual_phase==0 && !motor_l && !motor_r);
  reset_test();TestSetStage(11,mock_time);
  const char *invalid_wheel[]={"$TEST,123,12,18001,600,0,2","$TEST,123,13,36000,401,0,2","$TEST,123,14,12900,300,1,2","$TEST,123,12,18000,600,0,3"};
  for(unsigned i=0;i<4;i++){HardwareTest_Command(invalid_wheel[i]);assert(!manual_pending);}
  puts("PASS: all 12 wheel jobs; exact three repetitions/direction/level, isolated wheel mapping, brake before reversal, low-PWM no-motion retained, +/-600 encoder cut, STOP cancels queued test, strict waveform bounds, capacity");
  /* Two jobs in one boot, a changed pose, and completion preempting a start
   * marker transmission. No sample may follow a terminal marker. */
  start_manual(1,1000,600,0);tilted=1;finish_manual();tilted=0;
  char next_request[96];snprintf(next_request,sizeof(next_request),"$TEST,%u,5,48000,600,0,0",++token);
  HardwareTest_Command(next_request);advance(5);assert(manual_phase==1);finish_manual();assert(test_flags==0);
  start_manual(1,1000,600,0);manual_announced=0;wire_hook=finish_in_transmit;HardwareTest_Service();
  for(unsigned i=0;i<100;i++)advance(5);
  assert(manual_phase==2 && manual_published_phase==2 && wire_job_frames==test_seq);
  for(int wheel=0;wheel<2;wheel++) {
    start_manual(10,45000,600,wheel);finish_manual();
    for(unsigned i=0;i<6;i++)assert(manual_db[i]>300 && manual_db[i]<=400);
  }
  start_manual(5,48000,600,0);
  for(unsigned i=0;i<302;i++)advance(5);
  assert(manual_phase==3 && (test_flags&4) && !motor_l && !motor_r); /* browser heartbeat lost */
  start_manual(8,300,600,0);command.emergency_stop=1;advance(5);
  assert(manual_phase==3 && (test_flags&4));
  start_manual(5,48000,600,0);sensor_ok=0;advance(5);assert(manual_phase==3 && (test_flags&8));
  start_manual(5,48000,600,0);advance(5);advance(5);advance(8);assert(manual_phase==3 && (test_flags&2));
  start_manual(5,48000,600,0);encoder_ok=0;
  for(unsigned i=0;i<1300 && manual_phase==1;i++){HardwareTest_Command("$TPING");advance(5);}
  assert(manual_phase==3 && (test_flags&16));
  start_manual(5,48000,600,0);
  for(unsigned i=0;i<1000;i++){HardwareTest_Command("$TPING");advance(5);}
  moving=2;HardwareTest_Command("$TPING");advance(5);
  assert(manual_phase==3 && (test_flags&32) && test_stop.gy==130 && !motor_l && !motor_r);
  reset_test();TestSetStage(11,mock_time);
  const char *bad[]={"$TEST,100,9,201,800,0,0","$TEST,100,9,100,801,0,0","$TEST,100,8,701,800,0,0","$TEST,100,1,100,99999999999999999,0,0","$TEST,100,1,100,-2147483648,0,0","$TEST,100,10,100,600,0,2","$TEST,100,1,100,600,0,0junk"};
  for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){assert(HardwareTest_Command(bad[i]));assert(!manual_pending);}
  puts("PASS: manual jobs 1-11, 3 repetitions/direction/level, coast encoder/pulse, stillness quality, fixed final sample count, config/pulse bounds, heartbeat/STOP/I2C/timing/encoder/motion interlocks");
  puts("PASS: production C all stages 0-11, 8 step levels per wheel, final 1800 deadband, +/-180 angle wrap, STOP/fault abort IDs, finish lock, CRC and metadata");
  return 0;
}
`;
const cfile = path.join(outDir, 'hardware_host.c');
const exe = path.join(outDir, 'hardware_host.exe');
fs.writeFileSync(cfile, mocks + diagnostic + checks);
const gcc = process.env.HOST_CC || 'C:/msys64/ucrt64/bin/gcc.exe';
let result = cp.spawnSync(gcc, ['-std=c11','-Wall','-Wextra','-Werror', '-I'+path.join(root,'Xe_Can_Bang/Core/Inc'),cfile,'-lm','-o',exe], {encoding:'utf8'});
if (result.status !== 0) throw new Error(result.stderr || result.error || 'Host compile failed');
result = cp.spawnSync(exe, [], {encoding:'utf8'});
process.stdout.write(result.stdout || '');
if (result.status !== 0) throw new Error(result.stderr || result.error || 'Host test failed');
// Execute the actual ESP32 receiver/CRC/capture code with a mocked UART stream.
const gateway = fs.readFileSync(path.join(root,'ESP32_S3_Gateway/ESP32_S3_Gateway.ino'),'utf8');
const receiver = gateway.slice(gateway.indexOf('void receiveSTM32() {'));
const capture = gateway.slice(gateway.indexOf('struct __attribute__'), gateway.indexOf('void downloadHardwareLog()'));
const lineBuffer = gateway.match(/#define STM32_LINE_MAX[\s\S]*?uint8_t stm32LineIdx = 0;/)[0];
const gatewayMock = `
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <deque>
#include <vector>
#define ARDUINO 1
#define portMAX_DELAY 0xffffffffU
#define pdMS_TO_TICKS(x) (x)
typedef int SemaphoreHandle_t;
static bool captureHeld=false;
static void xSemaphoreTake(SemaphoreHandle_t h,unsigned) { assert(h && !captureHeld);captureHeld=true; }
static void xSemaphoreGive(SemaphoreHandle_t h) { assert(h && captureHeld);captureHeld=false; }
static void vTaskDelay(unsigned) {}
static uint32_t millis() { return 10000; }
static struct { std::deque<uint8_t> bytes; int available(){return bytes.size();} int read(){int b=bytes.front();bytes.pop_front();return b;} } Serial1;
static struct { unsigned messages=0; void broadcastTXT(const char*){messages++;} } webSocket;
`;
const gatewayChecks = `
static void feedBalance(BalanceFrame f,bool corrupt=false,bool remove=false) {
  f.sync=0x5ea5;if(!f.controller)f.controller=12;f.crc=testCRC((uint8_t*)&f,sizeof(f)-2);
  auto p=(uint8_t*)&f;if(corrupt)p[40]^=0x20;
  for(unsigned i=0;i<sizeof(f);i++)if(!remove||i!=60)Serial1.bytes.push_back(p[i]);
}
static void feed(TestFrame f, bool corrupt=false, bool remove=false) {
  f.sync=0x5aa5; f.crc=testCRC((const uint8_t*)&f,sizeof(f)-2);
  auto p=(uint8_t*)&f;if(corrupt)p[20]^=0x10;
  for(unsigned i=0;i<sizeof(f);i++)if(!remove || i!=30)Serial1.bytes.push_back(p[i]);
}
int main() {
  captureMutex=1;assert(captureTask!=nullptr);
  char storage[8192];testLog=storage;testCapacity=sizeof(storage);
  TestFrame f={};f.dt_us=5000;f.az=4096;f.gx=0x5aa5;f.flags=1;f.pwm_l=600;f.ticks_l=12;f.ticks_r=-12;
  assert(testCRC((const uint8_t*)"123456789",9)==0x29b1);
  feed(f);receiveSTM32();assert(testSamples==1 && testSequenceGaps==0 && testCRCError==0);
  assert(strstr(testLatest,",600,0,12,-12,") && !strchr(testLatest,'\\n'));
  assert(testLog[testUsed-1]=='\\n');
  const char *tel="$TEL,0,0,0\\r\\n";
  for(const char *p=tel;*p;p++)Serial1.bytes.push_back(*p);
  receiveSTM32();assert(!captureHeld && !strcmp(pendingTelemetry,"$TEL,0,0,0") && webSocket.messages==0);
  const char *meta="$TINFO,1,6000,3,0,1,104,209,40,8,40,0,0.1,-0.2,0.3,360,360,360,360,0\\r\\n";
  for(const char *p=meta;*p;p++) { Serial1.bytes.push_back(*p); }
  receiveSTM32();
  assert(strstr(testMeta,",360,360,360,360,0") && testStage==3);
  const char *stop="$TSTOP,8,40000,9.2,0.1,0.2,0.3,1.1,0,1800,1\\r\\n";
  for(const char *p=stop;*p;p++) { Serial1.bytes.push_back(*p); }
  receiveSTM32();assert(strstr(testStop,",1800,1"));
  f.sequence=1;f.t_ms=5;feed(f,true);f.sequence=2;f.t_ms=10;feed(f);receiveSTM32();
  assert(testCRCError>=1 && testSequenceGaps==1 && testSamples==2);
  f.sequence=3;f.t_ms=15;feed(f,false,true);f.sequence=4;f.t_ms=20;feed(f);f.sequence=5;f.t_ms=25;feed(f);receiveSTM32();
  assert(testLastSequence==5 && testSequenceGaps>=2);
  size_t saved=testUsed;testCapacity=testUsed;f.sequence=6;f.t_ms=30;feed(f);receiveSTM32();
  assert(testOverflow==1 && testUsed==saved);
  testCapacity=sizeof(storage);f.sequence=0;f.t_ms=0;feed(f);receiveSTM32();
  assert(testSamples==1 && testSequenceGaps==0 && testOverflow==0);
  std::vector<uint8_t> binary(3*1024*1024+16,0xcc);
  jobLogs.attach(binary.data(),binary.size()-16);
  assert(jobLogs.perSlot>=14000);
  auto j=jobLogs.reserve(101);assert(j && !jobLogs.reserve(102));
  jobLogs.marker("$TJ,1,101,5,1,48000,600,0,0,0,0",testMeta);
  unsigned legacy_samples=testSamples;
  f.stage=13;
  for(unsigned i=0;i<10601;i++){f.sequence=i;f.t_ms=i*5;feed(f);receiveSTM32();}
  jobLogs.marker("$TJ,1,101,5,2,48000,600,0,10601,0,0",testMeta);
  assert(j->sealed && j->count==10601 && j->gaps==0 && j->statisticsCount==9601 && jobLogs.active==-1);
  assert(j->sum[0]==(int64_t)f.gx*9601 && j->squares[0]==(int64_t)f.gx*f.gx*9601);
  f.sequence=10601;f.t_ms=53005;feed(f);receiveSTM32();assert(testSamples==legacy_samples && j->count==10601);
  assert(jobLogs.frames(*j)[10600].sequence==10600 && testSamples==legacy_samples);
  j->readers=1;assert(!jobLogs.erase(101));j->readers=0;
  jobLogs.marker("$TJ,1,101,5,2,48000,600,0,999999,0,0",testMeta);assert(j->expected==10601);
  for(unsigned id=102;id<=104;id++) {
    assert(jobLogs.reserve(id));char line[120];snprintf(line,sizeof(line),"$TJ,1,%u,1,1,100,0,0,0,0,0",id);jobLogs.marker(line,testMeta);
    for(unsigned i=0;i<3;i++){f.sequence=i;f.t_ms=i*5;feed(f);receiveSTM32();}
    snprintf(line,sizeof(line),"$TJ,1,%u,1,2,100,0,0,5,0,0",id);jobLogs.marker(line,testMeta);
    assert(jobLogs.find(id)->gaps==2); /* last two frames missing, recovered from final marker */
  }
  assert(!jobLogs.reserve(105) && jobLogs.find(101)->count==10601); /* full never overwrites */
  assert(jobLogs.erase(102));assert(jobLogs.reserve(105));
  jobLogs.marker("$TJ,1,105,1,1,100,0,0,0,0,0",testMeta);
  f.sequence=0;f.t_ms=0;feed(f);receiveSTM32();f.sequence=1;f.t_ms=5;feed(f,true);receiveSTM32();
  f.sequence=2;f.t_ms=10;feed(f);receiveSTM32();
  assert(jobLogs.find(105)->crcErrors>=1 && jobLogs.find(105)->gaps==1);
  jobLogs.failActive(512);assert(jobLogs.find(105)->phase==3 && jobLogs.find(105)->sealed);
  assert(jobLogs.erase(105));assert(jobLogs.reserve(106));jobLogs.marker("$TJ,1,106,1,1,100,0,0,0,0,0",testMeta);
  for(size_t i=0;i<jobLogs.perSlot+1;i++){f.sequence=i;f.t_ms=i*5;feed(f);receiveSTM32();}
  assert(jobLogs.find(106)->overflow==1 && jobLogs.find(106)->count==jobLogs.perSlot);
  for(size_t i=binary.size()-16;i<binary.size();i++)assert(binary[i]==0xcc);
  jobLogs.failActive(512);assert(jobLogs.erase(106));assert(jobLogs.reserve(107));jobLogs.marker("$TJ,1,107,1,1,100,0,0,0,0,0",testMeta);
  f.sequence=5;f.t_ms=25;feed(f);receiveSTM32();f.sequence=0;f.t_ms=0;feed(f);receiveSTM32();
  assert(jobLogs.active<0 && jobLogs.find(107)->sealed && (jobLogs.find(107)->flags&256));
  assert(jobProtocolStop);jobProtocolStop=false;
  assert(jobLogs.erase(107));assert(jobLogs.reserve(108));jobLogs.marker("$TJ,1,108,1,1,100,0,0,0,0,0",testMeta);
  f.sequence=0;f.t_ms=0;f.stage=0;feed(f);receiveSTM32();
  assert(jobLogs.active<0 && (jobLogs.find(108)->flags&256) && jobProtocolStop);
  assert(!balanceDownloading);
  std::vector<uint8_t> balStorage(4*sizeof(BalanceFrame)+16,0xcc);
  balanceFrames=(BalanceFrame*)balStorage.data();balanceCapacity=4;
  BalanceFrame b={};b.state=3;b.dt_us=5000;b.gx=0x5aa5;b.pitch=1.25f;b.ticks_r=-100;
  feedBalance(b);receiveSTM32();assert(balanceCount==1 && balanceLatest.pitch==1.25f);
  b.controller=7;feedBalance(b);receiveSTM32();assert(balanceCount==1&&balanceLatest.controller==12);b.controller=12;
  b.sequence=1;feedBalance(b,true);b.sequence=2;feedBalance(b);receiveSTM32();
  assert(balanceLatest.sequence==2 && balanceCRC>=1 && balanceGaps==1);
  b.sequence=3;feedBalance(b,false,true);b.sequence=4;feedBalance(b);b.sequence=5;feedBalance(b);receiveSTM32();
  assert(balanceLatest.sequence==5 && balanceCount==4 && balanceGaps==2);
  b.sequence=6;feedBalance(b);receiveSTM32();assert(balanceCount==4 && balanceOverflow==1);
  for(size_t i=4*sizeof(BalanceFrame);i<balStorage.size();i++)assert(balStorage[i]==0xcc);
  b.sequence=0;b.t_ms=0;b.state=2;feedBalance(b);receiveSTM32();assert(balanceResets==1);
  balanceCount=0;balanceOverflow=0;balancePostUntil=0;
  b.sequence=1;b.state=7;feedBalance(b);receiveSTM32();assert(balanceCount==1);
  b.sequence=2;b.state=2;feedBalance(b);receiveSTM32();assert(balanceCount==2);
  puts("PASS: balance/pulse binary104 CRC and controller tag, mixed frame lengths/payload sync, missing/corrupt byte recovery, gaps, no overwrite, overflow canary, reset accounting and pulse tail capture");
  puts("PASS: production job archive/receiver, 53s full capture, four slots/no overwrite, missing tail/CRC accounting, 64-bit statistics, frozen end markers, download pins, explicit deletion, overflow guard and STM32 reset detection");
  puts("PASS: production ESP32 CRC/parser, binary sync inside payload, metadata, corrupt/missing bytes, sequence gaps, CSV/JSON preview, buffer overflow and new session");
}
`;
const cppfile = path.join(outDir,'gateway_host.cpp');
const cppExe = path.join(outDir,'gateway_host.exe');
fs.writeFileSync(cppfile,gatewayMock+capture+lineBuffer+receiver+gatewayChecks);
result = cp.spawnSync('C:/msys64/ucrt64/bin/g++.exe',['-std=c++17','-Wall','-Wextra','-Werror','-I'+path.join(root,'ESP32_S3_Gateway'),cppfile,'-o',cppExe],{encoding:'utf8'});
if(result.status!==0)throw new Error(result.stderr || result.error || 'Gateway host compile failed');
result = cp.spawnSync(cppExe,[],{encoding:'utf8'});process.stdout.write(result.stdout || '');
if(result.status!==0)throw new Error(result.stderr || result.error || 'Gateway test failed');
// Validate actual embedded JS, and exercise the component panel with captured fields.
const html = fs.readFileSync(path.join(root,'ESP32_S3_Gateway/index_html.h'),'utf8');
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];
new vm.Script(script);
const elements = {};
const data = {seen:true,stage:11,flags:1,samples:10000,t_ms:50000,crc_errors:0,gaps:0,overflow:0,capacity:2097152,age_ms:0,
  meta:'$TINFO,1,6000,11,0,1,104,209,40,8,40,0,0.1,-0.2,0.3,360,360,360,360,0,4095,255',
  latest:'9999,50000,5000,11,1,0,0,123,-123,2,-3,5,0,0,4096,0'};
const context = {document:{getElementById(id){return elements[id] ||= {textContent:''};}},
  fetch:async()=>({ok:true,json:async()=>data}),setInterval(){},location:{href:''},console};
vm.createContext(context);
const panel = script.slice(script.indexOf('var hardwareLocked'),script.indexOf('var stateLabels'));
vm.runInContext(panel,context);
(async()=>{
  await context.refreshHardwareTest();
  assert.match(elements['hardware-status'].textContent,/HOÀN TẤT/);
  assert.match(elements['hardware-values'].textContent,/0x68 \/ 0xd1/);
  assert.match(elements['hardware-values'].textContent,/360 \/ 360 \/ 360 \/ 360/);
  assert.match(elements['hardware-history'].textContent,/ID 10.*Đã kết thúc bước/);
  data.meta='$TINFO,1,6000,12,16,1,104,209,40,8,40,0,0.1,-0.2,0.3,0,0,0,0,0,4111,3';
  data.stage=12;data.flags=17;await context.refreshHardwareTest();
  assert.match(elements['hardware-history'].textContent,/ID 3.*DỪNG TẠI ĐÂY/);
  assert.match(elements['hardware-history'].textContent,/ID 4.*Chưa chạy/);
  assert.match(elements['hardware-values'].textContent,/Dừng tại ID 3/);
  data.stop='$TSTOP,8,40000,9.2,0.1,0.2,0.3,1.1,0,1800,1';await context.refreshHardwareTest();
  assert.match(elements['hardware-values'].textContent,/PWM trước phanh L\/R: 0 \/ 1800/);
  assert.match(elements['hardware-values'].textContent,/góc từ gia tốc >8°/);
  data.meta=data.meta.replace('$TINFO,1,','$TINFO,2,');await context.refreshHardwareTest();
  assert.match(elements['hardware-values'].textContent,/250 ms/);
  assert.match(elements['hardware-values'].textContent,/Bản test STM32: 2/);
  const exportFunction=script.match(/function exportDataLog\(\) \{[\s\S]*?\n    \}/)[0];
  vm.runInContext(exportFunction,context);context.exportDataLog();
  assert.equal(context.location.href,'/hardware-log'); /* legacy button cannot export stale tuning buffer */
  data.stage=12;data.flags=9;await context.refreshHardwareTest();
  assert.match(elements['hardware-values'].textContent,/lỗi đọc I2C/);
  data.capacity=131072;data.gaps=2;data.age_ms=4000;await context.refreshHardwareTest();
  assert.match(elements['hardware-status'].textContent,/OPI PSRAM/);
  assert.match(elements['hardware-status'].textContent,/mẫu thiếu: 2/);
  console.log('PASS: embedded JS syntax, live measurements, config/deadband display, fault, missing data and PSRAM warnings');
})().catch(error=>{console.error(error);process.exitCode=1;});
