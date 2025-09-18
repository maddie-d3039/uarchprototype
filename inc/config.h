// #define num_control_store_bits
#include "cassert"

// icache
#define icache_banks 2
#define icache_sets 8
#define icache_ways 2
#define ibuffer_size 4
#define cache_line_size 16

// dcache
#define dcache_banks 2
#define dcache_sets 4
#define dcache_ways 4

// tlb
#define tlb_entries 8
#define pte_size 4

// reservation station
#define num_entries_per_RS 8

// MSHR
#define mshr_size 16
#define pre_mshr_size 8

// flag alias table
#define flag_alias_pool_entries 64
#define FAT_size 7 // cant change

// branch
#define spec_exe_tracker_size 4
#define history_length 8
#define starting_val 2 // weakly taken, 10

// serializer/deserializer
#define num_serializer_entries 8 // might not change
#define num_deserializer_entries 8

// ROB
#define rob_size 16

// PMEM
#define bytes_per_column 4
#define columns_per_row 16
#define rows_per_bank 128
#define banks_in_DRAM 4

// table pools
#define REGISTER_ALIAS_POOL_ENTRIES 64
#define FLAG_ALIAS_POOL_ENTRIES 64
#define MEMORY_ALIAS_POOL_ENTRIES 64

// Base offsets for each alias range
#define REGISTER_ALIAS_BASE 0
#define FLAG_ALIAS_BASE (REGISTER_ALIAS_BASE + REGISTER_ALIAS_POOL_ENTRIES) // 64
#define MEMORY_ALIAS_BASE (FLAG_ALIAS_BASE + FLAG_ALIAS_POOL_ENTRIES)       // 128

// VMEM
#define SBR 0x500

// serializers and deserializers
#define serializer_depth 2
#define deserializer_depth 2