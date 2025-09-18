#include "../inc/dcache.h"

cacheline *dcache::dcache_access(int addr)
{
    int bank_mask = (1 << dcache_banks) - 1;
    int set_mask = (1 << dcache_sets) - 1;

    int bank = (addr >> ((int)std::log2(dcache_banks))) & bank_mask;
    int set = (addr >> ((int)std::log2(dcache_sets))) & set_mask;

    int tag = (addr >> 8) & 0x007F; // not sure if this can change
    return cache[bank][set];        // returns the ways you need to search
}

cache_lookup_result dcache::dcache_try_hit(int addr)
{
    cacheline *set = dcache_access(addr);
    int tag = tlb_access(addr);
    if (tag == -1)
    {
        return {{}, tlb_miss}; // return instantly if there's no tlb entry
    }
    for (int i = 0; i < dcache_ways; i++)
    {
        if (set[i].valid && (set[i].tag == tag))
        {
            return {set[i], none}; // this is the line you need to put into the ibuffer
        }
    }
    return {{}, tag_miss};
}
