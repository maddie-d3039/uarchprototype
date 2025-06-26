#include "serializer.h"
#include "cpu.h"
int serializer::allocate(int address, int data[cache_line_size]){
    for(serializer_entry entry : entries){
        if(!entry.valid){
            entry.valid = true;
            entry.address = address;
            for(int i = 0; i < cache_line_size; i++){
                entry.data[i] = data[i];
            }
            occupancy++;
            entry.age = occupancy;
            return 1; //success
        }
    }
    return 0; //no available entries
}

serializer_entry serializer::get_active_entry(){
    for(serializer_entry entry : entries){
        if((entry.old_bits == 1) && entry.valid){
            return entry;
        }
    }
}

void serializer::deallocate(int address){
    for(serializer_entry entry : entries){
        if((entry.old_bits == 1) && entry.valid){
            entry.valid = false;
        }
     }
     for(serializer_entry entry : entries){
        if(entry.valid){
            entry.old_bits--;
        }
     }

}

int serializer::send(){ //send one byte from serializer
     cpu::metadata_bus[0] = burst_counter;
     cpu::metadata_bus[1] = address;
     cpu::data_bus = 0
     serializer_entry entry = get_active_entry();
     for(int i = 0; i < 4){
        cpu::data_bus += entry.data[i + ((burst_counter * 4) + 1)] << (i * 8); //data is a char, data_bus is a byte. need to shift to alighn everything
     }
}