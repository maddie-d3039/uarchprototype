#include "../inc/config.h"
enum origin{
    icache,
    dcache,
    TLB,
};
class tlb{
    struct tlb_entry{
        int valid;
        int vpn;
        int pfn;
    };
    tlb_entry tlb_table[8];

    int tlb_access(int addr){
        int incoming_vpn = (addr>>12)&0x7;
        for(tlb_entry entry : tlb_table){
            if(entry.valid && (entry.vpn == incoming_vpn)){
                return entry.pfn; //tlb hit
            }
        }
        translate_miss(incoming_vpn);
        return -1; //tlb miss 
    }

    void translate_miss(int vpn){
    int phys_addr = SBR + (vpn * pte_size);
    mshr_preinserter(phys_addr, TLB); //need to link this somehow
    }
};