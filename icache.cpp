//address mapping: TTT TTT TII IBO OOO
#include "config.h"
class icache{
struct I$_TagStoreEntry_Struct{
    int valid_way0;
    int tag_way0;
    int dirty_way0;
    int valid_way1;
    int tag_way1;
    int dirty_way1;
    int lru; 
};

int icache_datastore[icache_banks][icache_sets][icache_ways][cache_line_size];
int icache_datastore[icache_banks][icache_sets][icache_ways][cache_line_size];
};