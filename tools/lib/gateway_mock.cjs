// tools/lib/gateway_mock.cjs — mock C cua gateway (nguon ban RT8) dung cho test_net1_routes.
// Chi chua chuoi mock, khong chay gi. File goc nam trong git history (tools/legacy/).
module.exports=String.raw`
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
char loadTestMeta[128]={};
char balanceMeta[256]="$BINFO,BAL-20261031-R12-8DIR-RT9-LOAD2,48,3.6,32,.5,.3,898.225,160,200,200,140,-.46,-.18,.46,2.166942649,1,7,.980,3";
unsigned controlDriveEpoch=0;
unsigned controlStopCount=0,controlStopOverflow=0,controlRevision=17;
char controlStops[32][128]={},controlLastStop[32]="none";
int controlOwner=-1;unsigned controlTokens[2]={123,0};bool identity=true;
bool controlCanClaim(long client,long token){return identity&&client==0&&token==123;}
#ifndef NET1_ROUTES_SKIP
bool controlIdentity(long client,long token){return identity&&client==0&&token==123;}
unsigned gatewayBootId=239,gatewayResetReason=3;const char* gatewayResetText(unsigned){return "brownout";}
struct {unsigned connectedClients(){return 0;}} webSocket;
bool archiveMounted=false;uint32_t archiveError=1;bool archiveFormatNow(){return false;}
#endif
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
