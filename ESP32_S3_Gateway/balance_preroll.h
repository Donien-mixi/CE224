#pragma once
#include <stddef.h>
#include "balance_wire.h"
// Fixed memory, chronological copy, no allocation/overwrite of the saved log.
struct BalancePreRoll {
    static constexpr size_t depth=100; // One second of 100Hz pre-ARM snapshots.
    BalanceFrame frames[depth];
    size_t next=0,count=0;
    void clear(){next=count=0;}
    void push(const BalanceFrame &f){frames[next]=f;next=(next+1)%depth;if(count<depth)count++;}
    size_t appendTo(BalanceFrame *dest,size_t used,size_t capacity,uint32_t now) {
        // Reserve room for the active frame; never displace already saved samples.
        size_t room=(dest&&used<capacity)?capacity-used-1:0;
        size_t take=count<room?count:room;
        size_t start=(next+depth-take)%depth;
        for(size_t i=0;i<take;i++) {
            const BalanceFrame &f=frames[(start+i)%depth];
            if((uint32_t)(now-f.t_ms)<=1000U)dest[used++]=f;
        }
        clear();return used;
    }
};
