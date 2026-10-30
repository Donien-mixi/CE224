// Run the actual HTTP handlers and CSV exporter, including mode locks and full transfers.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/r12_gateway');fs.mkdirSync(out,{recursive:true});
const source=fs.readFileSync(path.join(root,'ESP32_S3_Gateway/balance_web.h'),'utf8');
const mock=String.raw`
#include <string>
#include <map>
#include <functional>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include "balance_wire.h"
struct String:std::string {
 using std::string::string;String(const std::string&s):std::string(s){}
 String(unsigned n):std::string(std::to_string(n)){} String(long n):std::string(std::to_string(n)){} String(int n):std::string(std::to_string(n)){}
 void remove(size_t p){erase(p);}
};
static bool held=false;static unsigned now=10000;
unsigned millis(){return now;}void yield(){}
int captureMutex,portMAX_DELAY,HTTP_GET=1,HTTP_POST=2;
void xSemaphoreTake(int,int){assert(!held);held=true;}
void xSemaphoreGive(int){assert(held);held=false;}
bool balanceSeen=true,balanceDownloading=false;
BalanceFrame balanceLatest={},*balanceFrames;
size_t balanceCount,balanceCapacity;
uint32_t balanceReceived=10000,balancePostUntil=0,balanceCRC=0,balanceGaps=0,balanceOverflow=0,balanceResets=0;
char balanceMeta[256]="$BINFO,BAL-20261024-R12-8DIR-RT8,48,3.6,32,.5,.3,898.225,160,200,200,140,-.46,-.18,.46,2.166942649,1,7,.980,3";
unsigned controlDriveEpoch=0;
unsigned controlStopCount=0,controlStopOverflow=0,controlRevision=17;
char controlStops[32][128]={},controlLastStop[32]="none";
int controlOwner=-1;unsigned controlTokens[2]={123,0};bool identity=true;
bool controlCanClaim(long client,long token){return identity&&client==0&&token==123;}
void controlClaim(long client){controlOwner=client;controlDriveEpoch++;}
struct {String text;void print(const String&s){assert(!held);text+=s;}}Serial1;
void controlStop(const char*){controlRevision++;Serial1.print("$STOP*\r\n");}
bool strictNumber(const String&s,long&n){char*end;n=strtol(s.c_str(),&end,10);return !s.empty()&&!*end;}
String body;unsigned calls=0;int failAfter=-1;bool stopped=false;
struct WiFiClient {void stop(){stopped=true;}};
bool sendMeasurementBytes(WiFiClient&,const char*p,size_t n){assert(!held);assert(balanceDownloading);if(failAfter>=0&&(int)calls++>=failAfter)return false;body.append(p,n);return true;}
struct {
 std::map<std::string,std::function<void()>> handlers;std::map<std::string,String> args;size_t length=0;int status=0;String disposition;
 void on(const char*p,int,std::function<void()>f){handlers[p]=f;}
 String arg(const char*k){return args[k];}
 void sendHeader(const char*k,const String&v){if(!strcmp(k,"Content-Disposition"))disposition=v;}
 void setContentLength(size_t n){length=n;}
 void send(int code,const char*,const String&s){assert(!held);status=code;body=s;}
 WiFiClient client(){return {};}
}server;
void request(const char*p){body.clear();calls=0;stopped=false;Serial1.text.clear();server.handlers.at(p)();assert(!held&&!balanceDownloading);}
void operation(const char*action,int mode=12){server.args={{"action",action},{"session","0"},{"token","123"},{"stop_revision","17"},{"controller",String((unsigned)mode)},{"surface","floor"},{"fixture","yes"},{"wheel","1"},{"direction","1"}};}
`;
const checks=String.raw`
int main(){setupBalanceRoutes();std::vector<BalanceFrame> frames(18000);balanceFrames=frames.data();balanceCapacity=frames.size();balanceLatest.state=2;balanceLatest.controller=7;balanceLatest.ready=1;
 operation("profile",7);request("/operate");assert(server.status==400&&Serial1.text.empty());
 operation("profile",12);request("/operate");assert(server.status==400&&Serial1.text.empty());
 operation("arm",7);request("/operate");assert(server.status==400&&Serial1.text.empty());
 operation("arm",12);request("/operate");assert(server.status==409&&Serial1.text.empty());
 balanceLatest.controller=12;request("/operate");assert(server.status==409); // old BINFO blocked
 strcpy(balanceMeta,"$BINFO,BAL-20261024-R12-8DIR-RT8,9.150813,6.128855,14.327945,1.160484,.25,898.225,160,200,200,140,-.46,-.18,.46,2.166942649,1,12,.998,60");
 request("/operate");assert(server.status==200&&Serial1.text=="$ARM,12,1*\r\n");
 for(unsigned state:{3u,7u}){balanceLatest.state=state;operation("arm",12);request("/operate");assert(server.status==409&&Serial1.text.empty());}balanceLatest.state=2;
 balanceDownloading=true;request("/operate"); // request asserts no downloading; handle explicitly below
 return 0;
}
`;
// Remaining scenarios use direct handlers when testing a pre-existing download lock.
const expanded=checks.replace(' balanceDownloading=true;request("/operate"); // request asserts no downloading; handle explicitly below\n return 0;',String.raw`
 balanceDownloading=true;Serial1.text.clear();server.handlers.at("/operate")();assert(server.status==409&&Serial1.text.empty());balanceDownloading=false;
 identity=false;request("/operate");assert(server.status==409);identity=true;
 server.args["stop_revision"]="16";request("/operate");assert(server.status==409);server.args["stop_revision"]="17";
 for(const char*bad:{"7","8","12x","","7,12"}){server.args["controller"]=bad;request("/operate");assert(server.status==400);}operation("arm",12);
 now+=600;request("/operate");assert(server.status==409);balanceReceived=now;
 operation("stop");identity=false;request("/operate");assert(server.status==200&&Serial1.text=="$STOP*\r\n");identity=true;
 // Full sized logs, both identities, fixed HTTP length, no overwritten frames.
 for(unsigned mode:{7u,12u}){
  for(unsigned i=0;i<frames.size();i++){frames[i].sequence=i;frames[i].t_ms=i*10;frames[i].state=3;frames[i].pitch=-1.25f;frames[i].ticks_r=-100;frames[i].controller=mode;}
  for(size_t count:{1u,150u,18000u}){balanceCount=count;request("/balance-log");assert(server.status==200&&server.length==body.size()&&!stopped);
   assert(body.find("# TWIP_BALANCE_RT4_V1")==0);assert(server.disposition=="attachment; filename=twip_balance_R"+String(mode)+".txt");
   assert(body.find("control_integral,stm32_drops,controller,control_phase,distance_limit_m,x_ref_m,v_ref_mps,drive_epoch,motion_flags,drive_direction,psi_ref_rad,w_ref_radps,turn_direction,drive_sequence,drive_rx_ms\n")!=String::npos);
   String footer="# END balance rows="+String((unsigned)count)+"\n";assert(body.substr(body.size()-footer.size())==footer);
   size_t rows=0,start=0;while(start<body.size()){size_t end=body.find('\n',start);assert(end!=String::npos);if(body[start]>='0'&&body[start]<='9'){rows++;unsigned commas=0;for(size_t j=start;j<end;j++)commas+=body[j]==',';assert(commas==39);assert(body.substr(start,end-start).find(","+String(mode)+",0,")!=String::npos);}start=end+1;}assert(rows==count);
  }
 }
 frames[0].controller=7;balanceCount=150;request("/balance-log");assert(server.disposition=="attachment; filename=twip_balance_MIXED.txt");
 failAfter=0;request("/balance-log");assert(stopped&&balanceCount==150);failAfter=-1;request("/balance-log");assert(server.length==body.size()&&!stopped);
 for(unsigned state:{3u,7u}){balanceLatest.state=state;request("/balance-log");assert(server.status==409);}balanceLatest.state=2;
 balancePostUntil=now+100;request("/balance-log");assert(server.status==409);balancePostUntil=0;
 now+=600;request("/balance-log");assert(server.status==409);balanceReceived=now;
 request("/balance-clear");assert(server.status==200&&balanceCount==0);operation("arm",12);server.args["stop_revision"]=String((unsigned)controlRevision);request("/operate");assert(server.status==200);
 puts("PASS R12-only production gateway: R7 ARM/switch rejected; STM/meta acknowledgment; authentication/revision, running/download/log/tail/stale locks; STOP independent; 1/150/18000-row exact-length V6 downloads, mode filenames and per-row IDs, mixed/reset evidence and transfer retry.");
 return 0;`);
const file=path.join(out,'gateway.cpp'),exe=path.join(out,'gateway.exe');fs.writeFileSync(file,mock+source.replace('#pragma once','')+expanded);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/g++.exe',['-std=c++17','-Wall','-Wextra','-Werror','-I'+path.join(root,'ESP32_S3_Gateway'),file,'-o',exe],{encoding:'utf8'});assert.equal(r.status,0,r.stderr||String(r.error));
r=cp.spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr||String(r.error));process.stdout.write(r.stdout);
