#include "config.h"
#include "cpu.h"

void cpu::fetch_stage(){
    the_ibuffer->operate(EIP, &new_pipeline);
    new_pipeline.predecode_EIP = EIP;
    oldEIP = EIP;
    EIP += length;
    the_ibuffer->invalidate();
}
