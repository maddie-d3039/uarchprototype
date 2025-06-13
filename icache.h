#include "config.h"
#include <cmath>
#include <optional> 

enum origin{
    icache,
    dcache,
    TLB,
};

enum class miss_tpye {
    none,      
    tlb_miss,
    tag_miss
};

struct cache_lookup_result {
    cacheline line;     // valid only if hit == true
    int miss_reason; // meaningful only if hit == false
};

struct cacheline {
    char data[cache_line_size];
    bool valid;
    bool dirty;
    int tag;
};

class icache{
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
 
 cacheline* icache_access(int addr);
 cache_lookup_result try_hit(int addr);
};