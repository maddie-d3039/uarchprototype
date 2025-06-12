#include "config.h"
#include "cpu.h"
   
void cpu::fetch_stage(){
    int offset = EIP & 0x3F;
int current_sector = offset/ibuffer_size;
int line_offset = offset%cache_line_size;
if(line_offset<=1){
    if(ibuffer_valid[current_sector]==TRUE){
        //latch predecode valid and ibuffer
        new_pipeline.predecode_valid=TRUE;
        new_pipeline.predecode_offset = offset;
        new_pipeline.predecode_current_sector = current_sector;
        new_pipeline.predecode_line_offset = line_offset;
        for(int i =0;i<ibuffer_size;i++){
            for(int j=0;j<cache_line_size;j++){
                new_pipeline.predecode_ibuffer[i][j]=ibuffer[i][j];
            }
        }
    }else{
        new_pipeline.predecode_valid=FALSE;
    }
}else{
    if((ibuffer_valid[current_sector]==TRUE)&&(ibuffer_valid[(current_sector+1)%ibuffer_size]==TRUE)){
        //latch predecode valid and ibuffer
        new_pipeline.predecode_valid=TRUE;
        new_pipeline.predecode_offset = offset;
        new_pipeline.predecode_current_sector = current_sector;
        new_pipeline.predecode_line_offset = line_offset;
        for(int i =0;i<ibuffer_size;i++){
            for(int j=0;j<cache_line_size;j++){
                new_pipeline.predecode_ibuffer[i][j]=ibuffer[i][j];
            }
        }
    }else{
        new_pipeline.predecode_valid=FALSE;
    }
}

}
