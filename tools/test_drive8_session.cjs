// Execute the actual WebSocket session policy, not a mock of its authorization.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/motion_session');fs.mkdirSync(out,{recursive:true});
const source=fs.readFileSync(path.join(root,'ESP32_S3_Gateway/control_session.h'),'utf8').replace('#pragma once','');
const mock=String.raw`
#include <string>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include "balance_wire.h"
struct String:std::string{using std::string::string;bool startsWith(const char*s)const{return rfind(s,0)==0;}bool endsWith(const char*s)const{return size()>=strlen(s)&&compare(size()-strlen(s),strlen(s),s)==0;}String substring(size_t a,size_t b)const{return substr(a,b-a);}String(const std::string&s):std::string(s){}};
#define WEBSOCKETS_SERVER_CLIENT_MAX 5
static uint32_t now=10000;uint32_t millis(){return now;}
int captureMutex,portMAX_DELAY;bool held=false;
void xSemaphoreTake(int,int){assert(!held);held=true;}void xSemaphoreGive(int){assert(held);held=false;}
bool balanceSeen=true;uint32_t balanceReceived=10000;size_t balanceCapacity=18000,balanceCount=0;
BalanceFrame balanceLatest={};
char balanceMeta[256]="$BINFO,BAL-20261024-R12-8DIR-RT8,";
struct {String text;void print(const String&s){assert(!held);text+=s;}}Serial1;
struct {void sendTXT(uint8_t,const char*){}}webSocket;
`;
const checks=String.raw`
static bool move(int client,int dir,uint32_t epoch,uint32_t seq){char m[64];snprintf(m,sizeof(m),"$MOVE8,%d,0,%u,%u,0*",dir,epoch,seq);return controlMessage(client,m);}
int main(){
 controlConnected(0);assert(controlMessage(0,"$PING*"));assert(controlCanClaim(0,controlTokens[0]));controlClaim(0);
 uint32_t epoch=controlDriveEpoch;balanceLatest.state=3;balanceLatest.controller=12;balanceLatest.drive_epoch=epoch;balanceLatest.motion_flags=1;
 assert(move(0,1,epoch,1));assert(!move(0,1,epoch,1));assert(move(0,0,epoch,2));assert(!move(0,-1,epoch,1));
 controlConnected(1);assert(!controlMessage(1,"$PING*"));assert(!move(1,1,epoch,3));
 assert(!controlMessage(0,"$MOVE8,1,0,1,3*$STOP,0*"));assert(!controlMessage(0,"$MOVE8,1,0,1,4294967297,0*"));
 assert(!controlMessage(0,"$MOVE8,01,0,1,3,0*"));assert(!controlMessage(0,"$CMD,0.10,0.0*"));
 for(int phase=1;phase<=1;phase++){balanceLatest.capture=phase;assert(!move(0,1,epoch,3));}balanceLatest.capture=0;
 balanceLatest.state=7;assert(!move(0,1,epoch,3));balanceLatest.state=3;
 balanceLatest.motion_flags=0;assert(!move(0,1,epoch,3));balanceLatest.motion_flags=1;
 balanceLatest.fault=128;assert(!move(0,1,epoch,3));balanceLatest.fault=0;
 balanceCount=balanceCapacity;assert(move(0,1,epoch,3));assert(!move(0,0,epoch,3));balanceCount=0; // full log must not interrupt active travel; duplicate sequence still rejected
 now+=400;assert(!move(0,1,epoch,4));assert(move(0,0,epoch,4));balanceReceived=now;
 now+=300;balanceReceived=now;assert(!move(0,1,epoch,5)); // PING lease expired despite fresh STM32
 assert(controlMessage(0,"$PING*"));assert(move(0,-1,epoch,5));
 Serial1.text.clear();controlStop("stop_button_ws");assert(Serial1.text=="$STOP*\r\n");assert(!move(0,1,epoch,6));
 controlClaim(0);uint32_t next=controlDriveEpoch;assert(next!=epoch);assert(!move(0,1,next,1)); // no STM32 ARM acknowledgment yet
 balanceLatest.drive_epoch=next;assert(move(0,1,next,1));assert(!move(0,1,epoch,999));
 Serial1.text.clear();assert(!controlMessage(1,"$BACKGROUND*"));assert(Serial1.text.empty());
 // RT7: hiding/closing the page must not stop a balancing vehicle.
 assert(!controlMessage(0,"$BACKGROUND*"));assert(Serial1.text.empty());
 assert(!move(0,1,next,2)); // heartbeat was reset by the reconnect: no motor until the page pings again
 controlConnected(0);assert(controlMessage(0,"$PING*"));controlClaim(0);balanceLatest.drive_epoch=controlDriveEpoch;
 Serial1.text.clear();controlDisconnected(0);assert(Serial1.text.empty());assert(controlOwner==-1);
 // The seat is free: any connected page may refresh its own heartbeat, but only
 // the adopted owner can move the vehicle (checked in $MOVE8 above).
 assert(controlMessage(1,"$PING*"));
 assert(!move(1,1,balanceLatest.drive_epoch,7));
 // RT7: the next page adopts the RUNNING epoch/sequence instead of re-arming.
 controlConnected(0);
 assert(controlOwner==0);
 assert(controlDriveEpoch==balanceLatest.drive_epoch);
 controlClaim(0);assert(controlMessage(0,"$PING*"));balanceLatest.drive_epoch=controlDriveEpoch;balanceReceived=now;
 char extended[80];snprintf(extended,sizeof(extended),"$MOVE8,1,0,%u,1,1*",controlDriveEpoch);assert(controlMessage(0,extended));
 snprintf(extended,sizeof(extended),"$MOVE8,1,0,%u,2,2*",controlDriveEpoch);assert(!controlMessage(0,extended));
 snprintf(extended,sizeof(extended),"$MOVE8,1,0,%u,2,01*",controlDriveEpoch);assert(!controlMessage(0,extended));
 snprintf(extended,sizeof(extended),"$MOVE8,0,0,%u,2,0*",controlDriveEpoch);assert(controlMessage(0,extended));
 // RT6: a recovery (capture2) no longer drops the held press; startup still does.
 balanceLatest.capture=1;assert(!move(0,1,controlDriveEpoch,3));
 balanceLatest.capture=2;assert(move(0,1,controlDriveEpoch,3));balanceLatest.capture=0;
 assert(!held);puts("PASS real motion session: owner, canonical legacy/40/60 preset packets, epoch/seq replay, confirmed ARM, readiness, startup/recovery, live STM/PING, storage, unconditional STOP, observer isolation and background/disconnect.");
}
`;
const file=path.join(out,'test.cpp'),exe=path.join(out,'test.exe');fs.writeFileSync(file,mock+source+checks);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/g++.exe',['-std=c++17','-Wall','-Wextra','-Werror','-I'+path.join(root,'ESP32_S3_Gateway'),file,'-o',exe],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
