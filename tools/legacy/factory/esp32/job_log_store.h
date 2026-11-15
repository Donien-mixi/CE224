#pragma once
#include <stdint.h>
#include "wheel_measurement.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Fixed slots, append-only binary capture. No String/CSV allocation in UART task.
 * Caller holds captureMutex. Deleting an active or downloading slot is forbidden. */
struct JobLog {
  uint32_t id=0,kind=0,phase=0,duration=0,expected=0,flags=0;
  int pwm=0,reference=0,tag=0;
  size_t count=0; uint32_t gaps=0,crcErrors=0,overflow=0,lastSequence=0,lastMs=0;
  uint16_t stmDrops=0,readers=0,plan=0;
  bool seen=false,sealed=false;
  char meta[256]={},jobMeta[192]={},note[192]={},stop[192]={};
  uint32_t statisticsCount=0;
  int64_t sum[6]={},squares[6]={};
  int32_t firstL=0,firstR=0,lastL=0,lastR=0;
};
class JobLogStore {
public:
  static constexpr unsigned slots=4;
  JobLog jobs[slots];
  TestFrame *memory=nullptr;
  size_t perSlot=0;
  int active=-1;
  void attach(void *storage,size_t bytes) {memory=(TestFrame*)storage;perSlot=bytes/(slots*sizeof(TestFrame));}
  JobLog *find(uint32_t id) {for(auto &j:jobs)if(j.id==id && id)return &j;return nullptr;}
  TestFrame *frames(JobLog &j) {return memory+(&j-jobs)*perSlot;}
  JobLog *reserve(uint32_t id) {
    if(!memory || perSlot<14000 || active>=0 || find(id))return nullptr;
    for(unsigned i=0;i<slots;i++)if(!jobs[i].id) {
      jobs[i]=JobLog{};jobs[i].id=id;active=(int)i;return &jobs[i];
    }
    return nullptr;
  }
  bool erase(uint32_t id) {
    JobLog *j=find(id);if(!j || !j->sealed || j->readers)return false;
    *j=JobLog{};return true;
  }
  void append(const TestFrame &f) {
    if(active<0)return;
    JobLog &j=jobs[active];if(j.phase!=1)return;
    if(f.stage!=13) {j.flags|=256;j.sealed=true;j.phase=3;active=-1;return;}
    if(!j.seen) {j.gaps+=f.sequence;j.firstL=f.ticks_l;j.firstR=f.ticks_r;}
    else if(f.sequence>j.lastSequence+1)j.gaps+=f.sequence-j.lastSequence-1;
    else if(f.sequence<=j.lastSequence) {j.flags|=256;j.sealed=true;j.phase=3;active=-1;return;}
    j.seen=true;j.lastSequence=f.sequence;j.lastMs=f.t_ms;j.stmDrops=f.dropped;
    j.lastL=f.ticks_l;j.lastR=f.ticks_r;j.flags|=f.flags&~1U;
    if(j.count<perSlot)frames(j)[j.count++]=f;else j.overflow++;
    if(f.t_ms>=5000 && (f.flags&1)) {
      int v[6]={f.gx,f.gy,f.gz,f.ax,f.ay,f.az};j.statisticsCount++;
      for(unsigned i=0;i<6;i++){j.sum[i]+=v[i];j.squares[i]+=(int64_t)v[i]*v[i];}
    }
  }
  /* End marker is sent after the STM32 DMA sample queue drains. */
  void marker(const char *line,const char *configuration) {
    unsigned version,id,kind,phase,duration,expected,flags;int pwm,reference,tag;
    if(sscanf(line,"$TJ,%u,%u,%u,%u,%u,%d,%d,%u,%u,%d",&version,&id,&kind,&phase,
       &duration,&pwm,&reference,&expected,&flags,&tag)!=10 || version!=1 ||
       kind<1 || kind>14 || phase<1 || phase>3 || duration<100 || duration>60000 ||
       pwm< -1800 || pwm>1800 || reference< -180000 || reference>180000 ||
       tag<0 || tag>8 || expected>20000 || flags>255)return;
    if(kind>=12 && !WheelMeasurementValid(kind,duration,pwm,tag,reference))return;
    JobLog *j=find(id);if(!j)return; /* unsolicited/reset markers must never overwrite a log */
    if(j->sealed)return;
    j->kind=kind;j->phase=phase;j->duration=duration;j->pwm=pwm;
    j->reference=reference;j->tag=tag;j->expected=expected;j->flags|=flags;
    snprintf(j->jobMeta,sizeof(j->jobMeta),"%s",line);
    if(configuration[0])snprintf(j->meta,sizeof(j->meta),"%s",configuration);
    if(phase>=2) {
      if(expected>j->count && expected-j->count>j->gaps)j->gaps=expected-j->count;
      j->sealed=true;active=-1;
    }
  }
  void failActive(unsigned flag) {
    if(active<0)return;
    jobs[active].flags|=flag;jobs[active].phase=3;jobs[active].sealed=true;active=-1;
  }
};
