#pragma once
#include <esp_random.h>
#include <ctype.h>
#include <limits.h>
// Web endpoints run on the web task. Only short metadata operations hold captureMutex.
static char measurementOwner[40]={};
static bool strictNumber(const String &s,long &value) {
  if(!s.length() || s.length()>11)return false;
  char *end;errno=0;value=strtol(s.c_str(),&end,10);return *end==0 && errno!=ERANGE;
}
static bool sendMeasurementBytes(WiFiClient &client,const char *bytes,size_t n) {
  uint32_t progress=millis();
  for(size_t sent=0;sent<n && client.connected();) {
    int written=::send(client.fd(),bytes+sent,n-sent,MSG_DONTWAIT);
    if(written>0){sent+=written;progress=millis();}
    else if(written<0 && errno!=EAGAIN && errno!=EWOULDBLOCK)return false;
    if(millis()-progress>5000)return false;
    webSocket.loop();yield();
    if(sent==n)return true;
  }
  return false;
}
static int measurementRow(char *row,size_t capacity,const TestFrame &f) {
  return snprintf(row,capacity,"%lu,%lu,%u,%u,%u,%d,%d,%ld,%ld,%d,%d,%d,%d,%d,%d,%u\n",
    (unsigned long)f.sequence,(unsigned long)f.t_ms,f.dt_us,f.stage,f.flags,f.pwm_l,f.pwm_r,
    (long)f.ticks_l,(long)f.ticks_r,f.gx,f.gy,f.gz,f.ax,f.ay,f.az,f.dropped);
}
static void downloadMeasurement() {
  long id;
  if(!strictNumber(server.arg("id"),id)){server.send(400,"text/plain","Invalid id");return;}
  xSemaphoreTake(captureMutex,portMAX_DELAY);
  JobLog *j=jobLogs.find((uint32_t)id);
  if(!j || !j->sealed || jobLogs.active>=0 || jobDownloadBusy) {
    xSemaphoreGive(captureMutex);server.send(409,"text/plain","Wait for completion; stop motors before downloading.");return;
  }
  j->readers++;jobDownloadBusy=true;
  const TestFrame *frames=jobLogs.frames(*j);size_t count=j->count;
  xSemaphoreGive(captureMutex);
  String header="# TWIP_MEASUREMENT_V2\n# Read200Hz; BMI160 ODR100Hz: repeated reads are not independent samples.\n";
  header+="# Settling: first5000ms excluded from summary; all raw rows are retained.\n";
  header+="# Raw gyro/16.4 dps, accel/4096 g; ticks are raw hardware signs; PWM applied BEFORE sample.\n";
  header+="# No voltage/current/load sensors. user_note is operator input, NOT a sensor measurement.\n";
  header+="# No calibration coefficients were changed. Kind4 uses COAST; kind11 COAST after pulse; others BRAKE at zero PWM.\n";
  header+="# TJ: version,token,kind,phase(1run/2done/3abort),requested_ms,pwm,reference,attempted_samples,flags,tag,DB_trial0..5\n";
  header+="# flags: 2 timing;4 STOP/heartbeat;8 I2C;16 encoder;32 motion;64 config;128 notStill;256 sequenceReset;512 noSTM32ACK/data.\n";
  header+="# reference: kind2 angle_millideg or axis tag; kind4 completed wheel turns; others annotation only.\n";
  header+="# "+String(j->jobMeta)+"\n# "+String(j->meta)+"\n# "+String(j->stop)+"\n";
  header+="# user_note="+String(j->note)+"\n# captured_samples="+String((unsigned)count)+
    " sequence_gaps="+String(j->gaps)+" CRC_errors="+String(j->crcErrors)+" buffer_drops="+String(j->overflow)+
    " STM32_drops="+String(j->stmDrops)+" flags="+String(j->flags)+" phase="+String(j->phase)+"\n";
  header+="# plan_version=1 plan_step="+String(j->plan)+"; plan note describes REQUESTED action, not verified physical conditions.\n";
  if(j->plan>=61 && j->plan<=72)header+="# wheel_plan_version=1 wheel_step="+String(j->plan-60)+"; raw PWM; three repeats; BRAKE between pulses; kind12 +/-600 2s/1s, kind13 levels120,160,180,200,220,260,320,400 1s/0.5s, kind14 +/-300 1.5s/0.3s/1.5s/1s; settling5s\n";
  header+="# firmware="+String(TWIP_WEB_BUILD)+"\n";
  header+="# delta_ticks_l="+String(j->lastL-j->firstL)+" delta_ticks_r="+String(j->lastR-j->firstR)+
    " summary_read_count="+String(j->statisticsCount)+"\n";
  const char *axes[6]={"gx","gy","gz","ax","ay","az"};
  for(unsigned i=0;i<6;i++) {
    double mean=j->statisticsCount?(double)j->sum[i]/j->statisticsCount:0;
    double variance=j->statisticsCount?(double)j->squares[i]/j->statisticsCount-mean*mean:0;
    char line[120];snprintf(line,sizeof(line),"# %s mean_raw=%.6f std_raw=%.6f\n",axes[i],mean,sqrt(variance>0?variance:0));header+=line;
  }
  header+="seq,t_ms,dt_us,stage,flags,pwm_l,pwm_r,ticks_l,ticks_r,gx_raw,gy_raw,gz_raw,ax_raw,ay_raw,az_raw,stm32_drops\n";
  char footer[96];snprintf(footer,sizeof(footer),"# END job_id=%lu rows=%u\n",(unsigned long)j->id,(unsigned)count);
  char row[192];size_t bytes=header.length()+strlen(footer);
  for(size_t i=0;i<count;i++)bytes+=(size_t)measurementRow(row,sizeof(row),frames[i]);
  server.sendHeader("Content-Disposition","attachment; filename=twip_test_"+String(j->id)+"_kind"+String(j->kind)+".txt");
  server.sendHeader("Cache-Control","no-store");server.setContentLength(bytes);
  server.send(200,"text/plain; charset=utf-8",header);
  auto client=server.client();bool ok=true;char block[2048];size_t used=0;
  for(size_t i=0;i<count && ok;i++) {
    int n=measurementRow(row,sizeof(row),frames[i]);
    if(used+(size_t)n>sizeof(block)){ok=sendMeasurementBytes(client,block,used);used=0;}
    memcpy(block+used,row,(size_t)n);used+=(size_t)n;
  }
  if(ok && used)ok=sendMeasurementBytes(client,block,used);
  if(ok)ok=sendMeasurementBytes(client,footer,strlen(footer));
  if(!ok)client.stop();
  xSemaphoreTake(captureMutex,portMAX_DELAY);j->readers--;jobDownloadBusy=false;xSemaphoreGive(captureMutex);
}

void setupMeasurementRoutes() {
  nextJobId=100+(esp_random()%1000000000U);
  server.on("/wheel-test-enter",HTTP_POST,[](){
    long client=-1,token=0,revision=-1;
    if(!strictNumber(server.arg("session"),client)||!strictNumber(server.arg("token"),token)||
       !controlCanClaim(client,token)||!strictNumber(server.arg("stop_revision"),revision)||
       revision<0||(uint32_t)revision!=controlRevision||server.arg("ready")!="yes") {
      server.send(409,"text/plain","Keep this page open; confirm fixture and wait for connection.");return;
    }
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool matched=!strncmp(balanceMeta,"$BINFO,BAL-20261015-R12,",sizeof("$BINFO,BAL-20261015-R12,")-1);
    bool diagnostic=testSeen && testStage>=11 && testStage!=13 && millis()-testLastReceived<2500 &&
                    !(balanceSeen && millis()-balanceReceived<1000);
    bool stopped=balanceSeen && millis()-balanceReceived<500 && balanceLatest.state!=3 && balanceLatest.state!=7 &&
                 balanceLatest.state!=6 && !(balanceLatest.fault&(32|256|512));
    bool ok=matched && (diagnostic||stopped) && jobLogs.active<0 && !jobDownloadBusy && !balanceDownloading;
    xSemaphoreGive(captureMutex);
    if(!ok){server.send(409,"text/plain","Need STM32 R7, stopped motors and fresh data. Stop and finish downloads first.");return;}
    controlClaim(client);
    if(!diagnostic)Serial1.print("$MEASURE*\r\n");
    server.send(200,"text/plain","Measurement mode requested; wait for STM32. Wheels remain braked until START.");
  });
  server.on("/measurement-status",HTTP_GET,[](){
    String reply="{\"available\":"+String(jobLogs.memory&&jobLogs.perSlot>=14000?"true":"false")+",\"records_per_slot\":"+String((unsigned)jobLogs.perSlot)+",\"jobs\":[";
    xSemaphoreTake(captureMutex,portMAX_DELAY);bool comma=false;
    for(auto &j:jobLogs.jobs)if(j.id) {
      if(comma)reply+=",";comma=true;
      char item[400];snprintf(item,sizeof(item),"{\"id\":%lu,\"kind\":%lu,\"phase\":%lu,\"sealed\":%s,\"samples\":%u,\"t_ms\":%lu,\"flags\":%lu,\"gaps\":%lu,\"crc\":%lu,\"overflow\":%lu,\"stm_drops\":%u,\"plan\":%u}",
        (unsigned long)j.id,(unsigned long)j.kind,(unsigned long)j.phase,j.sealed?"true":"false",(unsigned)j.count,
        (unsigned long)j.lastMs,(unsigned long)j.flags,(unsigned long)j.gaps,(unsigned long)j.crcErrors,(unsigned long)j.overflow,j.stmDrops,j.plan);
      reply+=item;
    }
    reply+="],\"active\":"+String(jobLogs.active>=0?(long)jobLogs.jobs[jobLogs.active].id:0)+",\"latest\":\""+String(testLatest)+"\"}";
    xSemaphoreGive(captureMutex);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",reply);
  });
  server.on("/measurement-start",HTTP_POST,[](){
    long kind,duration,pwm,reference,tag,plan=0;
    if(server.hasArg("plan") && (!strictNumber(server.arg("plan"),plan)||plan<0||plan>100)) {
      server.send(400,"text/plain","Invalid plan step");return;
    }
    String owner=server.arg("owner");
    if(!strictNumber(server.arg("kind"),kind)||!strictNumber(server.arg("duration"),duration)||
       !strictNumber(server.arg("pwm"),pwm)||!strictNumber(server.arg("reference"),reference)||
       !strictNumber(server.arg("tag"),tag)||kind<1||kind>14||duration<100||duration>60000||pwm< -1800||pwm>1800||
       reference< -180000||reference>180000||tag<0||tag>8||owner.length()<8||owner.length()>32||
       ((kind==8||kind==11) && (duration>700||tag>2))||(kind==9 && (duration>200||pwm< -800||pwm>800||tag>2))||
       (kind==10 && tag>1)||(kind>=5 && server.arg("ready")!="yes")) {
      server.send(400,"text/plain","Invalid parameters or fixture confirmation missing.");return;
    }
    if(kind>=12 && (!WheelMeasurementValid(kind,(uint32_t)duration,(int)pwm,(int)tag,(int)reference) ||
       plan<61 || plan>72 || kind!=(plan<=63?12:plan<=69?13:14) ||
       tag!=(plan<=63?plan-61:plan<=69?(plan-64)/2:plan-70) ||
       (kind==13 && pwm!=((plan-64)%2?-400:400)))) {
      server.send(400,"text/plain","Invalid prepared wheel test");return;
    }
    for(unsigned i=0;i<owner.length();i++)if(!isalnum((unsigned char)owner[i])) {server.send(400,"text/plain","Invalid owner");return;}
    if(kind>=12) {
      long client=-1,token=0,revision=-1;
      if(!strictNumber(server.arg("session"),client)||!strictNumber(server.arg("token"),token)||
         !controlCanClaim(client,token)||!strictNumber(server.arg("stop_revision"),revision)||
         revision<0||(uint32_t)revision!=controlRevision) {
        server.send(409,"text/plain","Stale request or lost control page. Refresh status and press START again.");return;
      }
      controlClaim(client);
    }
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    if(testStage<11 || testStage==13 || millis()-testLastReceived>2500 || jobDownloadBusy) {
      xSemaphoreGive(captureMutex);server.send(409,"text/plain","Wait for boot test or current job; check STM32 link.");return;
    }
    if(kind>=12 && (strncmp(balanceMeta,"$BINFO,BAL-20261015-R12,",sizeof("$BINFO,BAL-20261015-R12,")-1) ||
       (balanceSeen && millis()-balanceReceived<1000))) {
      xSemaphoreGive(captureMutex);server.send(409,"text/plain","Enter wheel measurement mode with matching STM32 firmware first.");return;
    }
    uint32_t id=++nextJobId;JobLog *j=jobLogs.reserve(id);
    if(!j){xSemaphoreGive(captureMutex);server.send(409,"text/plain","Storage full/unavailable. Download then explicitly delete an old log.");return;}
    if(!legacyFrozen) {
      snprintf(legacyMeta,sizeof(legacyMeta),"%s",testMeta);
      snprintf(legacyStop,sizeof(legacyStop),"%s",testStop);
      legacyCRC=testCRCError;legacyFrozen=true;
    }
    j->kind=kind;j->plan=(uint16_t)plan;snprintf(j->meta,sizeof(j->meta),"%s",testMeta);
    String note=server.arg("note");note.replace("\r"," ");note.replace("\n"," ");
    snprintf(j->note,sizeof(j->note),"%s",note.c_str());
    snprintf(measurementOwner,sizeof(measurementOwner),"%s",owner.c_str());
    jobRequestedAt=jobLastRx=millis();
    xSemaphoreGive(captureMutex);
    Serial1.printf("$TEST,%lu,%ld,%ld,%ld,%ld,%ld*\r\n",(unsigned long)id,kind,duration,pwm,reference,tag);
    server.send(200,"application/json","{\"id\":"+String(id)+"}");
  });
  server.on("/measurement-ping",HTTP_POST,[](){
    bool owns=server.arg("owner")==measurementOwner;
    if(owns)Serial1.print("$TPING*\r\n");
    server.send(owns?200:403,"text/plain",owns?"ok":"observer");
  });
  server.on("/measurement-stop",HTTP_POST,[](){
    Serial1.print("$STOP*\r\n");server.send(200,"text/plain","STOP requested");
  });
  server.on("/measurement-delete",HTTP_POST,[](){
    long id;if(!strictNumber(server.arg("id"),id)||server.arg("confirm")!="delete") {server.send(400,"text/plain","Confirm deletion");return;}
    xSemaphoreTake(captureMutex,portMAX_DELAY);bool erased=jobLogs.erase((uint32_t)id);xSemaphoreGive(captureMutex);
    server.send(erased?200:409,"text/plain",erased?"deleted":"Active/downloading/missing log cannot be deleted.");
  });
  server.on("/measurement-log",HTTP_GET,downloadMeasurement);
}
void measurementWatchdog() {
  xSemaphoreTake(captureMutex,portMAX_DELAY);bool stop=jobProtocolStop;jobProtocolStop=false;
  if(jobLogs.active>=0) {
    JobLog &j=jobLogs.jobs[jobLogs.active];
    if((j.phase==0 && millis()-jobRequestedAt>3000)||(j.phase==1 && millis()-jobLastRx>2500)) {
      jobLogs.failActive(512);stop=true;
    } else if(j.overflow)stop=true;
  }
  xSemaphoreGive(captureMutex);if(stop)Serial1.print("$STOP*\r\n");
}
