#include "config.h"
#include "cpu.h"
#include "ibuffer.h"

void ibuffer::operate(int EIP, cpu::PipeState_Entry& new_pipeline){

    int offset = EIP & 0x3F;
    int current_sector = offset / ibuffer_size;
    int line_offset = offset % cache_line_size;

    //first update predecode latches
    if((ibuffer_valid[current_sector] && (offset < 2)) || (ibuffer_valid[current_sector]) && (ibuffer_valid[(current_sector+1) % ibuffer_size])){
        //checks which sectors you need valid to latch predecode 
        //(need only current sector if offset <= 1, need current sector and next sector if the instruction might cross cache lines)
        new_pipeline.predecode_valid = true;
        new_pipeline.predecode_offset = offset;
        new_pipeline.predecode_current_sector = current_sector;
        new_pipeline.predecode_line_offset = line_offset;
        for(int i = 0; i < ibuffer_size; i++){
            for(int j = 0; j < cache_line_size; j++){
                new_pipeline.predecode_ibuffer[i][j] = buffer[i][j];
            }
        }
    }
    else{
        new_pipeline.predecode_valid = false;
    }

    //second bring in new cachelines 
    for(int i = current_sector; i < ibuffer_size; (i = ((i + 1) % ibuffer_size))){ //priority to current sector
        if(!ibuffer_valid[i]){ //if there's in invalid entry
             icache::cache_lookup_result result = cpu::the_icache->try_hit(EIP);
             if(result.miss_reason == none){
                if(i % 2){
                    if(!bank_align_odd){
                        for(int j = 0; j < cache_line_size; j++){ //copy over the cacheline if there was a hit in the cache
                        buffer[i][j] = result.cacheline[i];
                        }
                        bank_align_odd = true;
                    }
                }
                else{
                    if(!bank_align_even){
                        for(int j = 0; j < cache_line_size; j++){ //copy over the cacheline if there was a hit in the cache
                        buffer[i][j] = result.cacheline[i];
                        }
                        bank_align_even = true;
                    }
                }
             }
             if(result.miss_reason == tag_miss){
                mshr_preinserter(EIP, icache);
             }
             if(result.miss_reason == tlb_miss){
                //something to stall? 
             }
        }
    }

    ibuffer::invalidate(int length, int EIP){
        int offset = EIP % cache_line_size;
        if(length >= (cache_line_size - offset)){
            ibuffer_valid[current_sector] = false;
        }
    }

    



}
