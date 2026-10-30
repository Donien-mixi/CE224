// Compile actual journal readers and archive HTTP routes with a bounded fake FS.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/net1_routes');fs.mkdirSync(out,{recursive:true});
let mock=fs.readFileSync(path.join(__dirname,'test_drive8_gateway.cjs'),'utf8').match(/const mock=String.raw`([\s\S]*?)`;/)[1];
mock=mock.replace('void remove(size_t p){erase(p);}','void remove(size_t p){erase(p);} int indexOf(char c)const{auto i=find(c);return i==npos?-1:(int)i;} int indexOf(const char*c)const{auto i=find(c);return i==npos?-1:(int)i;}');
mock=mock.replace('int captureMutex,portMAX_DELAY,HTTP_GET=1,HTTP_POST=2;','bool archiveLocked=false;int captureMutex=0,archiveMutex=1,portMAX_DELAY,HTTP_GET=1,HTTP_POST=2;');
mock=mock.replace('void xSemaphoreTake(int,int){assert(!held);held=true;}','int xSemaphoreTake(int m,int){if(m){assert(!archiveLocked);archiveLocked=true;}else{assert(!held);held=true;}return 1;}');
mock=mock.replace('void xSemaphoreGive(int){assert(held);held=false;}','void xSemaphoreGive(int m){if(m){assert(archiveLocked);archiveLocked=false;}else{assert(held);held=false;}}');
const ext=String.raw`
#include <memory>
#include <cmath>
#include "balance_journal.h"
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
#define portENTER_CRITICAL(x) ((void)x)
#define portEXIT_CRITICAL(x) ((void)x)
int wifiHealthMux;
bool controlIdentity(long c,long t){return identity&&c==0&&t==123;}
unsigned gatewayBootId=8,gatewayResetReason=9,gatewayApConnect=1,gatewayApDisconnect=2,gatewayApStop=0,gatewayApReason=4,gatewayWsConnected=1,gatewayWsDisconnected=1,gatewayLoopGapMax=40;
const char*gatewayResetText(unsigned){return "brownout";}
struct{unsigned getFreeHeap(){return 100000;}unsigned getMinFreeHeap(){return 90000;}unsigned getFreePsram(){return 1000000;}}ESP;
struct{unsigned softAPgetStationNum(){return 1;}}WiFi;
struct{int active=-1;}jobLogs;
bool archiveMounted=true;unsigned archiveError=0,archiveSaved=3,archiveCount=1;
char archiveCurrentName[40]={};
struct ArchiveEntry{char name[40];uint32_t frames;bool partial;};ArchiveEntry archiveEntries[4]={};
unsigned char payloadBuffer[sizeof(JournalInfo)+50*sizeof(BalanceFrame)];unsigned char*archiveBuffer=payloadBuffer;
const size_t ARCHIVE_PAYLOAD=sizeof(payloadBuffer);
const char*FILE_READ="r";
struct File {
 std::shared_ptr<std::vector<unsigned char>>v;size_t at=0;
 explicit operator bool()const{return !!v;}bool available(){return v&&at<v->size();}
 size_t read(unsigned char*b,size_t n){if(!v)return 0;n=std::min(n,v->size()-at);memcpy(b,v->data()+at,n);at+=n;return n;}
 bool seek(size_t p){if(!v||p>v->size())return false;at=p;return true;}
 void close(){v.reset();}
};
struct{std::map<std::string,std::shared_ptr<std::vector<unsigned char>>> files;
 File open(const char*p,const char*){auto i=files.find(p);return i==files.end()?File{}:File{i->second,0};}
 bool remove(const char*p){return files.erase(p)!=0;}
}FFat;
uint16_t testCRC(const uint8_t*p,unsigned n){uint16_t c=0xffff;while(n--){c^=(uint16_t)*p++<<8;for(int j=0;j<8;j++)c=(c&0x8000)?(uint16_t)((c<<1)^0x1021):(uint16_t)(c<<1);}return c;}
struct Partition{size_t size;};static Partition part={1536};static std::vector<unsigned char> rawPartition(1536,0xff);static bool missingPartition=false,readError=false;
const int ESP_PARTITION_TYPE_DATA=1,ESP_PARTITION_SUBTYPE_DATA_FAT=2,ESP_OK=0;
const Partition*esp_partition_find_first(int,int,const char*){return missingPartition?nullptr:&part;}
int esp_partition_read(const Partition*,size_t at,void*b,size_t n){if(readError)return -1;memcpy(b,rawPartition.data()+at,n);return 0;}
void delay(unsigned){}
`;
const storage=fs.readFileSync('ESP32_S3_Gateway/balance_archive.h','utf8');
const readers=storage.slice(storage.indexOf('static bool archiveReadChunk'),storage.indexOf('static void archiveScan'));
const blank=storage.slice(storage.indexOf('static bool archivePartitionBlank'),storage.indexOf('static void archiveTask'));
const rows=fs.readFileSync('ESP32_S3_Gateway/balance_web.h','utf8').split('static uint8_t balanceMetaController')[0].replace('#pragma once','');
const routes=fs.readFileSync('ESP32_S3_Gateway/network_routes.h','utf8').replace('#pragma once','');
const checks=String.raw`
void check(bool partial){
 auto v=std::make_shared<std::vector<unsigned char>>();JournalHeader h={};h.magic=JOURNAL_MAGIC;h.version=1;h.frame_bytes=sizeof(BalanceFrame);h.boot_id=7;strcpy(h.web_build,"NET1");h.crc=journalCRC(&h,offsetof(JournalHeader,crc));
 auto p=(uint8_t*)&h;v->insert(v->end(),p,p+sizeof(h));
 for(unsigned block=0;block<2;block++){
  unsigned count=block?2:3;std::vector<unsigned char> payload(sizeof(JournalInfo)+count*sizeof(BalanceFrame),0);JournalInfo info={};strcpy(info.meta,"$BINFO,test,12");memcpy(payload.data(),&info,sizeof(info));
  for(unsigned i=0;i<count;i++){BalanceFrame f={};f.sync=0x60a5;f.controller=12;f.sequence=block*3+i;f.pitch=-1.25;f.crc=testCRC((uint8_t*)&f,sizeof(f)-2);memcpy(payload.data()+sizeof(info)+i*sizeof(f),&f,sizeof(f));}
  JournalChunk c={CHUNK_MAGIC,block,count,(uint32_t)payload.size(),journalCRC(payload.data(),payload.size())};p=(uint8_t*)&c;v->insert(v->end(),p,p+sizeof(c));v->insert(v->end(),payload.begin(),payload.end());
 }
 if(partial)v->resize(v->size()-13);
 FFat.files["/twip-net1/boot7.bin"]=v;archiveEntries[0]={};strcpy(archiveEntries[0].name,"boot7.bin");archiveEntries[0].frames=partial?3:5;archiveEntries[0].partial=partial;
 server.args={{"name","boot7.bin"}};request("/balance-archive");assert(server.status==200&&server.length==body.size());
 assert(body.find("boot_id=7")!=String::npos&&body.find("gateway_at_export_boot=8 reset=brownout")!=String::npos);
 unsigned count=0;for(size_t i=0;i<body.size();i++)if((i==0||body[i-1]=='\n')&&body[i]>='0'&&body[i]<='9')count++;assert(count==(partial?3:5));
 String footer="# END archive rows="+String(count)+" partial_tail="+(partial?"1":"0")+"\n";assert(body.substr(body.size()-footer.size())==footer);
 failAfter=0;request("/balance-archive");assert(stopped&&FFat.files.size()==1);failAfter=-1;
}
int main(){
 assert(archivePartitionBlank());rawPartition.back()=0;assert(!archivePartitionBlank());rawPartition.back()=0xff;
 readError=true;assert(!archivePartitionBlank());readError=false;missingPartition=true;assert(!archivePartitionBlank());missingPartition=false;
 setupNetworkRoutes();balanceLatest.state=2;
 check(false);check(true);
 balanceLatest.state=3;request("/balance-archive");assert(server.status==409);balanceLatest.state=2;
 server.args["name"]="../boot7.bin";request("/balance-archive");assert(server.status==404);
 server.args={{"name","boot7.bin"},{"session","0"},{"token","123"}};
 identity=false;request("/balance-archive-delete");assert(server.status==409&&FFat.files.size()==1);identity=true;
 strcpy(archiveCurrentName,"boot7.bin");request("/balance-archive-delete");assert(server.status==409&&FFat.files.size()==1);
 archiveCurrentName[0]=0;request("/balance-archive-delete");assert(server.status==200&&FFat.files.empty()&&archiveCount==0);
 request("/network-status");assert(server.status==200&&body.find("brownout")!=String::npos&&body.find("boot_id\":8")!=String::npos);
 assert(!held&&!archiveLocked);
 puts("PASS actual NET1 archive routes/readers: full and torn-tail CRC recovery, exact HTTP length/40-column rows, interrupted download retry, motor lock, path traversal, session/delete/current-log locks, no auto-delete, boot/reset diagnostics.");
}
`;
const file=path.join(out,'net1.cpp'),exe=path.join(out,'net1.exe');fs.writeFileSync(file,mock+ext+readers+blank+rows+routes+checks);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/g++.exe',['-std=c++17','-Wall','-Wextra','-Werror','-I'+path.join(root,'ESP32_S3_Gateway'),file,'-o',exe],{encoding:'utf8'});assert.equal(r.status,0,r.stderr||String(r.error));
r=cp.spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr||String(r.error));process.stdout.write(r.stdout);
