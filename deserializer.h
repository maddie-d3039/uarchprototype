#include "config.h"
class deserializer{
    struct deserializer_entry{
        int address;
        int age;
        bool valid;
        bool recieving_data;
        char data[cache_line_size];
    };

    deserializer_entry entries[deserializer_depth];
    deserializer(){
        for(deserializer_entry entry : entries){
            entry.valid = false;
        }
    }

    void recieve();
    deserializer_entry allocate();
};