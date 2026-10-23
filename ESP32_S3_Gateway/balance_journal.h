#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* Append-only, independently CRC checked batches. A torn last batch is never
 * exported as valid samples. Mount/filesystem survival still depends on power. */
static const uint32_t JOURNAL_MAGIC=0x314e5754U,CHUNK_MAGIC=0x31435754U;
static const unsigned JOURNAL_BATCH=50;
struct __attribute__((packed)) JournalHeader {
 uint32_t magic,version,frame_bytes,boot_id,reset_reason;
 char web_build[80];uint32_t crc;
};
struct __attribute__((packed)) JournalInfo {
 char meta[256],stop[128];uint32_t captured,crc_errors,gaps,overflow,resets;
};
struct __attribute__((packed)) JournalChunk {
 uint32_t magic,index,frames,bytes,crc;
};
static inline uint32_t journalCRC(const void *data,size_t n){
 const uint8_t *p=(const uint8_t*)data;uint32_t c=0xffffffffU;
 while(n--){c^=*p++;for(int i=0;i<8;i++)c=(c>>1)^(0xedb88320U&(0U-(c&1U)));}
 return ~c;
}
static inline bool journalHeaderValid(const JournalHeader &h,size_t frameBytes){
 return h.magic==JOURNAL_MAGIC&&h.version==1&&h.frame_bytes==frameBytes&&
   h.crc==journalCRC(&h,offsetof(JournalHeader,crc));
}
static inline bool journalChunkValid(const JournalChunk &h,uint32_t index,
 const void *payload,size_t available,size_t frameBytes){
 return h.magic==CHUNK_MAGIC&&h.index==index&&h.frames>0&&h.frames<=JOURNAL_BATCH&&
   h.bytes==sizeof(JournalInfo)+h.frames*frameBytes&&h.bytes<=available&&
   h.crc==journalCRC(payload,h.bytes);
}
