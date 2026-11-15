// Bounds and lifecycle of the temporary experiment, using the production helper.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict'),path=require('path');
const dir=path.resolve('.build/load_speed');fs.mkdirSync(dir,{recursive:true});
assert.deepEqual(fs.readFileSync('Xe_Can_Bang/Core/Inc/load_speed_test.h'),fs.readFileSync('ESP32_S3_Gateway/load_speed_test.h'));
const source=String.raw`
#include <assert.h>
#include <stdio.h>
#include "load_speed_test.h"
static uint32_t now;
static void step(LoadSpeedTest*s,int request,int fresh,int allow,int ready,float vl,float vr,float x,float ref,int neutral){
 now+=5;LoadTest_Update(s,request,fresh,true,allow,ready,0,0,vl,vr,x,ref,neutral,now);
}
static LoadSpeedTest begin(void){LoadSpeedTest s={0};for(int i=0;i<200;i++)step(&s,0,0,1,1,0,0,0,0,1);step(&s,1,1,1,1,0,0,0,0,1);assert(s.phase==1&&LoadTest_Guard(&s));assert(LoadTest_Flags(&s)==20480);return s;}
int main(void){
 Drive8Packet p;assert(LoadTest_Parse("$LOADTEST,12,5,1",&p));assert(p.epoch==12&&p.sequence==5&&p.direction==1&&p.turn==0&&p.preset==2);
 const char*bad[]={"$LOADTEST,12,5,0","$LOADTEST,12,5,2","$LOADTEST,012,5,1","$LOADTEST,12,05,1","$LOADTEST,12,5,1*","$LOADTEST,12,5,1junk","$LOADTEST,12,4294967297,1","$MOVE8,1,0,12,5,2"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)assert(!LoadTest_Parse(bad[i],&p));
 assert(!Drive8_Parse("$MOVE8,1,0,12,5,2",&p));
 LoadSpeedTest s={0};step(&s,1,1,1,1,0,0,0,0,1);assert(s.phase==4&&s.reason==6&&!LoadTest_Guard(&s));
 s=(LoadSpeedTest){0};for(int i=0;i<200;i++)step(&s,0,0,1,1,0,0,0,0,1);step(&s,1,1,1,1,.13f,.13f,0,0,1);assert(s.phase==4);
 step(&s,0,1,1,1,0,0,0,0,1);assert(s.phase==0);
 s=(LoadSpeedTest){0};for(int i=0;i<100;i++){now+=5;LoadTest_Update(&s,false,false,true,true,true,2.8f,12,.075f,.065f,0,0,true,now);}step(&s,1,1,1,1,.059411f,.046158f,0,0,1);assert(s.phase==1); // observed balancing corrections must allow entry
 s=begin();step(&s,1,0,1,1,.5f,.5f,1,.5f,0);assert(s.phase==2&&s.reason==1&&LoadTest_Guard(&s));
 for(int i=0;i<20;i++)step(&s,1,1,1,1,0,0,1,0,1);assert(s.phase==3&&!LoadTest_Guard(&s));
 for(int i=0;i<250;i++)step(&s,1,1,1,1,0,0,1,0,1);assert(s.phase==3); // no repeat in same ARM
 s=begin();step(&s,0,1,1,1,.5f,.5f,1,.5f,0);assert(s.phase==2&&s.reason==1);
 s=begin();step(&s,1,1,0,1,.5f,.5f,1,.5f,0);assert(s.phase==2&&s.reason==4);
 s=begin();step(&s,1,1,1,1,.95f,.55f,1,.75f,0);assert(s.phase==2&&s.reason==4);
 s=begin();step(&s,1,1,1,1,.95f,.95f,6,.95f,0);assert(s.phase==2&&s.reason==3);
 now=UINT32_MAX-2000;s=begin();now=s.started_ms+LOAD_TEST_TIME_MS-5;step(&s,1,1,1,1,.95f,.95f,5,.95f,0);assert(s.phase==2&&s.reason==2); // rollover
 s=begin();for(int i=0;i<19;i++){LoadTest_Reserve(&s,2300,0,now);}assert(s.phase==1);LoadTest_Reserve(&s,2300,0,now);assert(s.phase==2&&s.reason==7);
 s=begin();LoadTest_Reserve(&s,2300,0,now);LoadTest_Reserve(&s,2000,0,now);assert(s.reserve==0&&s.phase==1);
 s=begin();LoadTest_Fault(&s,now);assert(s.phase==3&&s.reason==5&&!LoadTest_Guard(&s));
 s=begin();LoadTest_Update(&s,true,true,true,true,true,NAN,0,0,0,0,0,false,now);assert(s.phase==3&&s.reason==5);
 puts("PASS LOADTEST parser, stationary start, time/distance, release/lease, recovery/asymmetry, fault, rollover and one attempt per ARM.");
}`;
const file=path.join(dir,'test.c'),exe=path.join(dir,'test.exe');fs.writeFileSync(file,source);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-IXe_Can_Bang/Core/Inc',file,'-lm','-o',exe],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);r=cp.spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
