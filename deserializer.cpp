#include "cpu.h"
#include "deserializer.h"

int deserializer::allocate(){
    for(deserializer_entry entry : entries){
        if(!entry.valid){
            entry.valid = 1;
            entry.recieving_data = true;
        }
    }
}

void deserializer::recieve(){ //recieve one byte into deserializer
    int burst_counter = metadata_bus[0];
    int address = metadata_bus[1];
    deserializer_entry active;
    for(deserializer_entry entry : entries){
        if(entry.valid && entry.recieving_data){
            active = entry;
        }
    }
    for(int i = 0; i < 4; i++){
        entry.data[i * (4 * burst_counter + 1)] = data_bus; 
    }
}

