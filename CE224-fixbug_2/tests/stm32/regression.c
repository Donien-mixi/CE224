#include "hal_mock.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bmi160.h"
#include "filter.h"
#include "esp32_comm.h"
#include "buzzer_led.h"
/* Include the real FSM so regressions can also inspect its IRQ queue. */
#include "../../Xe_Can_Bang/Core/Src/robot_fsm.c"

I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1,htim2,htim3,htim4;
MockCore mock_core;
uint32_t SystemCoreClock=100000000;
int mock_busy;
static uint32_t tick;
static uint8_t regs[256], address=0x68, ready_bits=0xc0;
static bool fresh=true, suspended, read_failure;
static int fail_write=-1, ignore_write=-1, read_calls, motor_l,motor_r;
static ESP32_Command_t command;
uint32_t HAL_GetTick(void){return tick;}
void HAL_Delay(uint32_t ms){tick+=ms;}
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h){(void)h;return HAL_OK;}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *h,uint16_t addr,uint16_t reg,uint16_t mode,uint8_t *buf,uint16_t n,uint32_t timeout){
 (void)h;(void)mode;(void)timeout;read_calls++;
 if(addr!=(address<<1)||read_failure)return HAL_ERROR;
 if(reg==BMI160_REG_STATUS&&fresh)regs[reg]|=ready_bits;
 memcpy(buf,regs+reg,n);
 if(reg==BMI160_REG_DATA_0)regs[BMI160_REG_STATUS]=0;
 return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *h,uint16_t addr,uint16_t reg,uint16_t mode,uint8_t *buf,uint16_t n,uint32_t timeout){
 (void)h;(void)mode;(void)n;(void)timeout;
 if(addr!=(address<<1)||reg==fail_write)return HAL_ERROR;
 if(reg==ignore_write)return HAL_OK;
 if(reg==BMI160_REG_CMD){
  if(*buf==BMI160_CMD_SOFT_RESET){uint8_t physical[12];memcpy(physical,regs+0x0c,12);memset(regs,0,sizeof(regs));memcpy(regs+0x0c,physical,12);regs[0]=0xd1;regs[0x41]=3;}
  if(!suspended&&*buf==BMI160_CMD_ACC_MODE_NORMAL)regs[3]|=0x10;
  if(!suspended&&*buf==BMI160_CMD_GYR_MODE_NORMAL)regs[3]|=4;
 }else regs[reg]=*buf;
 return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h,uint16_t a,uint32_t b,uint32_t c){(void)h;(void)b;(void)c;return a==(address<<1)?HAL_OK:HAL_ERROR;}
void HAL_GPIO_TogglePin(int a,int b){(void)a;(void)b;}
void HAL_NVIC_SetPriority(int a,int b,int c){(void)a;(void)b;(void)c;}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h){(void)h;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *h){(void)h;return HAL_OK;}
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *h,uint32_t c){(void)h;(void)c;return HAL_OK;}
void Motor_Init(void){}
void Motor_Stop(void){motor_l=motor_r=0;}
void Motor_Coast(void){Motor_Stop();}
void Motor_SetDuty(int16_t l,int16_t r){motor_l=l;motor_r=r;}
void Motor_SelfTest(void){}
void PID_Init(void){}
void PID_Reset_Integral(void){}
void BuzzerLED_Init(void){}
void Buzzer_On(void){}
void Buzzer_BeepAsync(uint16_t ms){(void)ms;}
void LED_SetPattern(LED_Pattern_t p){(void)p;}
void ESP32_Comm_Init(void){memset(&command,0,sizeof(command));}
void ESP32_Comm_FlushRx(void){}
ESP32_Command_t *ESP32_Comm_GetCommand(void){return &command;}
bool ESP32_Comm_SendBytes(const void *p,uint16_t n){(void)p;(void)n;return true;}
static void raw16(unsigned reg,int16_t v){regs[reg]=(uint16_t)v&255;regs[reg+1]=(uint16_t)v>>8;}
static void pose(float degrees){
 float rad=degrees*DEG_TO_RAD;
 raw16(0x12,(int16_t)(sinf(rad)*4096));raw16(0x14,0);raw16(0x16,(int16_t)(cosf(rad)*4096));
 raw16(0x0c,0);raw16(0x0e,0);raw16(0x10,0);
}
static void init_sensor(void){
 read_failure=suspended=false;mock_busy=0;fail_write=ignore_write=-1;fresh=true;ready_bits=0xc0;regs[0]=0xd1;
 assert(BMI160_Init());pose(0);
}
static void loop_tick(void){tick+=5;mock_core.CYCCNT+=500000;Robot_ControlLoop_200Hz();}
static void filter_pose(float target,float *x,float *z){
 float raw=(target/PITCH_ACC_SIGN+BALANCE_RAW_ZERO_DEG)*DEG_TO_RAD;
 *x=sinf(raw)*BALANCE_ACC_SX+BALANCE_ACC_BX;
 *z=cosf(raw)*BALANCE_ACC_SZ+BALANCE_ACC_BZ;
}
int main(void){
 BMI160_Data_t data;
 init_sensor();address=0x69;assert(BMI160_Init());address=0x68;
 const int writes[]={BMI160_REG_CMD,0x40,0x41,0x42,0x43};
 for(unsigned i=0;i<sizeof(writes)/sizeof(*writes);i++){
  init_sensor();fail_write=writes[i];assert(!BMI160_Init());assert(!BMI160_Read_All(&data));
 }
 init_sensor();ignore_write=0x41;assert(!BMI160_Init());
 init_sensor();suspended=true;assert(!BMI160_Init());
 init_sensor();assert(BMI160_Read_All(&data));
 /* At rest, unchanged values and 200 Hz duplicate polls remain valid. */
 fresh=false;tick+=5;assert(BMI160_Read_All(&data));
 tick+=5;fresh=true;assert(BMI160_Read_All(&data));
 fresh=false;tick+=29;assert(BMI160_Read_All(&data));tick++;assert(!BMI160_Read_All(&data));
 /* Detect either sensor stopping independently. */
 init_sensor();assert(BMI160_Read_All(&data));ready_bits=0x40;tick+=30;assert(!BMI160_Read_All(&data));
 init_sensor();assert(BMI160_Read_All(&data));ready_bits=0x80;tick+=30;assert(!BMI160_Read_All(&data));
 init_sensor();tick=UINT32_MAX-10;assert(BMI160_Read_All(&data));fresh=false;tick+=29;assert(BMI160_Read_All(&data));tick++;assert(!BMI160_Read_All(&data));
 init_sensor();mock_busy=1;int before=read_calls;assert(!BMI160_Read_All(&data));assert(read_calls==before);mock_busy=0;
 init_sensor();raw16(0x0e,-164);raw16(0x12,-4096);assert(BMI160_Read_All(&data));assert(fabsf(data.gy+10)<.01f&&data.ax==-1);
 init_sensor();raw16(0x0e,8);BMI160_Calibrate_Gyro(200);assert(BMI160_CalibrationOK());
 float bias[3],old[3];BMI160_GetBias(old);raw16(0x0e,164);BMI160_Calibrate_Gyro(200);assert(!BMI160_CalibrationOK());BMI160_GetBias(bias);assert(bias[1]==old[1]);
 init_sensor();pose(0);raw16(0x16,0);BMI160_Calibrate_Gyro(200);assert(!BMI160_CalibrationOK());
 /* Normal +/-90 static tilt converges; wrap boundary never blends to zero. */
 float x,z;
 for(int direction=-1;direction<=1;direction+=2){
  Filter_Init(0);filter_pose(direction*90,&x,&z);
  for(int i=0;i<6000;i++)Filter_Complementary_Update(0,x,z,.005f);
  assert(fabsf(Filter_GetAngle()-direction*90)<.01f);
 }
 Filter_Init(179/PITCH_ACC_SIGN);filter_pose(-179,&x,&z);
 assert(fabsf(Filter_Complementary_Update(0,x,z,.005f))>178);
 Filter_Init(0);filter_pose(60,&x,&z);
 for(int i=0;i<1000;i++)Filter_Complementary_Update(0,x,z,.005f);
 float angle=Filter_GetAngle();Filter_Init(0);
 for(int i=0;i<500;i++)Filter_Complementary_Update(0,x,z,.010f);
 assert(fabsf(angle-Filter_GetAngle())<.005f);
 /* Exercise real driver -> filter -> FSM -> motor -> telemetry queue. */
 init_sensor();Robot_Init();pose(0);
 for(int i=0;i<45;i++)loop_tick();assert(s_robot.ready);
 command.arm_profile=12;command.arm_request=1;command.last_cmd_time=tick;
 loop_tick();assert(s_robot.state==ROBOT_STATE_BALANCING);
 /* Stall while actively balancing: stop outputs and require recovery. */
 fresh=false;for(int i=0;i<8;i++)loop_tick();
 assert(s_robot.state==ROBOT_STATE_EMERGENCY&&(s_robot.fault&32)&&motor_l==0&&motor_r==0);
 fresh=true;Robot_RecalibrateIMU();pose(0);
 for(int i=0;i<45;i++)loop_tick();assert(s_robot.ready);
 command.arm_profile=12;command.arm_request=1;command.last_cmd_time=tick;
 loop_tick();assert(s_robot.state==ROBOT_STATE_BALANCING);
 command.emergency_stop=1;loop_tick();assert(s_robot.state==ROBOT_STATE_FALLEN&&motor_l==0&&motor_r==0);
 pose(90);for(int i=0;i<6000;i++)loop_tick();assert(fabsf(s_robot.pitch)>80&&motor_l==0&&motor_r==0);
 /* ACK without new data faults, rejects ARM; re-calibration recovers. */
 fresh=false;for(int i=0;i<8;i++)loop_tick();assert(s_robot.state==ROBOT_STATE_EMERGENCY&&(s_robot.fault&32));
 command.emergency_stop=1;loop_tick();assert(s_robot.state==ROBOT_STATE_EMERGENCY);
 command.arm_request=1;loop_tick();assert(!s_robot.ready&&motor_l==0&&motor_r==0);
 fresh=true;command.pulse_request=1;Robot_RecalibrateIMU();assert(!command.arm_request&&!command.pulse_request);
 assert(s_robot.state==ROBOT_STATE_STANDBY&&bal_last_cycle==0);
 float previous=s_robot.pitch;pose(-45);for(int i=0;i<6000;i++)loop_tick();assert(s_robot.pitch>30&&s_robot.pitch!=previous);
 /* A newly emitted frame carries the current post-fall angle and valid CRC. */
 bal_tail=bal_head;for(int i=0;i<4;i++)loop_tick();
 assert(bal_head!=bal_tail);BalanceFrame f=bal_queue[bal_tail];
 assert(fabsf(f.pitch-s_robot.pitch)<.1f&&f.pwm_l==0&&f.pwm_r==0);
 assert(f.crc==TestCRC((uint8_t*)&f,sizeof(f)-2));
 /* A failed reinitialization remains an emergency; no stale pulse can run. */
 command.pulse_request=1;fail_write=0x41;Robot_RecalibrateIMU();
 assert(s_robot.state==ROBOT_STATE_EMERGENCY&&!command.pulse_request&&motor_l==0&&motor_r==0);
 puts("PASS: BMI160 init/freshness/calibration, angle filter, STM32 fall/fault/recalibration/telemetry pipeline");
}
