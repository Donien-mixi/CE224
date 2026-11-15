// Exercise the actual driver with NACK/BUSY/stuck-wire/register fault injection.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/bmi160_host';fs.mkdirSync(dir,{recursive:true});
const header=fs.readFileSync('Xe_Can_Bang/Core/Inc/bmi160.h','utf8').replace(/^#include "[^\n]+\r?\n/gm,'');
const driver=fs.readFileSync('Xe_Can_Bang/Core/Src/bmi160.c','utf8').split('void BMI160_Test_Run(void)')[0].replace(/^#include "[^\n]+\r?\n/gm,'');
const mock=String.raw`
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef enum{HAL_OK,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct{uint32_t CYCCNT;} Dwt;static Dwt dwt;
#define DWT (&dwt)
static uint32_t SystemCoreClock=100000000,ms,hal_error;static int hi2c1;
#define HAL_I2C_STATE_READY 0
#define I2C_MEMADD_SIZE_8BIT 1
#define I2C_FLAG_BUSY 1
#define HAL_I2C_ERROR_AF 4U
#define HAL_I2C_ERROR_TIMEOUT 32U
typedef struct{int Pin,Mode,Pull,Speed;} GPIO_InitTypeDef;
#define GPIOB 0
#define GPIO_PIN_8 256
#define GPIO_PIN_9 512
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define GPIO_MODE_OUTPUT_OD 1
#define GPIO_PULLUP 1
#define GPIO_SPEED_FREQ_HIGH 1
#define __HAL_RCC_I2C1_FORCE_RESET() ((void)0)
#define __HAL_RCC_I2C1_RELEASE_RESET() ((void)0)
static uint8_t regs[256];static int nack,failed_write=-1,corrupt_reg=-1,stuck_sda,stuck_scl,pulses,busy,reads;
static uint32_t failure_cycles=20000;
#define __HAL_I2C_GET_FLAG(h,f) (busy)
static uint32_t HAL_GetTick(void){return ms;}
static void HAL_Delay(int n){ms+=n;}
static uint32_t HAL_I2C_GetError(int *p){(void)p;return hal_error;}
static int HAL_I2C_GetState(int *p){(void)p;return 0;}
static int HAL_I2C_Init(int *p){(void)p;busy=0;return HAL_OK;}
static int HAL_I2C_DeInit(int *p){(void)p;return HAL_OK;}
static void HAL_GPIO_Init(int p,GPIO_InitTypeDef *v){(void)p;(void)v;}
static void HAL_GPIO_WritePin(int port,int pins,int value){(void)port;if((pins&GPIO_PIN_8)&&value==1)pulses++;}
static int HAL_GPIO_ReadPin(int port,int pin){(void)port;return pin==GPIO_PIN_8?!stuck_scl:(!stuck_sda||pulses>=stuck_sda);}
static int HAL_I2C_Mem_Write(int *p,int addr,int reg,int size,uint8_t *v,int n,int timeout){
 (void)p;(void)addr;(void)size;(void)n;(void)timeout;if(reg==failed_write)return HAL_ERROR;
 regs[reg]=*v;if(reg==0x7e&&*v==0x11)regs[3]|=0x10;if(reg==0x7e&&*v==0x15)regs[3]|=4;return HAL_OK;
}
static int HAL_I2C_Mem_Read(int *p,int addr,int reg,int size,uint8_t *v,int n,int timeout){
 (void)p;(void)addr;(void)size;(void)timeout;reads++;dwt.CYCCNT+=failure_cycles;
 if(nack){nack--;hal_error=HAL_I2C_ERROR_AF;return HAL_ERROR;}
 memcpy(v,regs+reg,n);if(corrupt_reg>=reg&&corrupt_reg<reg+n)v[corrupt_reg-reg]^=1;return HAL_OK;
}
`;
const tests=String.raw`
int main(void){
 regs[0]=0xd1;assert(BMI160_Init());
 // No silently successful initialization after a failed configuration write.
 failed_write=0x43;assert(!BMI160_Init());failed_write=-1;
 corrupt_reg=0x41;assert(!BMI160_Init());corrupt_reg=-1;assert(BMI160_Init());
 // A short NACK retries once and delivers real data, not the previous sample.
 regs[0x12]=1;BMI160_Data_t data={0};nack=1;int before=reads;
 assert(BMI160_Read_All(&data)&&reads==before+2&&data.ax==1.0f/4096);
 uint32_t h[4];BMI160_GetHealth(h);assert(h[0]==1&&h[1]==1&&h[2]==4);
 nack=2;before=reads;assert(!BMI160_Read_All(&data)&&reads==before+2);
 // Late errors cannot spend another transaction inside a control deadline.
 nack=1;failure_cycles=80000;before=reads;assert(!BMI160_Read_All(&data)&&reads==before+1);nack=0;
 busy=1;before=reads;assert(!BMI160_Read_All(&data)&&reads==before);BMI160_GetHealth(h);assert(h[2]==0x80000000U);
 // Stopped-only bus clear: release an interrupted slave, refuse a stuck clock.
 pulses=0;stuck_sda=9;assert(BMI160_Init());stuck_sda=0;
 stuck_scl=1;assert(!BMI160_Init());stuck_scl=0;assert(BMI160_Init());
 assert(!BMI160_CalibrationOK());
 // A moved calibration cannot overwrite the previous valid gyro bias.
 regs[0x0e]=4;regs[0x0f]=0;BMI160_Calibrate_Gyro(200);assert(BMI160_CalibrationOK());
 float bias[3];BMI160_GetBias(bias);float good=bias[1];assert(fabsf(good-4.0f/16.4f)<1e-5f);
 regs[0x0e]=100;BMI160_Calibrate_Gyro(200);assert(!BMI160_CalibrationOK());
 BMI160_GetBias(bias);assert(bias[1]==good);
 puts("PASS BMI160: checked writes/readback/PMU, one bounded NACK retry, BUSY fast fail, stopped bus clear, stuck clock rejected.");
}
`;
fs.writeFileSync(dir+'/test.c',mock+header+driver+tests);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',dir+'/test.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
