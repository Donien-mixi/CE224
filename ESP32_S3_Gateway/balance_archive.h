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
static void archiveScan(){
 archiveCount=0;File dir=FFat.open("/twip-net1");if(!dir)return;
 for(File f=dir.openNextFile();f&&archiveCount<4;f=dir.openNextFile()){
  if(f.isDirectory()){f.close();continue;}
  JournalHeader h;ArchiveEntry e={};
  if(archiveOpenHeader(f,h)){
   snprintf(e.name,sizeof(e.name),"%s",f.name());
   uint32_t index=0;while(f.available()){
    JournalChunk c;if(!archiveReadChunk(f,index++,c)){e.partial=true;break;}e.frames+=c.frames;delay(1);
   }
   archiveEntries[archiveCount++]=e;
  }
  f.close();
 }
 dir.close();
}
static bool archivePartitionBlank(){
 auto part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_FAT,"ffat");
 if(!part)return false;
 uint8_t b[512];
 for(size_t at=0;at<part->size;at+=sizeof(b)){
  size_t n=fmin(sizeof(b),part->size-at);
  if(esp_partition_read(part,at,b,n)!=ESP_OK)return false;
  for(size_t i=0;i<n;i++)if(b[i]!=0xff)return false;
  if(at%4096==0)delay(1);
 }
 return true;
}
static void archiveTask(void *){
 File file;uint32_t generation=0xffffffffU,source=0,index=0,lastFlush=0;int entry=-1;
 for(;;){
  uint32_t count,gen;bool settled;JournalInfo info={};
  xSemaphoreTake(captureMutex,portMAX_DELAY);
  count=balanceCount;gen=archiveGeneration;
  settled=balanceSeen&&balanceLatest.state!=3&&balanceLatest.state!=7&&(int32_t)(balancePostUntil-millis())<=0;
  xSemaphoreGive(captureMutex);
  if(gen!=generation){
   xSemaphoreTake(archiveMutex,portMAX_DELAY);if(file)file.close();archiveCurrentName[0]=0;xSemaphoreGive(archiveMutex);
   generation=gen;source=0;index=0;entry=-1;archiveSaved=0;
  }
  if(count>source&&(count-source>=JOURNAL_BATCH||settled||millis()-lastFlush>=1000)&&!archiveError){
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
    file.flush();lastFlush=millis();
    if(ok){source+=n;index++;archiveSaved=source;archiveEntries[entry].frames=source;}
    else{archiveError=4;archiveEntries[entry].partial=true;file.close();}
   }
   xSemaphoreGive(archiveMutex);
  }
  if(settled&&source==count&&file){xSemaphoreTake(archiveMutex,portMAX_DELAY);file.close();xSemaphoreGive(archiveMutex);}
  vTaskDelay(pdMS_TO_TICKS(20));
 }
}
static void archiveBegin(){
 bool blank=archivePartitionBlank();
 archiveMounted=FFat.begin(false,"/ffat",8);
 if(!archiveMounted&&blank)archiveMounted=FFat.begin(true,"/ffat",8);
 if(!archiveMounted){archiveError=1;return;}
 if(!FFat.exists("/twip-net1")&&!FFat.mkdir("/twip-net1")){archiveError=4;return;}
 archiveMutex=xSemaphoreCreateMutex();archiveBuffer=(uint8_t*)malloc(ARCHIVE_PAYLOAD);
 if(!archiveMutex||!archiveBuffer){archiveError=5;return;}
 archiveScan();
 if(xTaskCreatePinnedToCore(archiveTask,"logArchive",8192,nullptr,1,nullptr,0)!=pdPASS)archiveError=5;
}
