#include "config.h"
#include "cpu.h"

class ibuffer{
    public:
    char buffer[ibuffer_size][cache_line_size];
    bool ibuffer_valid[ibuffer_size];
    int current_sector;
    bool bank_align_cur;
    bool bank_align_other;

    ibuffer(){
    current_sector = 0;
    bank_align_odd = false;
    bank_align_even = false;
    for(int i = 0; i < ibuffer_size; i++){
        ibuffer_valid[i] = 0;
        }
    }
    void operate(int EIP, cpu::PipeState_Entry& new_pipeline);
    void invalidate(int length, int EIP);
    

};