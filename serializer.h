#include "config.h"
class serializer{
    struct serializer_entry{
        int address;
        int age;
        bool valid;
        bool in_flight;
        char data[cache_line_size];

    };
    int burst_counter;
    int occupancy;
    serializer_entry entries[serializer_depth];
    
    serializer(){
        for(serializer_entry entry: entries){
            entry.valid = false;
            entry.age = 0;
            entry.in_flight = false;
            entry.address = 0;
        }
        occupancy = 0;
    }
    
    void send();
    int allocate(int address, int data[cache_line_size]);
    serializer_entry get_active_entry();
};