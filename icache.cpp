//address mapping: TTT TTT TII IBO OOO
#include "config.h"
#include <cmath>
#include <optional> 

class icache{
struct cacheline {
    char data[cache_line_size];
    bool valid;
    bool dirty;
    int tag;
};

 cacheline cache[icache_banks][icache_sets][icache_ways];
 char lru[icache_sets]; //stores the least recently used way for each set

 icache(){
    for(int i = 0; i < icache_banks; i++){
        for(int j = 0; i < icache_sets; j++){
           for(int k = 0; i < icache_ways; k++){
            cache[i][j][k].valid = 0; //all start invalid
           }
        }
    }
 }
cacheline icache_access(int addr){
    int bank_mask = (1 << icache_banks) - 1;
    int set_mask = (1 << icache_sets) - 1;

    int bank = (addr >> (std::log2(icache_banks))) & bank_mask;
    int set = (addr >> (std::log2(icache_sets))) & set_mask;

    int tag = (addr >> 8) & 0x007F; //not sure if this can change
    for(int i = 0; i < icache_ways; i++){
        if((tag == cache[bank][set][i].tag) && cache[bank][set][i].valid){
            return cache[bank][set][i]; //cache hit
        }
    }
    return std::nullopt;   //cache miss
}


};