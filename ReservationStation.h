#include "config.h"

class ReservationStation
{
    typedef struct ReservationStation_Entry_Struct
    {
        int store_tag, entry_valid, updated_flags;
        // operand 1
        int op1_mem_alias, op1_addr_mode, op1_base_valid, op1_base_tag, op1_base_val;
        int op1_index_valid, op1_index_tag, op1_index_val, op1_scale, op1_imm;
        int op1_ready, op1_combined_val, op1_load_alias;
        int op1_valid, op1_data;
        // operand 2
        int op2_mem_alias, op2_addr_mode, op2_base_valid, op2_base_tag, op2_base_val;
        int op2_index_valid, op2_index_tag, op2_index_val, op2_scale, op2_imm;
        int op2_ready, op2_combined_val, op2_load_alias;
        int op2_valid, op2_data;
        // I don't think you need the result in the entry? Correct me if I'm wrong
    } ReservationStation_Entry;

    ReservationStation_Entry entries[num_entries_per_RS];
};
// test