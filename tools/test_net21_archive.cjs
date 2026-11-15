// NET21: boot must not scan the whole 9.9 MB FFat partition when the volume
// mounts, an unmountable volume must still leave the recovery path usable, and
// a listed archive must carry a basename the page can request.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/net21_archive');
fs.mkdirSync(path.join(out,'inc'),{recursive:true});
const ino=fs.readFileSync(path.join(root,'ESP32_S3_Gateway/ESP32_S3_Gateway.ino'),'utf8');
const crcStart=ino.indexOf('uint16_t testCRC('),crcEnd=ino.indexOf('\n}',crcStart)+2;
assert(crcStart>0&&crcEnd>crcStart,'cannot extract testCRC from the gateway sketch');
const testCRC=ino.slice(crcStart,crcEnd);

fs.writeFileSync(path.join(out,'inc/FFat.h'),String.raw`
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <set>
enum { FILE_READ=0, FILE_WRITE=1, FILE_APPEND=2 };
struct MockEntry { std::string path; std::vector<uint8_t> data; };
extern std::vector<MockEntry> g_fs;
extern std::set<std::string> g_dirs;
extern bool g_mounted;
extern std::vector<int> g_beginFormat;
extern size_t g_freeBytes;
class File {
 public:
  std::string dir;int idx=-1;size_t pos=0;bool wr=false;unsigned cursor=0;
  File(){}
  explicit File(std::string d):dir(std::move(d)){}
  File(int i,bool w):idx(i),wr(w){}
  bool operator!() const { return dir.empty()&&idx<0; }
  explicit operator bool() const { return !dir.empty()||idx>=0; }
  bool isDirectory(){ return idx<0; }
  const char* name(){ return idx>=0?g_fs[idx].path.c_str():dir.c_str(); }
  int read(uint8_t*b,size_t n){ if(idx<0)return 0;size_t left=g_fs[idx].data.size()-pos;if(n>left)n=left;
   memcpy(b,g_fs[idx].data.data()+pos,n);pos+=n;return (int)n; }
  size_t write(const uint8_t*b,size_t n){ if(idx<0)return 0;auto &d=g_fs[idx].data;
   if(pos+n>d.size()){d.resize(pos+n);}
   memcpy(d.data()+pos,b,n);pos+=n;return n; }
  size_t position(){ return pos; }
  bool seek(size_t p){ if(idx<0)return false;pos=p;return true; }
  int available(){ if(idx<0)return 0;return (int)(g_fs[idx].data.size()-pos); }
  void flush(){}
  void close(){ dir.clear();idx=-1;pos=0; }
  File openNextFile(){
   if(idx>=0||dir.empty())return File();
   std::string prefix=dir+"/";unsigned seen=0;
   for(size_t i=0;i<g_fs.size();i++){
    if(g_fs[i].path.compare(0,prefix.size(),prefix))continue;
    if(seen++<cursor)continue;
    cursor++;return File((int)i,false);
   }
   return File();
  }
};
class MockFFat {
 public:
  bool begin(bool format,const char*,int){ g_beginFormat.push_back(format?1:0);
   if(format){g_fs.clear();g_dirs.clear();g_dirs.insert("/twip-net1");g_mounted=true;return true;}
   return g_mounted; }
  void end(){ g_mounted=false; }
  bool exists(const char*p){ for(auto &e:g_fs)if(e.path==p)return true;return g_dirs.count(p)>0; }
  bool mkdir(const char*p){ g_dirs.insert(p);return true; }
  bool remove(const char*p){ for(size_t i=0;i<g_fs.size();i++)if(g_fs[i].path==p){g_fs.erase(g_fs.begin()+i);return true;}return false; }
  size_t freeBytes(){ return g_freeBytes; }
  File open(const char*p,int mode=FILE_READ){
   if(g_dirs.count(p))return File(std::string(p));
   for(size_t i=0;i<g_fs.size();i++)if(g_fs[i].path==p){
    if(mode==FILE_WRITE){g_fs[i].data.clear();}
    File f((int)i,mode!=FILE_READ);
    if(mode==FILE_APPEND){f.pos=g_fs[i].data.size();}
    return f;
   }
   if(mode==FILE_READ)return File();
   g_fs.push_back({p,{}});File f((int)g_fs.size()-1,true);
   if(mode==FILE_APPEND){f.pos=g_fs.back().data.size();}
   return f;
  }
};
static MockFFat FFat;
`);
fs.writeFileSync(path.join(out,'inc/esp_partition.h'),String.raw`
#pragma once
#include <cstdint>
#include <cstring>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_PARTITION_TYPE_DATA 1
#define ESP_PARTITION_SUBTYPE_DATA_FAT 0
struct esp_partition_t { size_t size; };
extern esp_partition_t g_part; extern bool g_blank; extern int g_partReadCalls;
inline const esp_partition_t* esp_partition_find_first(int,int,const char*){ return &g_part; }
inline esp_err_t esp_partition_read(const esp_partition_t*,size_t,void*dst,size_t n){
 g_partReadCalls++;memset(dst,g_blank?0xff:0x00,n);return ESP_OK;}
`);

const mock=String.raw`
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include <set>
#include <math.h>
#include "FFat.h"
#include "esp_partition.h"
std::vector<MockEntry> g_fs;std::set<std::string> g_dirs;bool g_mounted=false,g_blank=false;
int g_partReadCalls=0;std::vector<int> g_beginFormat;size_t g_freeBytes=8u*1024u*1024u;
esp_partition_t g_part={0x9E0000};
typedef void* SemaphoreHandle_t;typedef void* TaskHandle_t;
#define portMAX_DELAY 0xffffffffu
#define pdTRUE 1
#define pdPASS 1
#define pdMS_TO_TICKS(x) (x)
SemaphoreHandle_t xSemaphoreCreateMutex(){return (SemaphoreHandle_t)1;}
int xSemaphoreTake(SemaphoreHandle_t,unsigned){return pdTRUE;}
int xSemaphoreGive(SemaphoreHandle_t){return pdTRUE;}
int xTaskCreatePinnedToCore(void(*)(void*),const char*,unsigned,void*,unsigned,TaskHandle_t*,int){return pdPASS;}
void vTaskDelay(unsigned){}
void yield(){}
unsigned millis(){return 1000;}
void delay(unsigned){}
SemaphoreHandle_t captureMutex=(SemaphoreHandle_t)2;
struct{void printf(const char*,...){}void begin(unsigned){}void println(const char*){}} Serial;
struct BalanceLatest { uint8_t state; } balanceLatest;
bool balanceSeen=false;size_t balanceCount=0;uint32_t balanceReceived=0;uint32_t balancePostUntil=0,balanceCRC=0,balanceGaps=0,balanceOverflow=0,balanceResets=0;
BalanceFrame *balanceFrames=nullptr;
char balanceMeta[256]={},gatewayStopRecord[128]={};
uint32_t gatewayBootId=239,gatewayResetReason=3;const char TWIP_WEB_BUILD[]="TEST-NET21";
`;
const scenarioMain=String.raw`
static int fails=0;
#define CHECK(c) do{ if(!(c)){ printf("FAIL line %d: %s\n",__LINE__,#c); fails++; } }while(0)
static void addJournal(const char*path,unsigned frames){
 g_dirs.insert("/twip-net1");
 JournalHeader h={};h.magic=JOURNAL_MAGIC;h.version=1;h.frame_bytes=sizeof(BalanceFrame);
 h.boot_id=7;h.reset_reason=1;snprintf(h.web_build,sizeof(h.web_build),"%s",TWIP_WEB_BUILD);
 h.crc=journalCRC(&h,offsetof(JournalHeader,crc));
 MockEntry e;e.path=path;
 e.data.insert(e.data.end(),(uint8_t*)&h,(uint8_t*)&h+sizeof(h));
 unsigned left=frames,c=0;
 while(left){
  unsigned n=left>JOURNAL_BATCH?JOURNAL_BATCH:left;left-=n;
  std::vector<uint8_t> payload(sizeof(JournalInfo)+n*sizeof(BalanceFrame));
  JournalInfo info={};snprintf(info.meta,sizeof(info.meta),"$BINFO,test");snprintf(info.stop,sizeof(info.stop),"none");
  memcpy(payload.data(),&info,sizeof(info));
  for(unsigned i=0;i<n;i++){
   BalanceFrame f{};f.sync=0x60a5;f.controller=12;f.sequence=i;f.crc=testCRC((uint8_t*)&f,sizeof(f)-2);
   memcpy(payload.data()+sizeof(info)+i*sizeof(f),&f,sizeof(f));
  }
  JournalChunk ch={CHUNK_MAGIC,c++,n,(uint32_t)payload.size(),journalCRC(payload.data(),payload.size())};
  e.data.insert(e.data.end(),(uint8_t*)&ch,(uint8_t*)&ch+sizeof(ch));
  e.data.insert(e.data.end(),payload.begin(),payload.end());
 }
 g_fs.push_back(std::move(e));
}
int main(int argc,char**argv){
 int sc=argc>1?atoi(argv[1]):0;
 if(sc==1){
  g_mounted=true;addJournal("/twip-net1/b00000007_g0001.bin",120);
  addJournal("/twip-net1/b00000007_g0002.bin",50);
  archiveBegin();
  CHECK(archiveError==0);CHECK(archiveMounted);CHECK(archiveCount==2);
  CHECK(g_partReadCalls==0);                       /* healthy path never scans the partition */
  CHECK(g_beginFormat.size()==1&&g_beginFormat[0]==0);
  CHECK(archiveMutex!=nullptr&&archiveBuffer!=nullptr);
  CHECK(archiveEntries[0].frames==120&&!archiveEntries[0].partial);
  CHECK(archiveEntries[1].frames==50&&!archiveEntries[1].partial);
  CHECK(strchr(archiveEntries[0].name,'/')==nullptr); /* the page can request this name */
  CHECK(!strcmp(archiveEntries[0].name,"b00000007_g0001.bin"));
 }
 if(sc==2){
  g_mounted=false;g_blank=false;
  archiveBegin();
  CHECK(archiveError==1);CHECK(!archiveMounted);
  CHECK(g_partReadCalls>0);                        /* blank check only after a failed mount */
  CHECK(g_beginFormat.size()==1&&g_beginFormat[0]==0);
  CHECK(archiveMutex!=nullptr&&archiveBuffer!=nullptr); /* recovery still possible */
  CHECK(archiveFormatNow());
  CHECK(archiveMounted&&archiveError==0&&archiveCount==0);
 }
 if(sc==3){
  g_mounted=false;g_blank=true;                    /* first boot on a fresh partition */
  archiveBegin();
  CHECK(archiveMounted&&archiveError==0);
  CHECK(g_beginFormat.size()==2&&g_beginFormat[0]==0&&g_beginFormat[1]==1);
 }
 if(fails)return 1;
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'archive.cpp');
fs.writeFileSync(file,['#include "balance_wire.h"',mock,testCRC,'#include "balance_archive.h"',scenarioMain].join('\n'));
let r=cp.spawnSync('C:/msys64/ucrt64/bin/g++.exe',['-std=c++17','-Wall','-Wextra','-Werror','-Wno-sign-compare','-Wno-unused-function','-I'+path.join(out,'inc'),'-I'+path.join(root,'ESP32_S3_Gateway'),file,'-o',path.join(out,'archive.exe')],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
for(const sc of ['1','2','3']){
 r=cp.spawnSync(path.join(out,'archive.exe'),[sc],{encoding:'utf8'});
 assert.equal(r.status,0,`scenario ${sc}:\n`+(r.stdout||'')+(r.stderr||''));
}
console.log('PASS NET21 archive: fast mount, recoverable unmountable volume, basename listing, blank-partition format.');
