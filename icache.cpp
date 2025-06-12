//address mapping: TTT TTT TII IBO OOO
#include "config.h"
#include "icache.h"


icache::cacheline* icache_access(int addr){
    int bank_mask = (1 << icache_banks) - 1;
    int set_mask = (1 << icache_sets) - 1;

    int bank = (addr >> (std::log2(icache_banks))) & bank_mask;
    int set = (addr >> (std::log2(icache_sets))) & set_mask;

    int tag = (addr >> 8) & 0x007F; //not sure if this can change
    return cache[bank][set]; //returns the ways you need to search
}

 icache::cache_lookup_result try_hit(int addr){
    cacheline* set = icache_access(addr);
    int tag = tlb_access(addr);
    if(tag == -1){
        return {{}, tlb_miss}; //return instantly if there's no tlb entry
    }
    for(int i = 0; i < icache_ways; i++){
        if(set[i].valid && (set[i].tag == tag)){
            return {set[i], none}; //this is the line you need to put into the ibuffer
        }
    }
    return {{}, tag_miss};
}


