#include "config.h"
#include "cpu.h"
#include "ibuffer.h"

void ibuffer::ibuffer_operate(int EIP, cpu::PipeState_Entry& new_pipeline){
    int offset = EIP & 0x3F;
    int current_sector = offset/ibuffer_size;
    int line_offset = offset%cache_line_size;

    if(line_offset<=1){
        if(ibuffer_valid[current_sector]){
            //latch predecode valid and ibuffer
            new_pipeline.predecode_valid = true;
            new_pipeline.predecode_offset = offset;
            new_pipeline.predecode_current_sector = current_sector;
            new_pipeline.predecode_line_offset = line_offset;
            for(int i =0;i<ibuffer_size;i++){
                for(int j=0;j<cache_line_size;j++){
                    new_pipeline.predecode_ibuffer[i][j] = ibuffer::buffer[i][j];
                }
            }
        }else{
            new_pipeline.predecode_valid = false;
        }
    }
}
