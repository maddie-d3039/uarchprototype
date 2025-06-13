#include "config.h"
#include "ibuffer.h"
#include "icache.h"

class cpu
{
public:
    cpu() = default;
    void fetch_stage();

    struct PipeState_Entry
    {
        int predecode_valid;
        int predecode_ibuffer[ibuffer_size][cache_line_size];
        int predecode_EIP;
        int predecode_offset;
        int predecode_current_sector;
        int predecode_line_offset;

        int decode_valid;
        int decode_instruction_length;
        int decode_EIP;
        int decode_immSize;

        int agbr_valid;
        int agbr_cs[num_control_store_bits];
        int agbr_NEIP;
        int agbr_op1_base;
        int agbr_op1_index;
        int agbr_op1_scale;
        int agbr_op1_disp;
        int agbr_op2_base;
        int agbr_op2_index;
        int agbr_op2_scale;
        int agbr_op2_disp;
        int agbr_offset;

        int rr_valid;
        int rr_operation;
        int rr_updated_flags;
        int rr_op1_base;
        int rr_op1_index;
        int rr_op1_scale;
        int rr_op1_disp;
        int rr_op1_addr_mode;
        int rr_op2_base;
        int rr_op2_index;
        int rr_op2_scale;
        int rr_op2_disp;
        int rr_op2_addr_mode;

        char decode_instruction_register[15];
        char agbr_instruction_register[15];
    };

    int EIP;
    int length;
    int data_bus;
    int metadata_bus;
    
    PipeState_Entry pipeline;
    PipeState_Entry new_pipeline;
    ibuffer* the_ibuffer = new ibuffer();
    icache* the_icache = new icache();
};
