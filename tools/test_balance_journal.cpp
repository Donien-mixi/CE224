#include <cassert>
#include <cstdio>
#include <vector>
#include "balance_wire.h"
#include "balance_journal.h"
static unsigned recover(const std::vector<unsigned char>&v,size_t size){
 if(size<sizeof(JournalHeader))return 0;
 JournalHeader h;memcpy(&h,v.data(),sizeof(h));if(!journalHeaderValid(h,sizeof(BalanceFrame)))return 0;
 size_t off=sizeof(h);unsigned rows=0,index=0;
 while(size-off>=sizeof(JournalChunk)){
  JournalChunk c;memcpy(&c,v.data()+off,sizeof(c));off+=sizeof(c);
  if(!journalChunkValid(c,index++,v.data()+off,size-off,sizeof(BalanceFrame)))break;
  rows+=c.frames;off+=c.bytes;
 }
 return rows;
}
int main(){
 assert(journalCRC("123456789",9)==0xcbf43926U);
 JournalHeader h={};h.magic=JOURNAL_MAGIC;h.version=1;h.frame_bytes=sizeof(BalanceFrame);h.boot_id=42;
 h.crc=journalCRC(&h,offsetof(JournalHeader,crc));assert(journalHeaderValid(h,sizeof(BalanceFrame)));
 std::vector<unsigned char> v((unsigned char*)&h,(unsigned char*)&h+sizeof(h));std::vector<size_t> ends;
 for(unsigned count:{50u,13u}){
  std::vector<unsigned char> payload(sizeof(JournalInfo)+count*sizeof(BalanceFrame),0x25);
  JournalChunk c={CHUNK_MAGIC,(uint32_t)ends.size(),count,(uint32_t)payload.size(),journalCRC(payload.data(),payload.size())};
  auto p=(unsigned char*)&c;v.insert(v.end(),p,p+sizeof(c));v.insert(v.end(),payload.begin(),payload.end());ends.push_back(v.size());
 }
 for(size_t cutoff=0;cutoff<=v.size();cutoff++){
  unsigned expected=cutoff>=ends[1]?63:cutoff>=ends[0]?50:0;
  assert(recover(v,cutoff)==expected);
 }
 auto broken=v;broken[sizeof(h)+sizeof(JournalChunk)+10]^=1;assert(!recover(broken,broken.size()));
 broken=v;broken[ends[0]+sizeof(JournalChunk)+10]^=1;assert(recover(broken,broken.size())==50);
 broken=v;JournalChunk bad={CHUNK_MAGIC,0,0xffffffffU,0xffffffffU,0};memcpy(broken.data()+sizeof(h),&bad,sizeof(bad));assert(!recover(broken,broken.size()));
 broken=v;broken[0]^=1;assert(!recover(broken,broken.size()));
 puts("PASS NET1 journal: CRC32 reference, both batches, every truncated byte boundary, payload corruption, header corruption and oversized counts; only committed prefix recovered.");
}
