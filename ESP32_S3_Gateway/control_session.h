#include "drive8_protocol.h"
#pragma once
// Main-loop only: HTTP handlers and WebSocket callbacks share this session state.
// UART capture fields are read under captureMutex; no UART protocol changes.
static uint32_t controlTokens[WEBSOCKETS_SERVER_CLIENT_MAX]={};
static uint32_t controlPings[WEBSOCKETS_SERVER_CLIENT_MAX]={};
static uint32_t controlGeneration=0;
static uint32_t controlDriveEpoch=0,controlMoveSequence=0;
// Reject a delayed HTTP motor request if any STOP arrived since it was created.
static uint32_t controlRevision=0;
static int controlOwner=-1;
static uint32_t controlClaimUntil=0;
static bool controlStopLatched=false;  
static const unsigned CONTROL_STOP_CAPACITY=32;
static char controlStops[CONTROL_STOP_CAPACITY][128]={};
static unsigned controlStopCount=0,controlStopOverflow=0;
static char controlLastStop[32]="none";

static bool controlIdentity(long client,long token) {
  return client>=0 && client<WEBSOCKETS_SERVER_CLIENT_MAX && token>0 &&
    controlTokens[client]==(uint32_t)token;
}
static bool controlCanClaim(long client,long token) {
  return controlIdentity(client,token) && millis()-controlPings[client]<600 &&
    (controlOwner==client || (int32_t)(controlClaimUntil-millis())<=0);
}
static void controlClaim(long client) {
  controlOwner=(int)client;controlStopLatched=false;
  controlClaimUntil=millis()+2000;
  strcpy(controlLastStop,"none");
  controlDriveEpoch=controlDriveEpoch%1000000000U+1U;controlMoveSequence=0;
}
/* Reconnect policy: the vehicle may already be balancing with nobody at the
 * wheel. A new page takes the free seat WITHOUT bumping the epoch, because the
 * STM32 only accepts the epoch it received at ARM. Sequence numbering continues
 * from the last packet the STM32 acknowledged. A genuinely live owner (recent
 * heartbeat) keeps the wheel. */
static void controlAdoptIfFree(uint8_t client) {
  if (client>=WEBSOCKETS_SERVER_CLIENT_MAX) return;
  if (controlOwner>=0 && controlOwner<WEBSOCKETS_SERVER_CLIENT_MAX &&
      (uint32_t)(millis()-controlPings[controlOwner])<2000U) return;
  bool balancing=false; uint32_t epoch=0,sequence=0;
  xSemaphoreTake(captureMutex,portMAX_DELAY);
  balancing=balanceSeen&&(uint32_t)(millis()-balanceReceived)<1000U&&
    balanceLatest.state==3&&balanceLatest.controller==12&&balanceLatest.fault==0;
  if(balancing){epoch=balanceLatest.drive_epoch;sequence=balanceLatest.drive_sequence;}
  xSemaphoreGive(captureMutex);
  if(!balancing||!epoch)return;
  controlOwner=(int)client;controlClaimUntil=millis()+2000;
  controlDriveEpoch=epoch;
  if(controlMoveSequence<sequence)controlMoveSequence=sequence;
  controlStopLatched=false;strcpy(controlLastStop,"none");
}
static void controlStop(const char *source) {
  controlRevision=(controlRevision%2000000000u)+1;
  controlDriveEpoch=controlDriveEpoch%1000000000U+1U;controlMoveSequence=0;
  // The first gateway STOP per arm/pulse is retained; duplicate WS+HTTP STOP
  // still brakes, but cannot replace the original reason in the log.
  if(!controlStopLatched) {
    controlStopLatched=true;
    snprintf(controlLastStop,sizeof(controlLastStop),"%s",source);
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    uint32_t stmMs=balanceLatest.t_ms,seq=balanceLatest.sequence;
    xSemaphoreGive(captureMutex);
    if(controlStopCount<CONTROL_STOP_CAPACITY)
      snprintf(controlStops[controlStopCount++],sizeof(controlStops[0]),
        "# GSTOP,%lu,%lu,%lu,%s\n",(unsigned long)millis(),
        (unsigned long)stmMs,(unsigned long)seq,source);
    else controlStopOverflow++;
#ifdef ARDUINO
    if(controlStopCount)gatewayRecordStop(controlStops[controlStopCount-1]);
#endif
  }
  Serial1.print("$STOP*\r\n");
}
static void controlConnected(uint8_t client) {
  if(client>=WEBSOCKETS_SERVER_CLIENT_MAX)return;
  // Slot reuse receives a new generation; an old HTTP page cannot claim it.
  controlGeneration=(controlGeneration%2000000000u)+1;
  controlTokens[client]=controlGeneration;controlPings[client]=millis()-1000;
  char hello[80];snprintf(hello,sizeof(hello),"{\"session\":%u,\"token\":%lu}",
    client,(unsigned long)controlTokens[client]);webSocket.sendTXT(client,hello);
  controlAdoptIfFree(client);
}
static void controlDisconnected(uint8_t client) {
  if(client>=WEBSOCKETS_SERVER_CLIENT_MAX)return;
  /* Closing the page must not stop a balancing vehicle. The seat is freed so the
   * next page can adopt the running session; the 350 ms move lease already
   * brakes any held command. Only an explicit STOP cuts the motor. */
  if(controlOwner==client) controlOwner=-1;
  controlTokens[client]=0;
}
static bool controlMessage(uint8_t client,const String &command) {
  if(client>=WEBSOCKETS_SERVER_CLIENT_MAX || !controlTokens[client])return false;
  if(command=="$PING*") {
    controlPings[client]=millis();
    // An observer must not mask loss of the controlling page's heartbeat.
    return controlOwner==-1 || controlOwner==client;
  }
  if(command=="$STOP*") {controlStop("stop_button_ws");return false;}
  if(command.startsWith("$MOVE8,") && command.endsWith("*") && command.length()<64) {
    if(strlen(command.c_str())!=command.length())return false;
    String body=command.substring(0,command.length()-1);Drive8Packet p;
    if(!Drive8_Parse(body.c_str(),&p)||p.epoch!=controlDriveEpoch||p.sequence<=controlMoveSequence||
       controlOwner!=client||controlStopLatched)return false;
    if(p.direction!=0||p.turn!=0){
      xSemaphoreTake(captureMutex,portMAX_DELAY);
      bool ok=balanceSeen&&millis()-balanceReceived<300&&balanceLatest.state==3&&
        balanceLatest.controller==12&&balanceLatest.capture!=1&&(balanceLatest.motion_flags&1U)&&
        balanceLatest.drive_epoch==p.epoch&&balanceLatest.fault==0&&
        !strncmp(balanceMeta,"$BINFO,BAL-20261024-R12-8DIR-RT8,",sizeof("$BINFO,BAL-20261024-R12-8DIR-RT8,")-1)&&
        balanceCapacity&&millis()-controlPings[client]<600;
      xSemaphoreGive(captureMutex);if(!ok)return false;
    }
    controlMoveSequence=p.sequence;return true;
  }
  if(command=="$BACKGROUND*" || command=="$LEAVE*") {
    /* Leaving or hiding the page keeps the vehicle balancing and keeps the seat
     * for this page, so returning to the tab can drive again with the same epoch.
     * A held command is released by the page before sending this. */
    controlConnected(client);
    return false;
  }
  if(command=="$DIAG*" || command=="$CALIB*") {
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    bool stopped=balanceLatest.state!=3 && balanceLatest.state!=7;
    xSemaphoreGive(captureMutex);
    if(!stopped)return false;
    if(command=="$DIAG*")controlClaim(client);
    return true;
  }  
  // ARM/PULSE require the checked HTTP operation path.
  return command=="$CMD,0.00,0.0*" && controlOwner==client;
}  
