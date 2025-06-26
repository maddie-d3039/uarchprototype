
#include "config.h"
class BP{
int branchTable[1 << history_length]; //each history is 
unsigned char BHR = 0;
public:
BP(){
    BHR = 0;
    for(int i = 0; i < (1 << history_length); i++){
        branchTable[i] = starting_val;
    }
}

void update(bool taken){
  if (taken) {
    if (branchTable[BHR] < 3) branchTable[BHR]++;
} else {
    if (branchTable[BHR] > 0) branchTable[BHR]--;
    }
    BHR = BHR << 1;
    if(taken){
        BHR |= 0x1;
    }
}

bool predict(){
    return branchTable[BHR] & 0x02;
}

};