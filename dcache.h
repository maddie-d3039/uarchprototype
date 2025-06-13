#include "icache.h"

class dcache
{
    cacheline cache[dcache_banks][dcache_sets][dcache_ways];
    char lru[dcache_banks][dcache_sets];

    dcache()
    {
        for (int i = 0; i < dcache_banks; i++)
        {
            for (int j = 0; j < dcache_sets; j++)
            {
                for (int k = 0; k < dcache_ways; k++)
                {
                    cache[i][j][k].valid = 0;
                }
            }
        }
    }
};