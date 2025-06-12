//address mapping: TTT TTT TII IBO OOO
#include "config.h"
#include <cmath>
#include <optional> 

enum origin{
    icache,
    dcache,
    TLB,
}

enum class miss_tpye {
    none,      
    tlb_miss,
    tag_miss
};

struct cache_lookup_result {
    cacheline line;     // valid only if hit == true
    MissType miss_reason; // meaningful only if hit == false
};

class icache{
struct cacheline {
    char data[cache_line_size];
    bool valid;
    bool dirty;
    int tag;
};

 cacheline cache[icache_banks][icache_sets][icache_ways];

 char lru[icache_banks][icache_sets]; //stores the least recently used way for each set for each bank

 icache(){
    for(int i = 0; i < icache_banks; i++){
        for(int j = 0; i < icache_sets; j++){
           for(int k = 0; i < icache_ways; k++){
            cache[i][j][k].valid = 0; //all start invalid
           }
        }
    }
 }
cacheline* icache_access(int addr){
    int bank_mask = (1 << icache_banks) - 1;
    int set_mask = (1 << icache_sets) - 1;

    int bank = (addr >> (std::log2(icache_banks))) & bank_mask;
    int set = (addr >> (std::log2(icache_sets))) & set_mask;

    int tag = (addr >> 8) & 0x007F; //not sure if this can change
    return cache[bank][set]; //returns the ways you need to search
}

 cache_lookup_result try_hit(int addr){
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
    mshr_preinserter(EIP, icache);
    return {{}, tag_miss};
}


};