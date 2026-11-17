#pragma once
#include <FFat.h>
#include <esp_partition.h>
#include "balance_journal.h"
/* Flash work never holds captureMutex or runs on the web task. At most four
 * archives; no automatic deletion. Existing nonblank/unmountable FAT is left
 * untouched. Committed CRC batches are best-effort recovery, not a power UPS. */
static SemaphoreHandle_t archiveMutex;
static uint8_t *archiveBuffer;
static volatile uint32_t archiveGeneration=0,archiveSaved=0,archiveError=0;
/* RT23 deferred recovery: f_mkfs on9.9MB takes tens of seconds, so the web task
 * only queues it. The archive task owns the call, which means the HTTP request
 * returns at once and a second click cannot stack two formats. */
static volatile bool archiveFormatRequest=false;
static volatile uint8_t archiveFormatState=0; /*0 idle,1 queued,2 running,3 ok,4 failed*/
static bool archiveMounted=false;
struct ArchiveEntry{char name[40];uint32_t frames;bool partial;};
static ArchiveEntry archiveEntries[4];static volatile unsigned archiveCount=0;
static char archiveCurrentName[40]={};
static const size_t ARCHIVE_PAYLOAD=sizeof(JournalInfo)+JOURNAL_BATCH*sizeof(BalanceFrame);
static bool archiveReadChunk(File &f,uint32_t index,JournalChunk &c){
 if(f.read((uint8_t*)&c,sizeof(c))!=sizeof(c)||c.frames>JOURNAL_BATCH||
    c.bytes>ARCHIVE_PAYLOAD||c.bytes<sizeof(JournalInfo))return false;
 if(f.read(archiveBuffer,c.bytes)!=c.bytes||!journalChunkValid(c,index,archiveBuffer,c.bytes,sizeof(BalanceFrame)))return false;
 for(unsigned j=0;j<c.frames;j++){
  BalanceFrame frame;memcpy(&frame,archiveBuffer+sizeof(JournalInfo)+j*sizeof(frame),sizeof(frame));
  if(frame.sync!=0x60a5||frame.controller!=12||testCRC((const uint8_t*)&frame,sizeof(frame)-2)!=frame.crc)return false;
 }
 return true;
}
static bool archiveOpenHeader(File &f,JournalHeader &h){return f.read((uint8_t*)&h,sizeof(h))==sizeof(h)&&journalHeaderValid(h,sizeof(BalanceFrame));}
static bool archiveFormatNow();
/* Listing only needs frame counts: walk chunk headers, never the 6 kB payload.
 * A torn tail still marks the archive partial; export re-validates every chunk. */
static uint32_t archiveWalkFrames(File &f,bool &partial){
 uint32_t frames=0,index=0;
 while(f.available()>=(int)sizeof(JournalChunk)){
  JournalChunk c;
  size_t got=f.read((uint8_t*)&c,sizeof(c));
  if(got!=sizeof(c)){partial=true;break;}
  if(c.magic!=CHUNK_MAGIC||c.index!=index||c.frames==0||c.frames>JOURNAL_BATCH||
     c.bytes<sizeof(JournalInfo)||c.bytes>ARCHIVE_PAYLOAD){partial=true;break;}
  if(!f.seek(f.position()+c.bytes)){partial=true;break;}
  frames+=c.frames;index++;
 }
 if(f.available())partial=true;
 return frames;
}
static void archiveScan(){
 archiveCount=0;File dir=FFat.open("/twip-net1");if(!dir)return;
 for(File f=dir.openNextFile();f&&archiveCount<4;f=dir.openNextFile()){
  if(f.isDirectory()){f.close();continue;}
  JournalHeader h;ArchiveEntry e={};
  if(archiveOpenHeader(f,h)){
   /* FFat returns the full path here; the page requests a bare file name. */
   const char *full=f.name(),*base=strrchr(full,'/');base=base?base+1:full;
   snprintf(e.name,sizeof(e.name),"%s",base);
   e.frames=archiveWalkFrames(f,e.partial);
   archiveEntries[archiveCount++]=e;
  }
  f.close();
 }
 dir.close();
}
static bool archivePartitionBlank(size_t limit){
 auto part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_FAT,"ffat");
 if(!part)return false;
 size_t span=part->size<(size_t)limit?part->size:(size_t)limit;
 uint8_t b[512];
 for(size_t at=0;at<span;at+=sizeof(b)){
  size_t n=fmin(sizeof(b),span-at);
  if(esp_partition_read(part,at,b,n)!=ESP_OK)return false;
  for(size_t i=0;i<n;i++)if(b[i]!=0xff)return false;
  if(at%4096==0)delay(1);
 }
 return true;
}
static void archiveTask(void *){
 File file;uint32_t generation=0xffffffffU,source=0,index=0;int entry=-1;
 for(;;){
  if(archiveFormatRequest){
   /* Never format while the vehicle balances: the RAM log is the only copy. */
   xSemaphoreTake(captureMutex,portMAX_DELAY);
   bool driving=balanceSeen&&(balanceLatest.state==3||balanceLatest.state==7)&&millis()-balanceReceived<500;
   xSemaphoreGive(captureMutex);
   archiveFormatRequest=false;
   if(driving){archiveFormatState=0;vTaskDelay(pdMS_TO_TICKS(20));continue;}
   archiveFormatState=2;
   Serial.printf("[LOG] archive format start (error=%lu)\n",(unsigned long)archiveError);
   bool ok=archiveFormatNow();
   archiveFormatState=ok?3:4;
   Serial.printf("[LOG] archive format %s error=%lu mounted=%d\n",ok?"ok":"FAILED",(unsigned long)archiveError,archiveMounted?1:0);
   if(file){file.close();file=File();}
   generation=0xffffffffU;source=0;index=0;entry=-1;archiveSaved=0;
   continue;
  }
  uint32_t count,gen;bool settled;JournalInfo info={};
  xSemaphoreTake(captureMutex,portMAX_DELAY);
  count=balanceCount;gen=archiveGeneration;
  bool live=balanceSeen&&(balanceLatest.state==3||balanceLatest.state==7)&&millis()-balanceReceived<500;
  settled=balanceSeen&&balanceLatest.state!=3&&balanceLatest.state!=7&&(int32_t)(balancePostUntil-millis())<=0;
  xSemaphoreGive(captureMutex);
  if(gen!=generation){
   xSemaphoreTake(archiveMutex,portMAX_DELAY);if(file)file.close();archiveCurrentName[0]=0;xSemaphoreGive(archiveMutex);
   generation=gen;source=0;index=0;entry=-1;archiveSaved=0;
  }
  /* NET34: khong ghi Flash khi xe dang can bang/chay.
   * Log RT32 (3)(5)(4)(5)(5)(5): ESP32 reset lien tuc, hai nguyen nhan da do duoc
   * (8 log reset=brownout, 1 log reset=interrupt_watchdog). Moi lan ghi Flash la mot
   * cua so cache bi tat va la luc FAT de bi hong neu nguon sut -> archive_error=1.
   * Vi vay trong luc chay chi giu log trong RAM (du ~6 phut), va ghi Flash MOT LAN
   * khi da dung (settled). Trang web cung chi cho tai log sau khi STOP. */
  if(count>source&&settled&&!live&&!archiveError){
   xSemaphoreTake(archiveMutex,portMAX_DELAY);
   if(entry<0){
    if(archiveCount>=4)archiveError=2;
    else if(FFat.freeBytes()<1500*1024)archiveError=3;
    else{
     char path[64];snprintf(path,sizeof(path),"/twip-net1/b%08lu_g%04lu.bin",(unsigned long)gatewayBootId,(unsigned long)gen);
     if(FFat.exists(path))archiveError=4; // never truncate an older capture
     else{
      file=FFat.open(path,FILE_WRITE);
      JournalHeader h={};h.magic=JOURNAL_MAGIC;h.version=1;h.frame_bytes=sizeof(BalanceFrame);
      h.boot_id=gatewayBootId;h.reset_reason=gatewayResetReason;snprintf(h.web_build,sizeof(h.web_build),"%s",TWIP_WEB_BUILD);h.crc=journalCRC(&h,offsetof(JournalHeader,crc));
      if(!file||file.write((uint8_t*)&h,sizeof(h))!=sizeof(h))archiveError=4;
      else{
       file.flush();entry=0;while(entry<4&&archiveEntries[entry].name[0])entry++;ArchiveEntry &e=archiveEntries[entry];e={};
       snprintf(e.name,sizeof(e.name),"b%08lu_g%04lu.bin",(unsigned long)gatewayBootId,(unsigned long)gen);archiveCount++;
       snprintf(archiveCurrentName,sizeof(archiveCurrentName),"%s",e.name);
      }
     }
    }
   }else if(!file){char p[64];snprintf(p,sizeof(p),"/twip-net1/%s",archiveEntries[entry].name);file=FFat.open(p,FILE_APPEND);if(!file)archiveError=4;}
   if(!archiveError){
    unsigned n=(unsigned)fmin(JOURNAL_BATCH,count-source);
    xSemaphoreTake(captureMutex,portMAX_DELAY);
    // A clear can occur between the first snapshot and this copy. Do not mix captures.
    if(archiveGeneration!=generation||balanceCount<source+n){xSemaphoreGive(captureMutex);xSemaphoreGive(archiveMutex);continue;}
    snprintf(info.meta,sizeof(info.meta),"%s",balanceMeta);snprintf(info.stop,sizeof(info.stop),"%s",gatewayStopRecord);
    info.captured=balanceCount;info.crc_errors=balanceCRC;info.gaps=balanceGaps;info.overflow=balanceOverflow;info.resets=balanceResets;
    memcpy(archiveBuffer,&info,sizeof(info));memcpy(archiveBuffer+sizeof(info),balanceFrames+source,n*sizeof(BalanceFrame));
    xSemaphoreGive(captureMutex);
    JournalChunk c={CHUNK_MAGIC,index,n,(uint32_t)(sizeof(info)+n*sizeof(BalanceFrame)),0};c.crc=journalCRC(archiveBuffer,c.bytes);
    bool ok=file.write((uint8_t*)&c,sizeof(c))==sizeof(c)&&file.write(archiveBuffer,c.bytes)==c.bytes;
    file.flush();
    if(ok){source+=n;index++;archiveSaved=source;archiveEntries[entry].frames=source;}
    else{archiveError=4;archiveEntries[entry].partial=true;file.close();}
   }
   xSemaphoreGive(archiveMutex);
  }
  if(settled&&source==count&&file){xSemaphoreTake(archiveMutex,portMAX_DELAY);file.close();xSemaphoreGive(archiveMutex);}
  vTaskDelay(pdMS_TO_TICKS(20));
 }
}
static bool archiveCreateResources(){
 if(!archiveMutex)archiveMutex=xSemaphoreCreateMutex();
 if(!archiveBuffer)archiveBuffer=(uint8_t*)malloc(ARCHIVE_PAYLOAD);
 return archiveMutex&&archiveBuffer;
}
static bool archiveMount(bool format){
 archiveMounted=FFat.begin(format,"/ffat",8);
 return archiveMounted;
}
static void archiveAttach(){
 if(!FFat.exists("/twip-net1")&&!FFat.mkdir("/twip-net1")){archiveError=4;return;}
 archiveScan();archiveError=0;archiveSaved=0;
}
static void archiveBegin(){
 archiveError=1;
 if(!archiveCreateResources()){archiveError=5;return;}
 /* RT23: only the first64kB decides whether this partition has ever been
  * formatted. The old probe read all9.9MB (20 224 flash reads +2 528 delay(1))
  * on every boot whose mount failed - and on this unit the mount has failed
  * every boot since the volume was corrupted (archive_error=1 in all8 logs),
  * so every power-on paid seconds of cache-disabled flash reads while the
  * STM32 could already be balancing. A formatted FFat volume always has its
  * wear-levelling sector map in the first sector, so64kB is decisive. */
 if(archiveMount(false))archiveAttach();
 else if(archivePartitionBlank(65536)&&archiveMount(true))archiveAttach();
 if(!archiveMounted)archiveError=1;
 if(xTaskCreatePinnedToCore(archiveTask,"logArchive",8192,nullptr,1,nullptr,0)!=pdPASS)archiveError=5;
}
/* Explicit operator recovery for a volume that cannot mount (archiveError=1).
 * Destructive by design: only the page may request it, after the operator has
 * downloaded what is still readable. */
static bool archiveFormatNow(){
 if(!archiveCreateResources()){archiveError=5;return false;}
 xSemaphoreTake(archiveMutex,portMAX_DELAY);
 if(archiveMounted){FFat.end();archiveMounted=false;} // f_mkfs needs the volume unmounted
 bool ok=archiveMount(true);
 if(ok&&!FFat.exists("/twip-net1")&&!FFat.mkdir("/twip-net1"))ok=false;
 if(ok){
  archiveScan();archiveError=0;archiveSaved=0;archiveCurrentName[0]=0;
  archiveGeneration++; // the writer task opens a fresh archive
 } else archiveError=1;
 xSemaphoreGive(archiveMutex);
 return ok;
}
