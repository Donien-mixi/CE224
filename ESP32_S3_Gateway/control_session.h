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
}
static void controlDisconnected(uint8_t client) {
  if(client>=WEBSOCKETS_SERVER_CLIENT_MAX)return;
  // -2 retains the lost-controller condition: observers cannot revive heartbeat.
  if(controlOwner==client) {controlStop("controller_disconnected");controlOwner=-2;}
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
        balanceLatest.controller==12&&balanceLatest.capture==0&&(balanceLatest.motion_flags&1U)&&
        balanceLatest.drive_epoch==p.epoch&&balanceLatest.fault==0&&
        !strncmp(balanceMeta,"$BINFO,BAL-20261018-R12-SPEED2-8DIR,",sizeof("$BINFO,BAL-20261018-R12-SPEED2-8DIR,")-1)&&
        balanceCapacity&&millis()-controlPings[client]<600;
      xSemaphoreGive(captureMutex);if(!ok)return false;
    }
    controlMoveSequence=p.sequence;return true;
  }
  if(command=="$BACKGROUND*" || command=="$LEAVE*") {
    if(controlOwner==client)
      controlStop(command=="$BACKGROUND*"?"controller_background":"controller_page_left");
    // Also invalidate an unclaimed HTTP ARM already in transit from this page.
    // An observer leaving never sends STOP to the active controller.
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
