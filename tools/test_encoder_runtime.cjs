// Production timer-count signs, wrap, LPF and independence. No simulated motor law.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/encoder_host';fs.mkdirSync(dir,{recursive:true});
const header=fs.readFileSync('Xe_Can_Bang/Core/Inc/encoder.h','utf8').replace(/^#include "[^\n]+\r?\n/gm,'');
const driver=fs.readFileSync('Xe_Can_Bang/Core/Src/encoder.c','utf8').replace(/^#include "[^\n]+\r?\n/gm,'');
const code=String.raw`
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#define BALANCE_PPR 898.225f
static uint16_t htim2,htim3;
#define TIM_CHANNEL_ALL 0
#define HAL_TIM_Encoder_Start(h,c) ((void)(h),(void)(c))
#define __HAL_TIM_GET_COUNTER(h) (*(h))
#define __HAL_TIM_SET_COUNTER(h,c) (*(h)=(c))
`+header+driver+String.raw`
int main(void){
 Encoder_Init();int32_t l,r;Encoder_GetHardwareTicks(&l,&r);assert(!l&&!r);
 for(int i=0;i<10000;i++){htim2+=10;htim3-=10;Encoder_Update(.005f);}
 Encoder_GetHardwareTicks(&l,&r);assert(l==100000&&r==-100000);
 float expected=10*METERS_PER_TICK/.005f;
 assert(fabsf(Encoder_GetLeftVelocity()-expected)<1e-5f&&fabsf(Encoder_GetRightVelocity()-expected)<1e-5f);
 for(int i=0;i<10000;i++){htim2-=10;htim3+=10;Encoder_Update(.005f);}
 Encoder_GetHardwareTicks(&l,&r);assert(!l&&!r);
 for(int i=0;i<200;i++)Encoder_Update(.005f);
 assert(fabsf(Encoder_GetAverageVelocity())<1e-6f);
 // Moving one wheel cannot alter the other channel or erase its counter.
 htim2+=50;Encoder_Update(.005f);Encoder_GetHardwareTicks(&l,&r);assert(l==50&&r==0);
 for(int i=0;i<100;i++)Encoder_Update(.005f);
 Encoder_GetHardwareTicks(&l,&r);assert(l==50&&r==0);
 puts("PASS actual encoder: forward signs, both wrap directions, independent channels, no phantom idle counts, LPF settling.");
}
`;
fs.writeFileSync(dir+'/test.c',code);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',dir+'/test.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
