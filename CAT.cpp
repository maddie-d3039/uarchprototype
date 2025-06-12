#include "config.h"
class CAT{
enum conditionCodes{ //condition codes changed by the subset of instructions
    CF,
    PF,
    AF,
    ZF,
    SF,
    OF,
    DF
}; //each entry in the FAT corresponds to a specific flag 
struct ConditionAliasPool {
    int tags[flag_alias_pool_entries];
    bool valid[flag_alias_pool_entries];

    ConditionAliasPool() {
        for (int i = 0; i < flag_alias_pool_entries; ++i) {
            tags[i] = i;
            valid[i] = false;
        }
    }
};

ConditionAliasPool* aliases = new ConditionAliasPool();
int valid[FAT_size] = {}; //all of these are initialized to zero
int tag[FAT_size] = {};
int val[FAT_size] = {};

int getAlias() {
     for (int i = 0; i < flag_alias_pool_entries; ++i) {
        if (!valid[i]) {
            valid[i] = true;
             return aliases->tags[i]; //new tag 
           }
       }
     return -1; //all aliases in use 
 }
void deallocateAlias(int alias) {
     if (alias >= 0 && alias < flag_alias_pool_entries) {
         aliases->valid[alias] = false;
     }
}

bool makeAliases(int* cs_line){ //make a class for the control store 
    int alias = getAlias();
    if(alias == -1){
        return false;
    }
    if(sets cf){ //placeholder
        tag[CF] = alias;
        valid[CF] = 0;
    }
    if(sets pf){ //placeholder
        tag[PF] = alias;
        valid[PF] = 0;
    }   
    if(sets af){ //placeholder
        tag[AF] = alias;
        valid[AF] = 0;
    }
    if(sets zf){ //placeholder
        tag[ZF] = alias;
        valid[ZF] = 0;
    }
    if(sets sf){ //placeholder
        tag[SF] = alias;
        valid[SF] = 0;
    }
    if(sets of){ //placeholder
        tag[OF] = alias;
        valid[OF] = 0;
    }
    if(sets df){ //placeholder
        tag[DF] = alias;
        valid[DF] = 0;
    }
    return true;
}

void writeback(int executeVal, int alias){ //add more params later, not sure what signals needed to set different flags
    if(executeVal < 0){
        if(tag[SF] == alias){
         val[SF] = 1;
        }
    }
    if(executeVal == 0){
        if(tag[ZF] == alias){
         val[ZF] = 1;
        }
    }
    //assign more flags
    for(int i = 0; i < FAT_size; i++){
        if(tag[i] == alias){
            valid[i] = 1; //mark all entries with the tag valid (completed)
        }
    }
    deallocateAlias(alias);

}
};