#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include "BP.cpp"
#include "RAT.cpp"
#include "CAT.cpp"
#include "config.h"
void fetch_stage();
void predecode_stage();
void decode_stage();
void addgen_branch_stage();
void register_rename_stage();
void execute_stage();
void memory_stage();
void writeback_stage();
void memory_controller();
void bus_arbiter();
void mshr_preinserter(int, int, int);
void mshr_inserter();
void serializer_inserter(int, int data[cache_line_size]);
void deserializer_handler();
void deserializer_inserter();
void rescue_stager();
void rescue_stager();
void icache_write_from_databus();
void dcache_write_from_databus();
void tlb_write();
void mshr_printer();
void bus_printer();
void rat_printer();
void station_printer();
void rob_printer();
void lsqueue_printer();
void cache_printer();

#define control_store_rows 20 // arbitrary
#define TRUE 1
#define FALSE 0

enum CS_BITS
{
    operation,
    updated_flags,
    is_control_instruction,
    control_instruction_type,
    op1_addr_mode,
    op2_addr_mode,
    is_op1_needed,
    is_op2_needed,
    num_control_store_bits
} CS_BITS;

int CONTROL_STORE[control_store_rows][num_control_store_bits];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/

#define WORDS_IN_MEM 0x01000
// simple memory array but its obsolete, use the dram struct
int MEMORY[WORDS_IN_MEM][4];

// address mapping
//  RRR RRRR CCBkBk CCBoBBoB
// actual structures
// a column has 4 bytes
typedef struct Column_Struct
{
    int bytes[bytes_per_column];
} Column;
typedef struct Row_Struct
{
    Column columns[columns_per_row];
} Row;
typedef struct Bank_Struct
{
    Row rows[rows_per_bank];
} Bank;
typedef struct DRAM_Struct
{
    Bank banks[banks_in_DRAM];
} DRAM;
DRAM dram;

// mask helper functions

int get_row_bits(int addr)
{
    return (addr >> 8) & 0x7F;
}

int get_bank_bits(int addr)
{
    return (addr >> 4) & 0x3;
}

int get_column_bits(int addr)
{
    return ((addr >> 6) & (0x3) << 2) + (addr >> 2) & (0x3);
}

// BUS

// make data bus of 32 bits
// make metadata bus capable of holding handshake and activation signals, address, etc. Figure out all that needs to be in it

#define bytes_on_data_bus 4
typedef struct Data_Bus_Struct
{
    int byte_wires[bytes_on_data_bus];
} Data_Bus;

Data_Bus data_bus;

typedef struct Metadata_Bus_Struct
{
    int mshr_address;
    int serializer_address;
    int store_address;
    int is_mshr_sending_addr;
    int to_mem_request_ID;
    int to_cpu_request_ID;
    int from_mem_request_ID;
    int from_cpu_request_ID;
    int is_serializer_sending_data;
    int burst_counter;
    int next_burst_counter;

    int receive_enable;
    int revival_in_progress;
    int current_revival_entry;

    int origin;
    int destination;
    int deserializer_full;
    int deserializer_can_store = 1;
    int deserializer_bank_targets[banks_in_DRAM];
    int deserializer_target; // oh this is for the deserializer that will be stored into next
    int deserializer_next_entry;
    int serializer_available = 1;   // equivalent to allow evictions
    int bank_status[banks_in_DRAM]; // 0 available, 1 performing load, 2 performing store, 3 attempting revival
    int bank_destinations[banks_in_DRAM];
} Metadata_Bus;

Metadata_Bus metadata_bus;

#define bits_in_word 32

// architectural registers

// rat and reg file
enum Registers
{
    EAX_idx,
    EBX_idx,
    ECX_idx,
    EDX_idx,
    ESI_idx,
    EDI_idx,
    EBP_idx,
    ESP_idx,
    GPR_Count
} Registers;

enum conditionCodes
{ // condition codes changed by the subset of instructions
    CF,
    PF,
    AF,
    ZF,
    SF,
    OF,
    DF,
    Flag_Count
};
typedef struct Register_File_Struct
{
    int EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP;
} Register_File;

// typedef struct RAT_MetadataEntry_Struct
// {
//     int valid, alias;
// } RAT_MetadataEntry;
// typedef struct RAT_Struct
// {
//     Register_File regfile;
//     RAT_MetadataEntry metadata[GPR_Count];
// } RAT;

RAT rat;

/* fat and flag file
enum Flags
{
    Carry_idx,
    Parity_idx,
    Auxiliary_Carry_idx,
    Zero_idx,
    Sign_idx,
    Trap_idx,
    Interrupt_enable_idx,
    Direction_idx,
    Overflow_idx,
    IO_Privilege_level_idx,
    Nester_task_flag_idx,
    Mode_flag_idx,
    Resume_idx,
    Alignnment_check_idx,
    Flag_Count
} Flags; // I didn't include some virtual process flags because I don't think we need them? Correct me if I'm wrong
typedef struct Flag_File_Struct
{
    int Carry, Parity, Auxiliary_Carry, Zero, Sign, Trap, Interrupt_enable, Direction,
        Overflow, IO_Privilege_level, Nester_task_flag, Mode_flag, Resume, Alignnment_check, Flag_Count;
} Flag_File;
typedef struct FAT_MetadataEntry_Struct
{
    int valid, alias;
} FAT_MetadataEntry;
typedef struct FAT_Struct
{
    Flag_File flagfile;
    FAT_MetadataEntry metadata[Flag_Count];
} FAT;

*/

CAT cat;

int EIP;
int oldEIP;
int tempEIP; // used for branch resteering
int tempOffset;
// used for bus arbiter
int serializer_entry_to_send;
int end_of_file = 0;
// --------------------------------------------------------------
// Struct for Register Alias Pool (global IDs 0..63)
// --------------------------------------------------------------
struct RegisterAliasPool
{
    int aliases[REGISTER_ALIAS_POOL_ENTRIES];
    bool valid[REGISTER_ALIAS_POOL_ENTRIES];

    RegisterAliasPool()
    {
        // Initialize each slot so aliases[i] = (base + i), and mark all as unused
        for (int i = 0; i < REGISTER_ALIAS_POOL_ENTRIES; ++i)
        {
            aliases[i] = REGISTER_ALIAS_BASE + i;
            valid[i] = false;
        }
    }

    // Returns one free alias in [0..63], or -1 if none left
    int get()
    {
        for (int i = 0; i < REGISTER_ALIAS_POOL_ENTRIES; ++i)
        {
            if (!valid[i])
            {
                valid[i] = true;
                return aliases[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [0..63] or wasn't actually allocated, an assert fires.
    void free(int alias)
    {
        // Compute local index
        int index = alias - REGISTER_ALIAS_BASE;

        // 1) Assert that alias is within this pool's global range
        assert(index >= 0 && index < REGISTER_ALIAS_POOL_ENTRIES && "RegisterAliasPool::free(): alias out of range!");

        // 2) Assert that this slot was previously allocated (valid == true)
        assert(valid[index] && "RegisterAliasPool::free(): alias was not allocated!");

        // Finally, mark it free again
        valid[index] = false;
    }
};

// --------------------------------------------------------------
// Struct for Flag Alias Pool (global IDs 64..127)
// --------------------------------------------------------------
struct FlagAliasPool
{
    int aliases[FLAG_ALIAS_POOL_ENTRIES];
    bool valid[FLAG_ALIAS_POOL_ENTRIES];

    FlagAliasPool()
    {
        for (int i = 0; i < FLAG_ALIAS_POOL_ENTRIES; ++i)
        {
            aliases[i] = FLAG_ALIAS_BASE + i; // 64 + i
            valid[i] = false;
        }
    }

    // Returns one free alias in [64..127], or -1 if none left
    int get()
    {
        for (int i = 0; i < FLAG_ALIAS_POOL_ENTRIES; ++i)
        {
            if (!valid[i])
            {
                valid[i] = true;
                return aliases[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [64..127] or wasn't allocated, an assert fires.
    void free(int alias)
    {
        int index = alias - FLAG_ALIAS_BASE;

        assert(index >= 0 && index < FLAG_ALIAS_POOL_ENTRIES && "FlagAliasPool::free(): alias out of range!");
        assert(valid[index] && "FlagAliasPool::free(): alias was not allocated!");

        valid[index] = false;
    }
};

// --------------------------------------------------------------
// Struct for Memory Alias Pool (global IDs 128..191)
// --------------------------------------------------------------
struct MemoryAliasPool
{
    int aliases[MEMORY_ALIAS_POOL_ENTRIES];
    bool valid[MEMORY_ALIAS_POOL_ENTRIES];

    MemoryAliasPool()
    {
        for (int i = 0; i < MEMORY_ALIAS_POOL_ENTRIES; ++i)
        {
            aliases[i] = MEMORY_ALIAS_BASE + i; // 128 + i
            valid[i] = false;
        }
    }

    // Returns one free alias in [128..191], or -1 if none left
    int get()
    {
        for (int i = 0; i < MEMORY_ALIAS_POOL_ENTRIES; ++i)
        {
            if (!valid[i])
            {
                valid[i] = true;
                return aliases[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [128..191] or wasn't allocated, an assert fires.
    void free(int alias)
    {
        int index = alias - MEMORY_ALIAS_BASE;

        assert(index >= 0 && index < MEMORY_ALIAS_POOL_ENTRIES && "MemoryAliasPool::free(): alias out of range!");
        assert(valid[index] && "MemoryAliasPool::free(): alias was not allocated!");

        valid[index] = false;
    }
};

RegisterAliasPool regpool;
FlagAliasPool flagpool;
MemoryAliasPool mempool;

#define ibuffer_size 4
#define dispimm_size 14
int ibuffer[ibuffer_size][cache_line_size];
int ibuffer_valid[ibuffer_size];

typedef struct PipeState_Entry_Struct
{
    int predecode_valid, predecode_potential[15], predecode_EIP,
        predecode_offset, predecode_current_sector, predecode_line_offset,
        decode_valid, decode_instruction_length, decode_EIP, decode_immSize,
        decode_offset, decode_is_prefix, decode_prefix, decode_opcode,
        decode_is_modrm, decode_modrm, decode_is_sib, decode_sib, decode_dispimm[dispimm_size],
        decode_1bdisp, decode_4bdisp, decode_1bimm, decode_2bimm, decode_4bimm,
        agbr_valid, agbr_cs[num_control_store_bits], agbr_NEIP, agbr_halt, agbr_opcode,
        agbr_op1_base, agbr_op1_index, agbr_op1_scale, agbr_op1_disp,
        agbr_op2_base, agbr_op2_index, agbr_op2_scale, agbr_op2_disp, agbr_offset, agbr32_16, //0 is 32 bits, 1 is 16 bits
        rr_valid, rr_operation, rr_updated_flags, rr32_16, rr_halt, rr_cat_alias,
        rr_op1_base, rr_op1_index, rr_op1_scale, rr_op1_disp, rr_op1_addr_mode,
        rr_op2_base, rr_op2_index, rr_op2_scale, rr_op2_disp, rr_op2_addr_mode;

    char decode_instruction_register[15];
    char agbr_instruction_register[15];
} PipeState_Entry;

PipeState_Entry pipeline, new_pipeline;
BP *branch_predictor = new BP();
int cycle_count;

// tlb
typedef struct TLBEntry_Struct
{
    int valid, present, permissions, vpn, pfn;
} TLBEntry;
typedef struct TLB_Struct
{
    TLBEntry entries[tlb_entries];
} TLB;

TLB tlb;

int RUN_BIT;

void cycle()
{
    new_pipeline = pipeline;
    mshr_inserter();
    // stages
    writeback_stage();
    memory_stage();
    execute_stage();
    register_rename_stage();
    addgen_branch_stage();
    decode_stage();
    predecode_stage();
    fetch_stage();
    // memory structures
    memory_controller();
    bus_arbiter();
    deserializer_handler();
    if (metadata_bus.receive_enable && metadata_bus.destination == 0)
    {
        icache_write_from_databus();
    }
    else if (metadata_bus.receive_enable && metadata_bus.destination == 1)
    {
        dcache_write_from_databus();
    }
    else if (metadata_bus.receive_enable && metadata_bus.destination == 2)
    {
        tlb_write();
    }
    pipeline = new_pipeline;
    // cache_printer();
    cycle_count++;
}

int halt=0;
void run(int num_cycles)
{
    int i;

    if (RUN_BIT == FALSE)
    {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++)
    {
        if (halt==1)
        {
            cycle();
            RUN_BIT = FALSE;
            printf("Simulator halted\n\n");
            break;
        }
        cycle();
    }
}

void go()
{
    if ((RUN_BIT == FALSE) || (EIP == 0x0000))
    {
        printf("Can't simulate, Simulator is halted\n\n");
        return;
    }
    printf("Simulating...\n\n");
    /* initialization */
    while (EIP != 0x0000)
        cycle();
    cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

void mdump(FILE *dumpsim_file, int start, int stop)
{
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start); address <= (stop); address += 2)
    {
        int i = (address & 0x30) >> 4;
        int j = (address & 0x7F00) >> 8;
        int k = (((address & 0xC) >> 2) + ((address & 0xC0) >> 4));
        int l = (address & 0x3);
        printf("  0x%04x (%d) : 0x%02x%02x\n", address, address, dram.banks[i].rows[j].columns[k].bytes[l + 1], dram.banks[i].rows[j].columns[k].bytes[l]);
    }
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
        fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

void rdump(FILE *dumpsim_file)
{
    int k;

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count : %d\n", cycle_count);
    printf("PC          : 0x%04x\n", EIP);
    // printf("Flags: %d\n", EFLAGS);// Need to fix this, currently dont have flags printing
    printf("Registers:\n");
    // printf("EAX: %d EBX: %d ECX: %d EDX: %d ESI: %d EDI: %d EBP: %d ESP: %d",EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP);//Need to fix this, currently dont have registers printing
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent architectural state :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count : %d\n", cycle_count);
    fprintf(dumpsim_file, "PC          : 0x%04x\n", EIP);
    // fprintf(dumpsim_file, "Flags: %d\n", EFLAGS);// Need to fix this, currently dont have flags printing
    fprintf(dumpsim_file, "Registers:\n");
    // fprintf(dumpsim_file, "EAX: %d EBX: %d ECX: %d EDX: %d ESI: %d EDI: %d EBP: %d ESP: %d",EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP);//Need to fix this, currently dont have registers printing
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

void idump(FILE *dumpsim_file)
{
    int k;

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count : %d\n", cycle_count);
    printf("PC          : 0x%04x\n", EIP);
    // printf("Flags: %d\n", EFLAGS);// Need to fix this, currently dont have flags printing
    printf("Registers:\n");
    // printf("EAX: %d EBX: %d ECX: %d EDX: %d ESI: %d EDI: %d EBP: %d ESP: %d",EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP); //Need to fix this, currently dont have registers printing
    printf("\n");
    printf("------------- FETCH Latches --------------\n");
    printf("Fetching?   :  %d\n", !pipeline.predecode_valid);
    printf("\n");
    printf("-----------IBUFFER------------\n");
    for (int i = 0; i < ibuffer_size; i++)
    {
        for (int j = 0; j < cache_line_size; j++)
        {
            printf("0x%x ", ibuffer[i][j]);
        }
        printf("\n");
    }
    printf("VALID:");
    for (int i = 0; i < ibuffer_size; i++)
    {
        printf(" %d ", ibuffer_valid[i]);
    }
    printf("\n");
    printf("------------- PREDECODE   Latches --------------\n");
    printf("PD V        :  0x%04x\n", pipeline.predecode_valid);
    printf("\n");
    printf("PD EIP      :  %d\n", EIP);
    printf("\n");

    printf("------------- DECODE Latches --------------\n");
    printf("DC V        :  0x%04x\n", pipeline.decode_valid);
    printf("\n");
    // printf("DC IR       :  0x%04x\n", pipeline.decode_instruction_register );
    printf("DC IL       :  0x%04x\n", pipeline.decode_instruction_length);
    printf("DC EIP      :  %d\n", pipeline.decode_EIP);
    printf("DC IMMSZ    :  %d\n", pipeline.decode_immSize);
    printf("DC OFF      :  %d\n", pipeline.decode_offset);
    printf("DC PF?      :  %d\n", pipeline.decode_is_prefix);
    printf("DC PF       :  0x%x\n", pipeline.decode_prefix);
    printf("DC OP       :  0x%x\n", pipeline.decode_opcode);
    printf("DC RM?      :  %d\n", pipeline.decode_is_modrm);
    printf("DC RM       :  0x%x\n", pipeline.decode_modrm);
    printf("DC SIB?     :  %d\n", pipeline.decode_is_sib);
    printf("DC SIB      :  0x%x\n", pipeline.decode_sib);
    printf("DC 1BD      :  %d\n", pipeline.decode_1bdisp);
    printf("DC 4BD      :  %d\n", pipeline.decode_4bdisp);
    printf("DC 1BI      :  %d\n", pipeline.decode_1bimm);
    printf("DC 2BI      :  %d\n", pipeline.decode_2bimm);
    printf("DC 4BI      :  %d\n", pipeline.decode_4bimm);
    printf("\n");

    printf("------------- AGEN_BR  Latches --------------\n");
    printf("AGBR V         :  0x%04x\n", pipeline.agbr_valid);
    printf("\n");
    printf("AGBR NEIP   :  0x%04x\n", pipeline.agbr_NEIP);
    printf("AGBR OP1 B  :  0x%04x\n", pipeline.agbr_op1_base);
    printf("AGBR OP1 I  :  %d\n", pipeline.agbr_op1_index);
    printf("AGBR OP1 S  :  0x%04x\n", pipeline.agbr_op1_scale);
    printf("AGBR OP1 D  :  %d\n", pipeline.agbr_op1_disp);
    printf("AGBR OP2 B  :  0x%04x\n", pipeline.agbr_op2_base);
    printf("AGBR OP2 I  :  %d\n", pipeline.agbr_op2_index);
    printf("AGBR OP2 S  :  0x%04x\n", pipeline.agbr_op2_scale);
    printf("AGBR OP2 D  :  %d\n", pipeline.agbr_op2_disp);
    printf("AGBR_CS     :  ");
    for (k = 0; k < num_control_store_bits; k++)
    {
        printf("%d  ", pipeline.agbr_cs[k]);
    }

    printf("\n\n------------- REG RENAME   Latches --------------\n");
    printf("RR V        :  0x%04x\n", pipeline.rr_valid);
    printf("\n");
    printf("RR OP       :  0x%04x\n", pipeline.rr_operation);
    printf("RR UF       :  0x%04x\n", pipeline.rr_updated_flags);
    printf("RR OP1 B    :  0x%04x\n", pipeline.rr_op1_base);
    printf("RR OP1 I    :  0x%04x\n", pipeline.rr_op1_index);
    printf("RR OP1 S    :  0x%04x\n", pipeline.rr_op1_scale);
    printf("RR OP1 D    :  0x%04x\n", pipeline.rr_op1_disp);
    printf("RR OP1 MODE    :  0x%04x\n", pipeline.rr_op1_addr_mode);
    printf("RR OP2 B    :  0x%04x\n", pipeline.rr_op2_base);
    printf("RR OP2 I    :  0x%04x\n", pipeline.rr_op2_index);
    printf("RR OP2 S    :  0x%04x\n", pipeline.rr_op2_scale);
    printf("RR OP2 D    :  0x%04x\n", pipeline.rr_op2_disp);
    printf("RR OP2 MODE    :  0x%04x\n", pipeline.rr_op2_addr_mode);

    printf("\n");
}

void init_memory()
{
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        for (int j = 0; j < rows_per_bank; j++)
        {
            for (int k = 0; k < columns_per_row; k++)
            {
                for (int l = 0; l < bytes_per_column; l++)
                {
                    dram.banks[i].rows[j].columns[k].bytes[l] = 0;
                }
            }
        }
    }
}

void init_state()
{
    memset(&pipeline, 0, sizeof(PipeState_Entry));
    memset(&new_pipeline, 0, sizeof(PipeState_Entry));
}

void get_command(FILE *dumpsim_file)
{
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch (buffer[0])
    {
    case 'G':
    case 'g':
        go();
        break;

    case 'M':
    case 'm':
        scanf("%i %i", &start, &stop);
        mdump(dumpsim_file, start, stop);
        break;

    case '?':
        // help();
        break;
    case 'Q':
    case 'q':
        printf("Bye.\n");
        exit(0);

    case 'x':
    case 'X':
        mshr_printer();
        break;

    case 'p':
    case 'P':
        rat_printer();
        break;

    case 'l':
    case 'L':
        lsqueue_printer();
        break;

    case 'c':
    case 'C':
        cache_printer();
        break;

    case 'b':
    case 'B':
        bus_printer();
        break;

    case 's':
    case 'S':
        station_printer();
        break;

    case 'v':
    case 'V':
        rob_printer();
        break;

    case 'R':
    case 'r':
        if (buffer[1] == 'd' || buffer[1] == 'D')
            rdump(dumpsim_file);
        else
        {
            scanf("%d", &cycles);
            run(cycles);
        }
        break;

    case 'I':
    case 'i':
        idump(dumpsim_file);
        break;

    default:
        printf("Invalid Command\n");
        break;
    }
}

void load_program(char *program_filename)
{
    FILE *prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL)
    {
        printf("Error: Can't open program file %s\n", program_filename);
        exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
        program_base = word >> 1;
    else
    {
        printf("Error: Program file is empty\n");
        exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF)
    {
        /* Make sure it fits. */
        if (program_base + ii >= WORDS_IN_MEM)
        {
            printf("Error: Program file %s is too long to fit in memory. %x\n",
                   program_filename, ii);
            exit(-1);
        }
        /* Write the word to memory array. */
        int address = program_base + ii;
        // printf("address %d\n",address);
        int i = (address & 0x30) >> 4;
        int j = (address & 0x7F00) >> 8;
        int k = (((address & 0xC) >> 2) + ((address & 0xC0) >> 4));
        int l = (address & 0x3);
        dram.banks[i].rows[j].columns[k].bytes[l] = word & 0x00FF;
        // printf("%x\n",word & 0x00FF);
        dram.banks[i].rows[j].columns[k].bytes[l + 1] = (word >> 8) & 0x00FF;
        // printf("%x\n",(word >> 8) & 0x00FF);
        ii += 2;
    }

    int a = 0xFFFF;
    int address = program_base + ii;

    int i = (address & 0x30) >> 4;
    int j = (address & 0x7F00) >> 8;
    int k = (((address & 0xC) >> 2) + ((address & 0xC0) >> 4));
    int l = (address & 0x3);

    dram.banks[i].rows[j].columns[k].bytes[l] = a & 0x00FF;
    dram.banks[i].rows[j].columns[k].bytes[l + 1] = (a >> 8) & 0x00FF;

    if (EIP == 0)
        EIP = program_base << 1;
    printf("Read %d words from program into memory.\n\n", ii);
}
int pause, globalID;
void initialize(char *program_filename, int num_prog_files)
{
    int i;

    init_memory();

    for (i = 0; i < num_prog_files; i++)
    {
        load_program(program_filename);
        while (*program_filename++ != '\0')
            ;
    }
    init_state();

    oldEIP = 0;
    pause = 0;
    globalID = 0;

    for (i = 0; i < GPR_Count; i++)
    {
        rat.valid[i] = 1;
    }
    for (i = 0; i < Flag_Count; i++)
    {
        cat.valid[i] = 1;
    }

    tlb.entries[0].valid = 1;
    tlb.entries[0].pfn = 0;
    tlb.entries[0].vpn = 0;
    tlb.entries[0].permissions = 0;
    tlb.entries[0].present = 1;

    RUN_BIT = TRUE;
}

int main(int argc, char *argv[])
{
    FILE *dumpsim_file;

    /* Error Checking */
    if (argc < 2)
    {
        printf("Error: usage: %s <program_file_1> <program_file_2> ...\n",
               argv[0]);
        exit(1);
    }

    printf("x86 Simulator\n\n");

    initialize(argv[1], argc - 1);

    if ((dumpsim_file = fopen("dumpsim", "w")) == NULL)
    {
        printf("Error: Can't open dumpsim file\n");
        exit(-1);
    }

    while (1)
        get_command(dumpsim_file);
}

// other structures

// DCACHE
// address mapping: TTT TTT TTI IBO OOO
typedef struct D$_TagStoreEntry_Struct
{
    int valid[dcache_ways], tag[dcache_ways], dirty[dcache_ways], lru;
} D$_TagStoreEntry;
typedef struct D$_TagStore_Struct
{
    D$_TagStoreEntry dcache_tagstore[dcache_banks][dcache_sets];
} D$_TagStore;
typedef struct D$_DataStore_Struct
{
    int dcache_datastore[dcache_banks][dcache_sets][dcache_ways][cache_line_size];
} D$_DataStore;
typedef struct D$_Struct
{
    D$_DataStore data;
    D$_TagStore tag;
    int bank_status[icache_banks]; // 0 means that its available, 1 means its being written tos
} D$;

int get_dcache_bank_bits(int address)
{
    return (address >> 3) & 0x1;
}
int get_dcache_idx_bits(int address)
{
    return (address >> 4) & 0x3;
}

D$ dcache;

// ICACHE
// address mapping: TTT TTT TII IBO OOO

typedef struct I$_TagStoreEntry_Struct
{
    int valid[icache_ways], tag[icache_ways], dirty[icache_ways];
    int lru;
} I$_TagStoreEntry;
typedef struct I$_TagStore_Struct
{
    I$_TagStoreEntry icache_tagstore[icache_banks][icache_sets];
} I$_TagStore;
typedef struct I$_DataStore_Struct
{
    int icache_datastore[icache_banks][icache_sets][icache_ways][cache_line_size];
} I$_DataStore;
typedef struct I$_Struct
{
    I$_DataStore data;
    I$_TagStore tag;
    int bank_status[icache_banks]; // 0 means that its available, 1 means its being written to
} I$;

I$ icache;

int get_icache_bank_bits(int address)
{
    return (address >> 4) & 0x1;
}
int get_icache_idx_bits(int address)
{
    return (address >> 5) & 0x7;
}

// load queue
#define max_lq_size 16
typedef struct LoadQueue_Entry_Struct
{
    int entry_valid;
    int address_valid, address_tag, address;
    int old_bits;
    int valid, tag, data[bytes_on_data_bus];
} LoadQueue_Entry;
typedef struct LoadQueue_Struct
{
    LoadQueue_Entry entries[max_lq_size];
    int occupancy;
} LoadQueue;
LoadQueue lq;
// store queue
#define max_sq_size 16
typedef struct StoreQueue_Entry_Struct
{
    int entry_valid;
    int address_valid, address_tag, address;
    int matchAddress;
    int old_bits, written;
    int valid, tag, data[bytes_on_data_bus];
} StoreQueue_Entry;
typedef struct StoreQueue_Struct
{
    StoreQueue_Entry entries[max_sq_size];
    int occupancy;
} StoreQueue;
StoreQueue sq;

// reservation_stations
// note that the RS will need the internal adders/shifters in whatever reservation station handler function we make

typedef struct ReservationStation_Entry_Struct
{
    int store_tag, store_tag_valid, entry_valid, updated_flags, old_bits, instruction_ID, blocked, mode32_16;
    // operand 1
    int op1_mem_alias, op1_mem_alias_valid;
    int op1_addr_mode, op1_base_valid, op1_base_tag, op1_base_val;
    int op1_index_valid, op1_index_tag, op1_index_val, op1_scale, op1_imm;
    int op1_ready, op1_combined_val, op1_load_alias, op1_load_alias_valid;

    int op1_valid, op1_data;
    // operand 2
    int op2_mem_alias, op2_mem_alias_valid;
    int op2_addr_mode, op2_base_valid, op2_base_tag, op2_base_val;
    int op2_index_valid, op2_index_tag, op2_index_val, op2_scale, op2_imm;
    int op2_ready, op2_combined_val, op2_load_alias, op2_load_alias_valid;

    int op2_valid, op2_data;
    // I don't think you need the result in the entry? Correct me if I'm wrong
} ReservationStation_Entry;

typedef struct ReservationStation_Struct
{
    ReservationStation_Entry entries[num_entries_per_RS];
    int occupancy;

} ReservationStation;

#define num_stations 2
ReservationStation stations[num_stations];
// mshr
// entries are the actual entries waiting for the data to come back or waiting to send their address to the memory controller
// pre-entries are what just got inserted this cycle and need to be sorted

typedef struct MSHR_Entry_Struct
{
    int valid, old_bits, origin, address, request_ID;
} MSHR_Entry;
typedef struct MSHR
{
    MSHR_Entry entries[mshr_size];
    MSHR_Entry pre_entries[pre_mshr_size];
    int occupancy;
    int pre_occupancy;
} MSHR;
MSHR mshr;
// btb

// Speculative Execution Tracker

typedef struct SpecExe_Entry_Struct
{
    int valid, not_taken_target, taken_target, prediction, 
    flagTags[Flag_Count], flagValues[Flag_Count], flagValid[Flag_Count];
} SpecExe_Entry;
typedef struct SpecExeTracker_Struct
{
    SpecExe_Entry entries[spec_exe_tracker_size];
} SpecExeTracker;

SpecExeTracker br_tracker;

// Seralizers
#define max_serializer_occupancy 8
typedef struct Serializer_Entry_Struct
{
    int valid, old_bits, sending_data;
    int data[cache_line_size], address;
    int rescue_lock, rescue_destination;

} Serializer_Entry;

typedef struct Serializer_Struct
{
    Serializer_Entry entries[num_serializer_entries];
    int occupancy;
} Serializer;

Serializer serializer;

// Deserializers
#define max_deserializer_occupancy 8
typedef struct Deserializer_Entry_Struct
{
    int valid, old_bits, writing_data_to_DRAM, receiving_data_from_data_bus;
    int data[cache_line_size], address;
    int revival_lock, revival_destination, revival_reqID;

} Deserializer_Entry;

typedef struct Deserializer_Struct
{
    Deserializer_Entry entries[num_deserializer_entries];
    int occupancy;
} Deserializer;

Deserializer deserializer;

// ROB

typedef struct ROB_Entry_Struct
{
    int valid, old_bits, retired, executed, value, store_tag, speculative, speculation_tag;
    int instruction_ID, flagResults, halt, cat_alias;
} ROB_Entry;
typedef struct ROB_Struct
{
    ROB_Entry entries[rob_size];
    int occupancy;
} ROB;
ROB rob;

// functionality

void mshr_preinserter(int address, int origin, int request_ID)
{
    // origin 0 is icache
    // origin 1 is dcache
    // origin 2 is tlb
    // search for duplicates and merge
    for (int i = 0; i < pre_mshr_size; i++)
    {
        if ((mshr.pre_entries[i].valid == TRUE) && (mshr.pre_entries[i].address == address))
        {
            return;
        }
    }
    // insert
    for (int i = 0; i < pre_mshr_size; i++)
    {
        if (mshr.pre_entries[i].valid == FALSE)
        {
            mshr.pre_entries[i].valid = TRUE;
            mshr.pre_entries[i].old_bits = mshr.pre_occupancy;
            mshr.pre_occupancy++;
            mshr.pre_entries[i].origin = origin;
            mshr.pre_entries[i].address = address;
            mshr.pre_entries[i].request_ID = request_ID;
            return;
        }
    }
}

void translate_miss(int vpn)
{
    int phys_addr = SBR + vpn * pte_size;
    mshr_preinserter(phys_addr, 2, 2);
}

int data_way0[cache_line_size], data_way1[cache_line_size];
void icache_access(int addr, I$_TagStoreEntry **tag_metadata)
{
    int bank = (addr >> 3) & 0x1;
    int set = (addr >> 4) & 0x7;
    for (int i = 0; i < cache_line_size; i++)
    {
        data_way0[i] = icache.data.icache_datastore[bank][set][0][i];
    }
    for (int i = 0; i < cache_line_size; i++)
    {
        data_way1[i] = icache.data.icache_datastore[bank][set][1][i];
    }
    *tag_metadata = &icache.tag.icache_tagstore[bank][set];
    return;
}

void dcache_access(int addr, int *data_way0[cache_line_size], int *data_way1[cache_line_size], int *data_way2[cache_line_size], int *data_way3[cache_line_size], D$_TagStoreEntry *tag_metadata)
{
    int bank = (addr >> 3) & 0x1;
    int set = (addr >> 4) & 0x3;
    *data_way0 = dcache.data.dcache_datastore[bank][set][0];
    *data_way1 = dcache.data.dcache_datastore[bank][set][1];
    *data_way2 = dcache.data.dcache_datastore[bank][set][2];
    *data_way3 = dcache.data.dcache_datastore[bank][set][3];
    tag_metadata = &dcache.tag.dcache_tagstore[bank][set];
    return;
}

void tlb_access(int addr, int *physical_tag, int *tlb_hit)
{
    int incoming_vpn = (addr >> 12) & 0x7;
    for (int i = 0; i < tlb_entries; i++)
    {
        if (tlb.entries[i].valid == TRUE && tlb.entries[i].present == TRUE)
        {
            if (incoming_vpn == tlb.entries[i].vpn)
            {
                *tlb_hit = 1;
                *physical_tag = tlb.entries[i].pfn;
                return;
            }
        }
    }
    *tlb_hit = 0;
    translate_miss(incoming_vpn);
    return;
}

void tlb_write()
{
}

void icache_write_from_databus()
{
    int bkbits = get_icache_bank_bits(metadata_bus.store_address);
    int idxbits = get_icache_idx_bits(metadata_bus.store_address);
    // evict if needed, if not can do the write
    if (icache.tag.icache_tagstore[bkbits][idxbits].valid[icache.tag.icache_tagstore[bkbits][idxbits].lru] == TRUE)
    {
        // reconstruct address
        int reconstructed_address = (icache.tag.icache_tagstore[bkbits][idxbits].tag[icache.tag.icache_tagstore[bkbits][idxbits].lru] << 7) + (idxbits << 5) + (bkbits << 4);

        serializer_inserter(reconstructed_address, icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru]);
        icache.tag.icache_tagstore[bkbits][idxbits].valid[icache.tag.icache_tagstore[bkbits][idxbits].lru = FALSE];
    }
    else
    {
        icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 0] = data_bus.byte_wires[0];
        icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 1] = data_bus.byte_wires[1];
        icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 2] = data_bus.byte_wires[2];
        icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 3] = data_bus.byte_wires[3];

        // printf("%d\n",metadata_bus.burst_counter);
        // printf("%d\n",icache.tag.icache_tagstore[bkbits][idxbits].lru);
        // printf("valid addr: %x\ndirty addr: %x\ntag addr: %x\nlru addr: %x\n",
        //         &icache.tag.icache_tagstore[bkbits][idxbits].valid[0],
        //         &icache.tag.icache_tagstore[bkbits][idxbits].dirty[0],
        //         &icache.tag.icache_tagstore[bkbits][idxbits].tag[0],
        //         &icache.tag.icache_tagstore[bkbits][idxbits].lru);
        if (metadata_bus.burst_counter == 3)
        {
            // printf("check %d %d %d\n", bkbits, idxbits, icache.tag.icache_tagstore[bkbits][idxbits].lru);
            int way = icache.tag.icache_tagstore[bkbits][idxbits].lru;
            icache.tag.icache_tagstore[bkbits][idxbits].lru = !(icache.tag.icache_tagstore[bkbits][idxbits].lru);
            icache.tag.icache_tagstore[bkbits][idxbits].valid[way] = 1;
            // printf("%d\n",icache.tag.icache_tagstore[bkbits][idxbits].valid[way]);
            icache.tag.icache_tagstore[bkbits][idxbits].dirty[way] = 0;
            // printf("%d\n",icache.tag.icache_tagstore[bkbits][idxbits].valid[way]);
            icache.tag.icache_tagstore[bkbits][idxbits].tag[way] = metadata_bus.store_address >> 8;
            // printf("val %d\n",icache.tag.icache_tagstore[bkbits][idxbits].valid[way]);
            // printf("tag %x\n",icache.tag.icache_tagstore[bkbits][idxbits].tag[way]);
            // printf("dir %d\n",icache.tag.icache_tagstore[bkbits][idxbits].dirty[way]);
            // printf("lru %d\n",icache.tag.icache_tagstore[bkbits][idxbits].lru);
            // cache_printer();
        }

        metadata_bus.next_burst_counter++;
    }
}

// from rescue
bool icache_paste(int address, int data[cache_line_size])
{
    int bkbits = get_icache_bank_bits(address);
    int idxbits = get_icache_idx_bits(address);
    // evict if needed, if not can do the write
    if (icache.tag.icache_tagstore[bkbits][idxbits].valid[icache.tag.icache_tagstore[bkbits][idxbits].lru] == TRUE)
    {
        // reconstruct address
        int reconstructed_address = (icache.tag.icache_tagstore[bkbits][idxbits].tag[icache.tag.icache_tagstore[bkbits][idxbits].lru] << 7) + (idxbits << 5) + (bkbits << 4);

        serializer_inserter(reconstructed_address, icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru]);
        icache.tag.icache_tagstore[bkbits][idxbits].valid[icache.tag.icache_tagstore[bkbits][idxbits].lru = FALSE];
        return false;
    }
    else
    {
        for (int i = 0; i < cache_line_size; i++)
        {
            icache.data.icache_datastore[bkbits][idxbits][icache.tag.icache_tagstore[bkbits][idxbits].lru][0] = data[0];
        }
        return true;
    }
}

void dcache_write_from_databus()
{
    int bkbits = get_dcache_bank_bits(metadata_bus.store_address);
    int idxbits = get_dcache_idx_bits(metadata_bus.store_address);
    // evict if needed, if not can do the write
    if (dcache.tag.dcache_tagstore[bkbits][idxbits].valid[dcache.tag.dcache_tagstore[bkbits][idxbits].lru] == TRUE)
    {
        // reconstruct address
        int reconstructed_address = (dcache.tag.dcache_tagstore[bkbits][idxbits].tag[dcache.tag.dcache_tagstore[bkbits][idxbits].lru] << 7) + (idxbits << 5) + (bkbits << 4);

        serializer_inserter(reconstructed_address, dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru]);
        dcache.tag.dcache_tagstore[bkbits][idxbits].valid[dcache.tag.dcache_tagstore[bkbits][idxbits].lru] = FALSE;
    }
    else
    {
        dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 0] = data_bus.byte_wires[0];
        dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 1] = data_bus.byte_wires[1];
        dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 2] = data_bus.byte_wires[2];
        dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru][metadata_bus.burst_counter * 4 + 3] = data_bus.byte_wires[3];

        metadata_bus.next_burst_counter++;
    }
}

// from rescue
bool dcache_paste(int address, int data[cache_line_size])
{
    int bkbits = get_dcache_bank_bits(address);
    int idxbits = get_dcache_idx_bits(address);
    // evict if needed, if not can do the write
    if (dcache.tag.dcache_tagstore[bkbits][idxbits].valid[dcache.tag.dcache_tagstore[bkbits][idxbits].lru] == TRUE)
    {
        // reconstruct address
        int reconstructed_address = (dcache.tag.dcache_tagstore[bkbits][idxbits].tag[dcache.tag.dcache_tagstore[bkbits][idxbits].lru] << 7) + (idxbits << 5) + (bkbits << 4);

        serializer_inserter(reconstructed_address, dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru]);
        dcache.tag.dcache_tagstore[bkbits][idxbits].valid[dcache.tag.dcache_tagstore[bkbits][idxbits].lru = FALSE];
        return false;
    }
    else
    {
        for (int i = 0; i < cache_line_size; i++)
        {
            dcache.data.dcache_datastore[bkbits][idxbits][dcache.tag.dcache_tagstore[bkbits][idxbits].lru][0] = data[0];
        }
        return true;
    }
}

// stages

int rob_broadcast_value, rob_broadcast_tag, rob_broadcast_cat_alias;
void writeback_stage()
{
    // update to account for resteering/clearing on mispredicts
    for (int i = 0; i < rob_size; i++)
    {
        if ((rob.entries[i].valid == 1) && (rob.entries[i].old_bits == 0) && (rob.entries[i].retired != 1) && (rob.entries[i].executed == 1))
        {
            if (rob.entries[i].speculative == TRUE)
            {
                return;
            }
            rob_broadcast_value = rob.entries[i].value;
            rob_broadcast_tag = rob.entries[i].store_tag;
            rob_broadcast_cat_alias = rob.entries[i].cat_alias;
            // rat broadcast
            for (int j = 0; j < GPR_Count; j++)
            {
                if (rat.valid[j] == FALSE && rat.tag[j] == rob_broadcast_tag)
                {
                    rat.val[j] = rob_broadcast_value;
                    rat.valid[j] = TRUE;
                }
            }
            //
            for (int j = 0; j < Flag_Count; j++)
            {
                if (cat.valid[j] == FALSE && cat.tag[j] ==rob_broadcast_cat_alias)
                {
                    cat.val[j] = ((rob.entries[i].flagResults>>j)&0x1);
                    cat.valid[j] = TRUE;
                }
            }
            // lq sq broadcast do later
            rob.entries[i].valid = 0;
            rob.entries[i].retired = 1;
            rob.occupancy--;
            for (int j = 0; j < rob_size; j++)
            {
                if ((rob.entries[j].valid == TRUE) && (rob.entries[j].retired == FALSE))
                {
                    rob.entries[j].old_bits--;
                }
            }
            // need to to do this : if(rat alias)
            rat.deallocateAlias(rob.entries[i].store_tag);
            break;
        }else if (rob.entries[i].valid == FALSE && rob.entries[i].retired == TRUE){
            rob.entries[i].retired = FALSE;
        }else if ((rob.entries[i].valid == TRUE) && rob.entries[i].halt == TRUE){
            halt=1;
        }
    }
}

void memory_stage()
{
}

void execute_stage()
{
    for (int i = 0; i < num_stations; i++)
    {
        for (int j = 0; j < num_entries_per_RS; j++)
        {
            if (stations[i].entries[j].entry_valid && stations[i].entries[j].op1_ready && stations[i].entries[j].op2_ready)
            {
                int index_id;
                printf("hi\n");
                for (int k = 0; k < rob_size; k++)
                {
                    printf("hi1\n");
                    if (stations[i].entries[j].store_tag == rob.entries[k].store_tag)
                    {
                        printf("hi2\n");
                        index_id = k;
                        break;
                    }
                }
                printf("hi\n");
                long result;int overflow;
                if (i == 0)
                { // add RS
                    result = stations[i].entries[j].op1_combined_val + stations[i].entries[j].op2_combined_val;
                    overflow=0;
                    if(((stations[i].entries[j].op1_combined_val&0x80000000)==0)&&((stations[i].entries[j].op2_combined_val&0x80000000)==0)&&((result&0x80000000)==1)){
                        overflow=1;
                    }else if(((stations[i].entries[j].op1_combined_val&0x80000000)==1)&&((stations[i].entries[j].op2_combined_val&0x80000000)==1)&&((result&0x80000000)==0)){
                        overflow=1;
                    }
                    if(stations[i].entries[j].mode32_16==1){
                        rob.entries[index_id].value = result&0x0FFFF;
                    }else{
                        rob.entries[index_id].value = result&0x0FFFFFFFF;
                    }

                    if (stations[i].entries[j].store_tag_valid == TRUE)
                    {
                        for (int k = 0; k < num_stations; k++)
                        {
                            for (int l = 0; l < num_entries_per_RS; l++)
                            {
                                if (i == k && j == l)
                                {
                                    break;
                                }
                                if (stations[k].entries[l].entry_valid == TRUE)
                                {
                                    // operand 1
                                    if (stations[k].entries[l].op1_base_valid == FALSE &&
                                        stations[k].entries[l].op1_base_tag ==
                                            stations[i].entries[j].store_tag)
                                    {
                                        stations[k].entries[l].op1_base_val = result;
                                    }
                                    // operand 2
                                    if (stations[k].entries[l].op2_base_valid == FALSE &&
                                        stations[k].entries[l].op2_base_tag ==
                                            stations[i].entries[j].store_tag)
                                    {
                                        stations[k].entries[l].op2_base_val = result;
                                    }
                                }
                            }
                        }
                    }
                    //parity flag
                    int bitsSet;
                    int LSB = result&0x0FF;
                    int layer0_0 = !((LSB>>7) ^ ((LSB>>6)&0x1));
                    int layer0_1 = !(((LSB>>5)&0x1) ^ ((LSB>>4)&0x1));
                    int layer0_2 = !(((LSB>>3)&0x1) ^ ((LSB>>2)&0x1));
                    int layer0_3 = !(((LSB>>1)&0x1) ^ ((LSB)&0x1));
                    int layer1_0 = !(layer0_0 ^ layer0_1);
                    int layer1_1 = !(layer0_2 ^ layer0_3);
                    int parity = (!(layer1_0 ^ layer1_1))<<5;
                    rob.entries[index_id].flagResults+=parity;
                    //zero flag
                    int zero=0;
                    if(result==0){
                        zero=1;
                    }
                    rob.entries[index_id].flagResults+=(zero<<3);
                    //sign flag
                    int sign=0;
                    if(result<0){
                        sign=1;
                    }
                    rob.entries[index_id].flagResults+=(sign<<2);
                    //carry flag
                    int carry=0;
                    if(stations[i].entries[j].mode32_16==1){
                        if((result&0x10000)!=0){
                            carry = 1;
                        }
                    }else if(stations[i].entries[j].mode32_16==0){
                        if((result&0x100000000)!=0){
                            carry = 1;
                        }
                    }
                    rob.entries[index_id].flagResults+=(carry<<6);
                    //auxiliary flag
                    int auxiliary = (((stations[i].entries[j].op1_combined_val ^ stations[i].entries[j].op2_combined_val ^ result) & 0x10) != 0);
                    rob.entries[index_id].flagResults+=(auxiliary<<4);
                    //overflow flag
                    rob.entries[index_id].flagResults+=(overflow<<1);

                    rob.entries[index_id].executed = TRUE;
                }
                stations[i].entries[j].entry_valid = FALSE;
                stations[i].entries[j].store_tag_valid = FALSE;
                stations[i].entries[j].blocked = TRUE;
                stations[i].occupancy--;
                break;
            }
            else if (stations[i].entries[j].blocked == TRUE)
            {
                stations[i].entries[j].blocked = FALSE;
            }
        }
    }
}

enum addressing_modes
{
    immediate,
    reg,
    direct,
    indirect,
    based,
    indexed,
    based_indexed,
    based_indexed_disp,
    relative,
    num_addr_modes
} addressing_modes;

enum reservation_stations
{
    ADD,
    OR
} reservation_stations;

int regren_stall; // this should occur when ROB or relevant RS is full
void register_rename_stage()
{
    ReservationStation thisRS;
    thisRS = stations[pipeline.rr_operation];
    bool found_RS_entry = false;
    int entry_index;

    // precompute the RS entry that will
    // be written into and write directly to that

    for (int i = 0; i < num_entries_per_RS; i++)
    {
        if (thisRS.entries[i].entry_valid == FALSE && thisRS.entries[i].blocked == FALSE)
        {
            entry_index = i;
            found_RS_entry = true;
            break;
        }
    }
    if (found_RS_entry == false)
    {
        // stall if RS is full
        regren_stall = TRUE;
        return;
    }
    else
    {
        // also need to stall if addressing mode indicates that the instruction requires memory
        // and the load or store queue is full
        if (op1_addr_mode > reg)
        {
            if (sq.occupancy == max_sq_size)
            {
                regren_stall = TRUE;
                return;
            }
            else if (lq.occupancy == max_lq_size)
            {
                regren_stall = TRUE;
                return;
            }
        }
        else if (op2_addr_mode > reg)
        {
            if (lq.occupancy == max_lq_size)
            {
                regren_stall = TRUE;
                return;
            }
        }
    }
    regren_stall = FALSE;

    // handle in progress operand generation through internal arithmetic

    // iterate through the reservation stations and for each station,
    // do the internal arithmetic and load queue allocation for one entry
    for (int i = 0; i < num_stations; i++)
    {
        int search_age = 0;
        for (int j = 0; j < num_entries_per_RS; j++)
        {
            bool evaluated_op1 = false;
            bool evaluated_op2 = false;
            if (thisRS.entries[j].old_bits == search_age)
            {
                if (thisRS.entries[j].op1_ready == FALSE || thisRS.entries[j].op2_ready == FALSE)
                {
                    // compute operand depending on addressing mode
                    // note that we don't need to compute for indirect, but we do need it to be valid in order to put it in the lq

                    // operand 1
                    if (thisRS.entries[j].op1_addr_mode == reg || thisRS.entries[j].op1_addr_mode == indirect)
                    {
                        printf("hi %d\n", thisRS.entries[j].op1_base_valid);
                        if (thisRS.entries[j].op1_base_valid == TRUE && thisRS.entries[j].op1_ready == FALSE)
                        {
                            thisRS.entries[j].op1_ready = TRUE;
                            thisRS.entries[j].op1_combined_val = thisRS.entries[j].op1_base_val;
                            evaluated_op1 = true;
                        }
                    }
                    else if (thisRS.entries[j].op1_addr_mode == based)
                    {
                        if (thisRS.entries[j].op1_base_valid == TRUE && thisRS.entries[j].op1_ready == FALSE)
                        {
                            thisRS.entries[j].op1_ready = TRUE;
                            thisRS.entries[j].op1_combined_val = thisRS.entries[j].op1_base_val + thisRS.entries[j].op1_imm;
                            evaluated_op1 = true;
                        }
                    }
                    else if (thisRS.entries[j].op1_addr_mode == indexed)
                    {
                        if (thisRS.entries[j].op1_index_valid == TRUE && thisRS.entries[j].op1_ready == FALSE)
                        {
                            thisRS.entries[j].op1_ready = TRUE;
                            thisRS.entries[j].op1_combined_val = thisRS.entries[j].op1_index_val * thisRS.entries[j].op1_scale + thisRS.entries[j].op1_imm;
                            evaluated_op1 = true;
                        }
                    }
                    else if (thisRS.entries[j].op1_addr_mode == based_indexed)
                    {
                        if (thisRS.entries[j].op1_base_valid == TRUE && thisRS.entries[j].op1_index_valid == TRUE && thisRS.entries[j].op1_ready == FALSE)
                        {
                            thisRS.entries[j].op1_ready = TRUE;
                            thisRS.entries[j].op1_combined_val = thisRS.entries[j].op1_base_val + thisRS.entries[j].op1_index_val * thisRS.entries[j].op1_scale;
                            evaluated_op1 = true;
                        }
                    }
                    else if (thisRS.entries[j].op1_addr_mode == based_indexed_disp)
                    {
                        if (thisRS.entries[j].op1_base_valid == TRUE && thisRS.entries[j].op1_index_valid == TRUE && thisRS.entries[j].op1_ready == FALSE)
                        {
                            thisRS.entries[j].op1_ready = TRUE;
                            thisRS.entries[j].op1_combined_val = thisRS.entries[j].op1_base_val + thisRS.entries[j].op1_index_val * thisRS.entries[j].op1_scale + thisRS.entries[j].op1_imm;
                            evaluated_op1 = true;
                        }
                    }
                    // operand 2
                    if (thisRS.entries[j].op2_addr_mode == reg || thisRS.entries[j].op2_addr_mode == indirect)
                    {
                        if (thisRS.entries[j].op2_base_valid == TRUE && thisRS.entries[j].op2_ready == FALSE)
                        {
                            thisRS.entries[j].op2_ready = TRUE;
                            thisRS.entries[j].op2_combined_val = thisRS.entries[j].op2_base_val;
                            evaluated_op2 = true;
                        }
                    }
                    else if (thisRS.entries[j].op2_addr_mode == based)
                    {
                        if (thisRS.entries[j].op2_base_valid == TRUE && thisRS.entries[j].op2_ready == FALSE)
                        {
                            thisRS.entries[j].op2_ready = TRUE;
                            thisRS.entries[j].op2_combined_val = thisRS.entries[j].op2_base_val + thisRS.entries[j].op2_imm;
                            evaluated_op2 = true;
                        }
                    }
                    else if (thisRS.entries[j].op2_addr_mode == indexed)
                    {
                        if (thisRS.entries[j].op2_index_valid == TRUE && thisRS.entries[j].op2_ready == FALSE)
                        {
                            thisRS.entries[j].op2_ready = TRUE;
                            thisRS.entries[j].op2_combined_val = thisRS.entries[j].op2_index_val * thisRS.entries[j].op2_scale + thisRS.entries[j].op2_imm;
                            evaluated_op2 = true;
                        }
                    }
                    else if (thisRS.entries[j].op2_addr_mode == based_indexed)
                    {
                        if (thisRS.entries[j].op2_base_valid == TRUE && thisRS.entries[j].op2_index_valid == TRUE && thisRS.entries[j].op2_ready == FALSE)
                        {
                            thisRS.entries[j].op2_ready = TRUE;
                            thisRS.entries[j].op2_combined_val = thisRS.entries[j].op2_base_val + thisRS.entries[j].op2_index_val * thisRS.entries[j].op2_scale;
                            evaluated_op2 = true;
                        }
                    }
                    else if (thisRS.entries[j].op2_addr_mode == based_indexed_disp)
                    {
                        if (thisRS.entries[j].op2_base_valid == TRUE && thisRS.entries[j].op2_index_valid == TRUE && thisRS.entries[j].op2_ready == FALSE)
                        {
                            thisRS.entries[j].op2_ready = TRUE;
                            thisRS.entries[j].op2_combined_val = thisRS.entries[j].op2_base_val + thisRS.entries[j].op2_index_val * thisRS.entries[j].op2_scale + thisRS.entries[j].op2_imm;
                            evaluated_op2 = true;
                        }
                    }
                    // compare operands to store queue and set load alias or data if you can, if not have to allocate load queue
                    // remember dont put in lq if reg or direct, reg shouldnt go in lq and direct has already been allocated
                    // operand 1 search - do the search if addressing mode not immediate, register, or direct
                    if (thisRS.entries[j].op1_addr_mode > direct)
                    {
                        bool found_data = false;
                        bool found_tag = false;
                        int current_old_bits = max_sq_size;
                        for (int k = 0; k < max_sq_size; k++)
                        {
                            if (sq.entries[k].entry_valid == TRUE && sq.entries[k].address == thisRS.entries[j].op1_combined_val &&
                                sq.entries[k].old_bits < current_old_bits)
                            {
                                current_old_bits = sq.entries[k].old_bits;
                                if (sq.entries[k].valid == TRUE)
                                {
                                    found_data = true;
                                    for (int l = 0; l < bytes_on_data_bus; l++)
                                    {
                                        thisRS.entries[j].op1_data = sq.entries[k].data[l];
                                    }
                                    thisRS.entries[j].op1_valid = TRUE;
                                }
                                else
                                {
                                    found_tag = true;
                                    thisRS.entries[j].op1_mem_alias = sq.entries[k].tag;
                                    thisRS.entries[j].op1_mem_alias_valid = TRUE;
                                }
                            }
                        }

                        // allocate lq for operand 1 if load alias/data not found
                        if (found_data == false && found_tag == false)
                        {
                            int alias = mempool.get();
                            for (int i = 0; i < max_lq_size; i++)
                            {
                                if (lq.entries[i].entry_valid == FALSE)
                                {
                                    lq.entries[i].entry_valid = TRUE;
                                    lq.entries[i].address_valid = TRUE;
                                    lq.entries[i].address = thisRS.entries[j].op1_combined_val;
                                    lq.entries[i].old_bits = lq.occupancy;
                                    lq.occupancy++;
                                    lq.entries[i].valid = FALSE;
                                    lq.entries[i].tag = alias;
                                    break;
                                }
                            }
                            thisRS.entries[j].op1_load_alias = alias;
                            thisRS.entries[j].op1_load_alias_valid = TRUE;
                        }

                        // allocate sq for operand 1
                        for (int i = 0; i < max_sq_size; i++)
                        {
                            if (sq.entries[i].entry_valid == FALSE)
                            {
                                sq.entries[i].entry_valid = TRUE;
                                sq.entries[i].address_valid = TRUE;
                                sq.entries[i].address = thisRS.entries[j].op1_combined_val;
                                sq.entries[i].old_bits = sq.occupancy;
                                sq.occupancy++;
                                sq.entries[i].valid = FALSE;
                                sq.entries[i].tag = thisRS.entries[entry_index].store_tag;
                                break;
                            }
                        }
                    }
                    // operand 2 search
                    if (thisRS.entries[j].op2_addr_mode > direct)
                    {
                        bool found_data = false;
                        bool found_tag = false;
                        int current_old_bits = max_sq_size;
                        for (int k = 0; k < max_sq_size; k++)
                        {
                            if (sq.entries[k].entry_valid == TRUE && sq.entries[k].address == thisRS.entries[j].op2_combined_val &&
                                sq.entries[k].old_bits < current_old_bits)
                            {
                                current_old_bits = sq.entries[k].old_bits;
                                if (sq.entries[k].valid == TRUE)
                                {
                                    found_data = true;
                                    for (int l = 0; l < bytes_on_data_bus; l++)
                                    {
                                        thisRS.entries[j].op2_data = sq.entries[k].data[l];
                                    }
                                    thisRS.entries[j].op2_valid = TRUE;
                                }
                                else
                                {
                                    found_tag = true;
                                    thisRS.entries[j].op2_mem_alias = sq.entries[k].tag;
                                    thisRS.entries[j].op2_mem_alias_valid = TRUE;
                                }
                            }
                        }

                        // allocate lq for operand 2 if load alias/data not found
                        // potential edge case to think about: what if operand 1 and 2 are the same address? we don't want to allocate
                        // 2 entries in the lq for the same thing
                        if (found_data == false && found_tag == false)
                        {
                            int alias = mempool.get();
                            for (int i = 0; i < max_lq_size; i++)
                            {
                                if (lq.entries[i].entry_valid == FALSE)
                                {
                                    lq.entries[i].entry_valid = TRUE;
                                    lq.entries[i].address_valid = TRUE;
                                    lq.entries[i].address = thisRS.entries[j].op2_combined_val;
                                    lq.entries[i].old_bits = lq.occupancy;
                                    lq.occupancy++;
                                    lq.entries[i].valid = FALSE;
                                    lq.entries[i].tag = alias;
                                    break;
                                }
                            }
                            thisRS.entries[j].op2_load_alias = alias;
                            thisRS.entries[j].op2_load_alias_valid = TRUE;
                        }
                    }
                }
                if (evaluated_op1 == false || evaluated_op2 == false)
                {
                    search_age++;
                    j = -1;
                }
            }
            if (evaluated_op1 == true || evaluated_op2 == true)
            {
                break;
            }
        }
    }

    if (pipeline.rr_valid == FALSE)
    {
        return;
    }
    if (rob.occupancy == rob_size)
    {
        printf("hello\n");
        regren_stall = TRUE;
        return;
    }

    // want to determine what the operands will be
    // operand 1 (destination)
    //printf("%d\n", pipeline.rr_op1_addr_mode);
    //printf("%d\n", pipeline.rr_op2_addr_mode);
    //printf("\n");

    int alias;

    if(pipeline.rr_halt==0){
        if (pipeline.rr_op1_addr_mode == immediate)
        {
            thisRS.entries[entry_index].old_bits = thisRS.occupancy;
            thisRS.occupancy++;
            thisRS.entries[entry_index].op1_addr_mode = pipeline.rr_op1_addr_mode;
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            thisRS.entries[entry_index].op1_ready = TRUE;
            thisRS.entries[entry_index].op1_combined_val = pipeline.rr_op1_base;
        }
        else if (pipeline.rr_op1_addr_mode == reg)
        {
            thisRS.entries[entry_index].old_bits = thisRS.occupancy;
            thisRS.occupancy++;
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].op1_addr_mode = pipeline.rr_op1_addr_mode;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            thisRS.entries[entry_index].op1_base_valid = rat.valid[pipeline.rr_op1_base];
            thisRS.entries[entry_index].op1_base_tag = rat.tag[pipeline.rr_op1_base];
            thisRS.entries[entry_index].op1_base_val = rat.val[pipeline.rr_op1_base];
            if (rat.valid[pipeline.rr_op1_base] == TRUE)
            {
                thisRS.entries[entry_index].op1_ready = TRUE;
                thisRS.entries[entry_index].op1_combined_val = rat.val[pipeline.rr_op1_base];
            }
            rat.valid[pipeline.rr_op1_base] = FALSE;
            alias = rat.getAlias();
            rat.tag[pipeline.rr_op1_base] = alias;
            thisRS.entries[entry_index].store_tag = alias;
            thisRS.entries[entry_index].store_tag_valid = TRUE;
        }
        else if (pipeline.rr_op1_addr_mode == direct)
        {
            thisRS.entries[entry_index].old_bits = thisRS.occupancy;
            thisRS.occupancy++;
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].op1_addr_mode = pipeline.rr_op1_addr_mode;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            // check store queue
            // this is the case where the address has already been computed
            thisRS.entries[entry_index].op1_combined_val = pipeline.rr_op1_base;
            thisRS.entries[entry_index].op1_ready = TRUE;

            // since in direct mode the address is fully computed, can immediately search the store queue
            bool found_data = false;
            bool found_tag = false;
            int current_old_bits = max_sq_size;
            for (int i = 0; i < max_sq_size; i++)
            {
                if (sq.entries[i].entry_valid == TRUE && sq.entries[i].address == pipeline.rr_op1_base &&
                    sq.entries[i].old_bits < current_old_bits)
                {
                    current_old_bits = sq.entries[i].old_bits;
                    if (sq.entries[i].valid == TRUE)
                    {
                        found_data = true;
                        for (int j = 0; j < bytes_on_data_bus; j++)
                        {
                            thisRS.entries[entry_index].op1_data = sq.entries[i].data[j];
                        }
                        thisRS.entries[entry_index].op1_valid = TRUE;
                    }
                    else
                    {
                        found_tag = true;
                        thisRS.entries[entry_index].op1_mem_alias = sq.entries[i].tag;
                        thisRS.entries[entry_index].op1_mem_alias_valid = TRUE;
                    }
                }
            }

            // allocate entry in load queue if store queue search failed
            // need address, and some form of alias to connect the data in load queue to the
            // operand in the RS entry
            if (found_data == false && found_tag == false)
            {
                int mem_alias = mempool.get();
                for (int i = 0; i < max_lq_size; i++)
                {
                    if (lq.entries[i].entry_valid == FALSE)
                    {
                        lq.entries[i].entry_valid = TRUE;
                        lq.entries[i].address_valid = TRUE;
                        lq.entries[i].address = pipeline.rr_op1_base;
                        lq.entries[i].old_bits = lq.occupancy;
                        lq.occupancy++;
                        lq.entries[i].valid = FALSE;
                        lq.entries[i].tag = mem_alias;
                        break;
                    }
                }
                thisRS.entries[entry_index].op1_load_alias = mem_alias;
                thisRS.entries[entry_index].op1_load_alias_valid = TRUE;
            }

            // allocate entry in store queue
            // need address, and the store tag of the RS entry to receive the data
            for (int i = 0; i < max_sq_size; i++)
            {
                if (sq.entries[i].entry_valid == FALSE)
                {
                    sq.entries[i].entry_valid = TRUE;
                    sq.entries[i].address_valid = TRUE;
                    sq.entries[i].address = pipeline.rr_op1_base;
                    sq.entries[i].old_bits = sq.occupancy;
                    sq.occupancy++;
                    sq.entries[i].valid = FALSE;
                    sq.entries[i].tag = thisRS.entries[entry_index].store_tag;
                    break;
                }
            }
        }
        else if (pipeline.rr_op1_addr_mode == indirect ||
                pipeline.rr_op1_addr_mode == based ||
                pipeline.rr_op1_addr_mode == indexed ||
                pipeline.rr_op1_addr_mode == based_indexed ||
                pipeline.rr_op1_addr_mode == based_indexed_disp)
        {
            thisRS.entries[entry_index].old_bits = thisRS.occupancy;
            thisRS.occupancy++;
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            thisRS.entries[entry_index].op1_mem_alias_valid = FALSE;
            thisRS.entries[entry_index].op1_addr_mode = pipeline.rr_op1_addr_mode;
            thisRS.entries[entry_index].op1_base_valid = rat.valid[pipeline.rr_op1_base];
            thisRS.entries[entry_index].op1_base_tag = rat.tag[pipeline.rr_op1_base];
            thisRS.entries[entry_index].op1_base_val = rat.val[pipeline.rr_op1_base];
            thisRS.entries[entry_index].op1_index_valid = rat.valid[pipeline.rr_op1_index];
            thisRS.entries[entry_index].op1_index_tag = rat.tag[pipeline.rr_op1_index];
            thisRS.entries[entry_index].op1_index_val = rat.val[pipeline.rr_op1_index];
            thisRS.entries[entry_index].op1_scale = pipeline.rr_op1_scale;
            thisRS.entries[entry_index].op1_imm = pipeline.rr_op1_disp;
            thisRS.entries[entry_index].op1_ready = FALSE;
            thisRS.entries[entry_index].op1_load_alias_valid = FALSE;
            thisRS.entries[entry_index].op1_valid = FALSE;
            for(int i=0;i<max_lq_size;i++){
                if(lq.entries[i].entry_valid==0){
                    lq.entries[i].entry_valid=1;
                    lq.entries[i].valid=0;
                    lq.entries[i].old_bits=lq.occupancy;
                    lq.occupancy++;
                    if(pipeline.rr_op1_addr_mode == 2){
                        lq.entries[i].address_valid=rat.valid[pipeline.rr_op1_base];
                        lq.entries[i].address_tag=rat.tag[pipeline.rr_op1_base];
                        lq.entries[i].address=rat.val[pipeline.rr_op1_base];
                    }else{

                    }
                    break;
                }
            }
        }
        // operand 2 (source)
        if (pipeline.rr_op2_addr_mode == immediate)
        {
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            thisRS.entries[entry_index].op2_addr_mode = pipeline.rr_op2_addr_mode;
            thisRS.entries[entry_index].op2_ready = TRUE;
            thisRS.entries[entry_index].op2_combined_val = pipeline.rr_op2_base;
        }
        else if (pipeline.rr_op2_addr_mode == reg)
        {
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            thisRS.entries[entry_index].op2_base_valid = rat.valid[pipeline.rr_op2_base];
            thisRS.entries[entry_index].op2_addr_mode = pipeline.rr_op2_addr_mode;
            thisRS.entries[entry_index].op2_base_tag = rat.tag[pipeline.rr_op2_base];
            thisRS.entries[entry_index].op2_base_val = rat.val[pipeline.rr_op2_base];
            if (rat.valid[pipeline.rr_op2_base] == TRUE)
            {
                thisRS.entries[entry_index].op2_ready = TRUE;
                thisRS.entries[entry_index].op2_combined_val = rat.val[pipeline.rr_op2_base];
            }
        }
        else if (pipeline.rr_op2_addr_mode == direct)
        {
            // check store queue
            // if there's a match, propagate that alias to RS
            // otherwise, allocate entry in load queue and propagate that alias to RS

            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            // check store queue
            // this is the case where the address has already been computed
            thisRS.entries[entry_index].op2_combined_val = pipeline.rr_op2_base;
            thisRS.entries[entry_index].op2_addr_mode = pipeline.rr_op2_addr_mode;
            thisRS.entries[entry_index].op2_ready = TRUE;

            // since in direct mode the address is fully computed, can immediately search the store queue
            bool found_data = false;
            bool found_tag = false;
            int current_old_bits = max_sq_size;
            for (int i = 0; i < max_sq_size; i++)
            {
                if (sq.entries[i].entry_valid == TRUE && sq.entries[i].address == pipeline.rr_op2_base &&
                    sq.entries[i].old_bits < current_old_bits)
                {
                    current_old_bits = sq.entries[i].old_bits;
                    if (sq.entries[i].valid == TRUE)
                    {
                        found_data = true;
                        for (int j = 0; j < bytes_on_data_bus; j++)
                        {
                            thisRS.entries[entry_index].op2_data = sq.entries[i].data[j];
                        }
                        thisRS.entries[entry_index].op2_valid = TRUE;
                    }
                    else
                    {
                        found_tag = true;
                        thisRS.entries[entry_index].op2_mem_alias = sq.entries[i].tag;
                        thisRS.entries[entry_index].op2_mem_alias_valid = TRUE;
                    }
                }
            }

            // allocate entry in load queue if store queue search failed
            // need address, and some form of alias to connect the data in load queue to the
            // operand in the RS entry
            if (found_data == false && found_tag == false)
            {
                int alias = mempool.get();
                for (int i = 0; i < max_lq_size; i++)
                {
                    if (lq.entries[i].entry_valid == FALSE)
                    {
                        lq.entries[i].entry_valid = TRUE;
                        lq.entries[i].address_valid = TRUE;
                        lq.entries[i].address = pipeline.rr_op1_base;
                        lq.entries[i].old_bits = lq.occupancy;
                        lq.occupancy++;
                        lq.entries[i].valid = FALSE;
                        lq.entries[i].tag = alias;
                        break;
                    }
                }
                thisRS.entries[entry_index].op2_load_alias = alias;
                thisRS.entries[entry_index].op2_load_alias_valid = TRUE;
            }
        }
        else if (pipeline.rr_op2_addr_mode == indirect ||
                pipeline.rr_op2_addr_mode == based ||
                pipeline.rr_op2_addr_mode == indexed ||
                pipeline.rr_op2_addr_mode == based_indexed ||
                pipeline.rr_op2_addr_mode == based_indexed_disp)
        {
            thisRS.entries[entry_index].old_bits = thisRS.occupancy;
            thisRS.occupancy++;
            thisRS.entries[entry_index].entry_valid = TRUE;
            thisRS.entries[entry_index].updated_flags = pipeline.rr_updated_flags;
            thisRS.entries[entry_index].op2_mem_alias_valid = FALSE;
            thisRS.entries[entry_index].op2_addr_mode = pipeline.rr_op1_addr_mode;
            thisRS.entries[entry_index].op2_base_valid = rat.valid[pipeline.rr_op2_base];
            thisRS.entries[entry_index].op2_base_tag = rat.tag[pipeline.rr_op2_base];
            thisRS.entries[entry_index].op2_base_val = rat.val[pipeline.rr_op2_base];
            thisRS.entries[entry_index].op2_index_valid = rat.valid[pipeline.rr_op2_index];
            thisRS.entries[entry_index].op2_index_tag = rat.tag[pipeline.rr_op2_index];
            thisRS.entries[entry_index].op2_index_val = rat.val[pipeline.rr_op2_index];
            thisRS.entries[entry_index].op2_scale = pipeline.rr_op2_scale;
            thisRS.entries[entry_index].op2_imm = pipeline.rr_op2_disp;
            thisRS.entries[entry_index].op2_ready = FALSE;
            thisRS.entries[entry_index].op2_load_alias_valid = FALSE;
            thisRS.entries[entry_index].op2_valid = FALSE;
        }
    }
    stations[pipeline.rr_operation] = thisRS;
    for (int i = 0; i < rob_size; i++)
    {
        if (rob.entries[i].valid == FALSE && rob.entries[i].retired == FALSE)
        {
            rob.entries[i].valid = TRUE;
            rob.entries[i].old_bits = rob.occupancy;
            rob.occupancy++;
            rob.entries[i].retired = FALSE;
            rob.entries[i].executed = FALSE;
            rob.entries[i].instruction_ID = globalID;
            rob.entries[i].store_tag = alias;
            rob.entries[i].cat_alias = pipeline.rr_cat_alias;
            rob.entries[i].halt = pipeline.rr_halt;
            break;
        }
    }
    thisRS.entries[entry_index].instruction_ID = globalID;
    thisRS.entries[entry_index].mode32_16 = pipeline.rr32_16;
    //cat.makeAliases(pipeline.rr_updated_flags, alias);
    globalID++;
}

void addgen_branch_stage()
{
    if (regren_stall == TRUE)
    {
        printf("agstall\n");
        return; // ***perhaps we can still predict even if stalling? worth discussing
    }
    if (pipeline.agbr_valid && pipeline.agbr_cs[is_control_instruction])
    { // add control store bit for this
        if(!pipeline.agbr_cs[control_instruction_type]){

        }else{
            int desiredFlags[Flag_Count];
            if(pipeline.agbr_opcode == 85 || pipeline.agbr_opcode == 75){
                desiredFlags[ZF]=1;
            }else if(pipeline.agbr_opcode == 87 || pipeline.agbr_opcode == 77){
                desiredFlags[ZF]=1;
                desiredFlags[CF]=1;
            }

            //compare desiredFlags to cat valid
            int needToPredict=0;
            int ZFtag=-1, CFtag=-1;
            for(int i =0;i<Flag_Count;i++){
                if(desiredFlags[i]==1){
                    if(cat.valid[i]!=1){
                        needToPredict=1;
                        if(i==ZF){
                            ZFtag=cat.tag[i];
                        }else if(i==CF){
                            CFtag=cat.tag[i];
                        }
                    }
                }
            }
            if(needToPredict){

                bool prediction = branch_predictor->predict();
                tempEIP = EIP; // store this EIP in case need to resteer
                tempOffset = pipeline.agbr_offset;
                if (prediction)
                {
                    EIP = pipeline.agbr_NEIP + pipeline.agbr_offset;
                }
                // something to let us know we are spec executing
                
                for(int i =0;i<spec_exe_tracker_size;i++){
                    if(br_tracker.entries[i].valid!=1){
                        br_tracker.entries[i].valid=1;
                        br_tracker.entries[i].not_taken_target = pipeline.agbr_NEIP;
                        br_tracker.entries[i].taken_target = pipeline.agbr_NEIP + pipeline.agbr_offset;
                        br_tracker.entries[i].prediction = prediction;
                        for(int j =0;j<Flag_Count;j++){
                            if(desiredFlags[j]==1 && j == ZF){
                                br_tracker.entries[i].flagTags[j] = ZFtag;
                            }else if(desiredFlags[j]==1 && j == CF){
                                br_tracker.entries[i].flagTags[j] = CFtag;
                            }else{
                                br_tracker.entries[i].flagTags[j] = -1;
                            }

                            if(ZFtag==-1){
                                br_tracker.entries[i].flagValid[ZF]=1;
                            }
                            if(CFtag==-1){
                                br_tracker.entries[i].flagValid[CF]=1;
                            }
                        }
                    }
                }
            }
        }
    }
    new_pipeline.rr_valid = pipeline.agbr_valid;
    new_pipeline.rr_operation = pipeline.agbr_cs[operation];
    new_pipeline.rr_updated_flags = pipeline.agbr_cs[updated_flags];
    new_pipeline.rr_op1_base = pipeline.agbr_op1_base;
    new_pipeline.rr_op1_index = pipeline.agbr_op1_index;
    new_pipeline.rr_op1_scale = pipeline.agbr_op1_scale;
    new_pipeline.rr_op1_disp = pipeline.agbr_op1_disp;
    new_pipeline.rr_op1_addr_mode = pipeline.agbr_cs[op1_addr_mode];
    new_pipeline.rr_op2_base = pipeline.agbr_op2_base;
    new_pipeline.rr_op2_index = pipeline.agbr_op2_index;
    new_pipeline.rr_op2_scale = pipeline.agbr_op2_scale;
    new_pipeline.rr_op2_disp = pipeline.agbr_op2_disp;
    new_pipeline.rr_op2_addr_mode = pipeline.agbr_cs[op2_addr_mode];
    new_pipeline.rr32_16 = pipeline.agbr32_16;
    int cat_alias = cat.getAlias();
    new_pipeline.rr_cat_alias = cat_alias;
    cat.makeAliases(new_pipeline.rr_updated_flags, cat_alias);
    new_pipeline.rr_halt = pipeline.agbr_halt;
}

#define operationRows 18
#define operationCols 14
#define addressingRows 26
#define addressingCols 9
#define operandRows 15
#define operandCols 7
int operationLUT[operationRows][operationCols] =
    {12, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     5, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     128, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     129, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     131, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     2, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     3, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     12, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     13, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     128, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     129, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     131, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     8, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     9, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     10, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0,
     11, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0};
int addressingLUT[addressingRows][addressingCols] =
    {
        4, 0, 0, 0, 1, 0, 0, 1, 1,
        5, 0, 0, 0, 1, 0, 0, 1, 1,
        12, 0, 0, 0, 1, 0, 0, 1, 1,
        13, 0, 0, 0, 1, 0, 0, 1, 1,
        128, 1, 3, 0, 1, 0, 0, 1, 1, // note: 128 = x80
        128, 1, 0, 1, 0, 0, 0, 1, 1,
        129, 1, 3, 0, 1, 0, 0, 1, 1,
        129, 1, 0, 1, 0, 0, 0, 1, 1,
        131, 1, 3, 0, 1, 0, 0, 1, 1,
        131, 1, 0, 1, 0, 0, 0, 1, 1,
        0, 1, 3, 0, 1, 0, 1, 1, 1,
        0, 1, 0, 1, 0, 0, 1, 1, 1,
        1, 1, 3, 0, 1, 0, 1, 1, 1,
        1, 1, 0, 1, 0, 0, 1, 1, 1,
        8, 1, 3, 0, 1, 0, 1, 1, 1,
        8, 1, 0, 1, 0, 0, 1, 1, 1,
        9, 1, 3, 0, 1, 0, 1, 1, 1,
        9, 1, 0, 1, 0, 0, 1, 1, 1,
        2, 1, 3, 0, 1, 0, 1, 1, 1,
        2, 1, 0, 0, 1, 1, 0, 1, 1,
        3, 1, 3, 0, 1, 0, 1, 1, 1,
        3, 1, 0, 0, 1, 1, 0, 1, 1,
        10, 1, 3, 0, 1, 0, 1, 1, 1,
        10, 1, 0, 0, 1, 1, 0, 1, 1,
        11, 1, 3, 0, 1, 0, 1, 1, 1,
        11, 1, 0, 0, 1, 1, 0, 1, 1};
int operandLUT[operandRows][operandCols] =
    {4, 0, 0, 0, 0, 1, 1,
     5, 0, 0, 0, 0, 1, 1,
     12, 0, 0, 0, 0, 1, 1,
     13, 0, 0, 0, 0, 1, 1,
     128, 0, 1, 0, 0, 1, 1,
     129, 0, 1, 0, 0, 1, 1,
     131, 0, 1, 0, 1, 0, 0,
     0, 0, 1, 0, 0, 0, 1,
     1, 0, 1, 0, 0, 0, 1,
     8, 0, 1, 0, 0, 0, 1,
     9, 0, 1, 0, 0, 0, 1,
     2, 0, 0, 1, 0, 1, 0,
     3, 0, 0, 1, 0, 1, 0,
     10, 0, 0, 1, 0, 1, 0,
     11, 0, 0, 1, 0, 1, 0};

void decode_stage()
{
    if (regren_stall == TRUE)
    {
        printf("decstall\n");
        return;
    }
    // use the pipeline latches to lookup in the LUTs
    // search operationLUT
    int mod = pipeline.decode_modrm >> 6;
    int reg = (pipeline.decode_modrm >> 3) & 0x7;
    int rm = (pipeline.decode_modrm) & 0x7;
    int scale = pipeline.decode_sib >> 6;
    int index = (pipeline.decode_sib >> 3) & 0x7;
    int base = (pipeline.decode_sib) & 0x7;
    for (int i = 0; i < operationRows; i++)
    {
        // printf("op %d", pipeline.decode_opcode);
        // printf("lu %d\n",operationLUT[i][0]);
        if (pipeline.decode_opcode == operationLUT[i][0])
        {
            if (operationLUT[i][1] == 1)
            {
                // printf("hello\n");
                if (reg == operationLUT[i][2])
                {

                    // can latch operation, updated flags, and cntrl here
                    new_pipeline.agbr_cs[operation] = operationLUT[i][3];
                    new_pipeline.agbr_cs[updated_flags] = (operationLUT[i][4] << 7) +
                                                          (operationLUT[i][5] << 6) +
                                                          (operationLUT[i][6] << 5) +
                                                          (operationLUT[i][7] << 4) +
                                                          (operationLUT[i][8] << 3) +
                                                          (operationLUT[i][9] << 2) +
                                                          (operationLUT[i][10] << 1) +
                                                          operationLUT[i][11];
                    new_pipeline.agbr_cs[is_control_instruction] = operationLUT[i][12];
                    new_pipeline.agbr_cs[control_instruction_type] = operationLUT[i][13];
                    break;
                }
            }
            else
            {
                new_pipeline.agbr_cs[operation] = operationLUT[i][3];
                // printf("hi\n");
                new_pipeline.agbr_cs[updated_flags] = (operationLUT[i][4] << 7) +
                                                      (operationLUT[i][5] << 6) +
                                                      (operationLUT[i][6] << 5) +
                                                      (operationLUT[i][7] << 4) +
                                                      (operationLUT[i][8] << 3) +
                                                      (operationLUT[i][9] << 2) +
                                                      (operationLUT[i][10] << 1) +
                                                      operationLUT[i][11];
                new_pipeline.agbr_cs[is_control_instruction] = operationLUT[i][12];
                new_pipeline.agbr_cs[control_instruction_type] = operationLUT[i][13];
                break;
            }
        }
    }
    // search addressingLUT
    int locked = 0;
    for (int i = 0; i < addressingRows; i++)
    {
        if (locked == 1)
        {
            new_pipeline.agbr_cs[op1_addr_mode] = (addressingLUT[i][3] << 1) + addressingLUT[i][4];
            new_pipeline.agbr_cs[op2_addr_mode] = (addressingLUT[i][5] << 1) + addressingLUT[i][6];
            new_pipeline.agbr_cs[is_op1_needed] = addressingLUT[i][7];
            new_pipeline.agbr_cs[is_op2_needed] = addressingLUT[i][8];
            break;
        }
        if (pipeline.decode_opcode == addressingLUT[i][0])
        {
            if (addressingLUT[i][1] == 1)
            {
                if (mod == addressingLUT[i][2])
                {
                    // printf("addresing lut%d\n", addressingLUT[i][2]);
                    new_pipeline.agbr_cs[op1_addr_mode] = (addressingLUT[i][3] << 1) + addressingLUT[i][4];
                    new_pipeline.agbr_cs[op2_addr_mode] = (addressingLUT[i][5] << 1) + addressingLUT[i][6];
                    // printf("%d %d\n", )
                    new_pipeline.agbr_cs[is_op1_needed] = addressingLUT[i][7];
                    new_pipeline.agbr_cs[is_op2_needed] = addressingLUT[i][8];
                    break;
                }
                else
                {
                    locked = 1;
                    continue;
                }
            }
            else
            {
                new_pipeline.agbr_cs[op1_addr_mode] = (addressingLUT[i][3] << 1) + addressingLUT[i][4];
                new_pipeline.agbr_cs[op2_addr_mode] = (addressingLUT[i][5] << 1) + addressingLUT[i][6];
                new_pipeline.agbr_cs[is_op1_needed] = addressingLUT[i][7];
                new_pipeline.agbr_cs[is_op2_needed] = addressingLUT[i][8];
            }
        }
    }

    //SIB LOGIC
    if(new_pipeline.agbr_cs[op1_addr_mode]==2){
        if((mod==0 && rm == 5)||(mod==0 && rm ==4 && index ==4 && base == 5)){
            new_pipeline.agbr_cs[op1_addr_mode]=2;
        }else if(mod==0 && rm != 4 && rm != 5){
            new_pipeline.agbr_cs[op1_addr_mode]=3;
        }else if((mod == 0 && rm == 4 && index ==4 && base != 5) || ((mod == 1 || mod == 2) && rm == 4 && index == 4) || ((mod == 1 || mod == 2) && rm == 4)){
            new_pipeline.agbr_cs[op1_addr_mode]=4;
        }else if(mod == 0 && rm == 4 && index != 4 && base == 5){
            new_pipeline.agbr_cs[op1_addr_mode]=5;
        }else if(mod == 0 && rm == 4 && index != 4 && base != 5){
            new_pipeline.agbr_cs[op1_addr_mode]=6;
        }else if((mod == 1 || mod == 2) && rm == 4 && index != 4){
            new_pipeline.agbr_cs[op1_addr_mode]=7;
        }
    }
    if(new_pipeline.agbr_cs[op2_addr_mode]==2){
        if((mod==0 && rm == 5)||(mod==0 && rm ==4 && index ==4 && base == 5)){
            new_pipeline.agbr_cs[op2_addr_mode]=2;
        }else if(mod==0 && rm != 4 && rm != 5){
            new_pipeline.agbr_cs[op2_addr_mode]=3;
        }else if((mod == 0 && rm == 4 && index ==4 && base != 5) || ((mod == 1 || mod == 2) && rm == 4 && index == 4) || ((mod == 1 || mod == 2) && rm == 4)){
            new_pipeline.agbr_cs[op2_addr_mode]=4;
        }else if(mod == 0 && rm == 4 && index != 4 && base == 5){
            new_pipeline.agbr_cs[op2_addr_mode]=5;
        }else if(mod == 0 && rm == 4 && index != 4 && base != 5){
            new_pipeline.agbr_cs[op2_addr_mode]=6;
        }else if((mod == 1 || mod == 2) && rm == 4 && index != 4){
            new_pipeline.agbr_cs[op2_addr_mode]=7;
        }
    }

    // search operandLUT
    int op1base_selector, op2base_selector;
    // printf("hello %d\n",pipeline.decode_opcode);
    for (int i = 0; i < operandRows; i++)
    {
        // printf("%d\n",operandLUT[i][0]);
        if (pipeline.decode_opcode == operandLUT[i][0])
        {
            op1base_selector = (operandLUT[i][1] << 2) + (operandLUT[i][2] << 1) + operandLUT[i][3];
            op2base_selector = (operandLUT[i][4] << 2) + (operandLUT[i][5] << 1) + operandLUT[i][6];
        }
    }

    new_pipeline.agbr_NEIP = pipeline.decode_EIP + pipeline.decode_instruction_length;

    // logic stuff
    int disp;
    int imm;
    int sext_imm;
    int disp_offset = 0;
    if (pipeline.decode_1bdisp)
    {
        disp = pipeline.decode_dispimm[0] & 0xFF;
        disp_offset = 1;
    }
    else if (pipeline.decode_4bdisp)
    {
        disp = ((pipeline.decode_dispimm[3] & 0xFF) << 24) + ((pipeline.decode_dispimm[2] & 0xFF) << 16) + ((pipeline.decode_dispimm[1] & 0xFF) << 8) + (pipeline.decode_dispimm[0] & 0xFF);
        disp_offset = 4;
    }
    if (pipeline.decode_1bimm)
    {
        imm = pipeline.decode_dispimm[disp_offset] & 0xFF;
    }
    else if (pipeline.decode_2bimm)
    {
        imm = ((pipeline.decode_dispimm[disp_offset + 1] & 0xFF) << 8) + (pipeline.decode_dispimm[disp_offset] & 0xFF);
    }
    else if (pipeline.decode_4bimm)
    {
        imm = ((pipeline.decode_dispimm[disp_offset + 3] & 0xFF) << 24) + ((pipeline.decode_dispimm[disp_offset + 2] & 0xFF) << 16) +
              ((pipeline.decode_dispimm[disp_offset + 1] & 0xFF) << 8) + (pipeline.decode_dispimm[disp_offset] & 0xFF);
    }
    // do this later
    sext_imm = imm;
    /*printf("op1: %d op2: %d\n",op1base_selector,op2base_selector);
    printf("imm stuff %d %d %d %d %d\n", pipeline.decode_1bdisp, pipeline.decode_4bdisp,
    pipeline.decode_1bimm, pipeline.decode_2bimm, pipeline.decode_4bimm);
    for(int i =0;i<14;i++){
        printf("%x ", pipeline.decode_dispimm[i]);
    }
    printf("\n");*/
    printf("imm: 0x%x\n", imm);
    // select operand bases here
    if (op1base_selector == 0)
    {
        new_pipeline.agbr_op1_base = 0;
    }
    else if (op1base_selector == 1)
    {
        new_pipeline.agbr_op1_base = reg;
    }
    else if (op1base_selector == 2)
    {
        new_pipeline.agbr_op1_base = rm;
    }
    else if (op1base_selector == 3)
    {
        new_pipeline.agbr_op1_base = imm;
    }
    else if (op1base_selector == 4)
    {
        new_pipeline.agbr_op1_base = sext_imm;
    }

    if (op2base_selector == 0)
    {
        new_pipeline.agbr_op2_base = 0;
    }
    else if (op2base_selector == 1)
    {
        new_pipeline.agbr_op2_base = reg;
    }
    else if (op2base_selector == 2)
    {
        new_pipeline.agbr_op2_base = rm;
    }
    else if (op2base_selector == 3)
    {
        new_pipeline.agbr_op2_base = imm;
    }
    else if (op2base_selector == 4)
    {
        new_pipeline.agbr_op2_base = sext_imm;
    }
    // latch the rest of the operand data here
    new_pipeline.agbr_op1_index = index;
    new_pipeline.agbr_op1_scale = scale;
    new_pipeline.agbr_op2_index = index;
    new_pipeline.agbr_op2_scale = scale;
    new_pipeline.agbr_op1_disp = disp;
    new_pipeline.agbr_op2_disp = disp;
    new_pipeline.agbr_opcode = pipeline.decode_opcode;
    new_pipeline.agbr32_16 = pipeline.decode_is_prefix;
    // latch offset and valid
    new_pipeline.agbr_offset = pipeline.decode_offset;
    new_pipeline.agbr_valid = pipeline.decode_valid;

    if(pipeline.decode_opcode==0xF4){
        new_pipeline.agbr_halt = 1;
    }
}

#define max_instruction_length 15
int length;
void predecode_stage()
{
    if (regren_stall == TRUE)
    {
        printf("pstall\n");
        return;
    }
    if (pipeline.predecode_valid)
    {
        new_pipeline.decode_immSize = 0;
        int len, prefix, opcode;
        int instIndex = 0;
        if (pipeline.predecode_potential[instIndex] == 0x66 || pipeline.predecode_potential[instIndex] == 0x67)
        { // check prefix
            len += 2;
            prefix = pipeline.predecode_potential[instIndex];
            instIndex++;
            opcode = pipeline.predecode_potential[instIndex];
            instIndex++;
            new_pipeline.decode_is_prefix = 1;
        }
        else
        {
            opcode = pipeline.predecode_potential[instIndex];
            instIndex++;
            len++;
            new_pipeline.decode_is_prefix = 0;
        }
        if (!(opcode == 0x05 || opcode == 0x04 || opcode== 0xF4))
        { // if an instruction tha requires a mod/rm byte
            len += 1;
            new_pipeline.decode_is_modrm = 1;
            printf("HELLO\n\n\n\n");
            unsigned char mod_rm = pipeline.predecode_potential[instIndex];
            instIndex++;
            new_pipeline.decode_modrm = mod_rm;
            if ((mod_rm & 0b11000000) == 0 && (mod_rm & 0b0100) != 0)
            { // uses SIB byte
                new_pipeline.decode_is_sib = true;
                len++;
                new_pipeline.decode_sib = pipeline.predecode_potential[instIndex];
                instIndex++;
            }
            else
            {
                new_pipeline.decode_is_sib = false;
            }
            int displacement = mod_rm & 0b11000000;
            if (displacement == 1)
            {
                len++;
                new_pipeline.decode_1bdisp = true;
            }
            if (displacement == 2)
            {
                len += 4;
                new_pipeline.decode_4bdisp = true;
            }
        }
        else
        {
            new_pipeline.decode_is_modrm = false;
        }
        if (((opcode == 0x81) && prefix == 0x66) || ((opcode == 0x05) && prefix == 0x66))
        { // determine size of immediates
            len += 2;
            new_pipeline.decode_immSize = 2;
            new_pipeline.decode_2bimm = true;
        }
        else if (opcode == 0x81 || opcode == 0x05)
        {
            len += 4;
            new_pipeline.decode_immSize = 4;
            new_pipeline.decode_4bimm = true;
        }
        else if ((opcode == 04) || (opcode == 80) || (opcode == 83))
        { // all instructions with 1 byte immediate
            len++;
            new_pipeline.decode_immSize = 1;
            new_pipeline.decode_1bimm = true;
        }
        int j = 0;
        for (int i = instIndex; i < 15; i++)
        { // copy over all displacement and immediate
            new_pipeline.decode_dispimm[j] = pipeline.predecode_potential[i];
            j++;
        }
        new_pipeline.decode_instruction_length = len;
        new_pipeline.decode_valid = 1;
        for (int i = 0; i < max_instruction_length; i++)
        {
            new_pipeline.decode_instruction_register[i] = pipeline.predecode_potential[i];
        }
        new_pipeline.decode_EIP = EIP;
        new_pipeline.decode_opcode = opcode;
        new_pipeline.decode_prefix = prefix;
        length = len;
        // oldEIP = EIP;
        // EIP+= length;
    }
    else
    {
        new_pipeline.decode_valid = 0;
    }
}

int oldIbuffer[4][16];
int oldIbufferValid[4];
void fetch_stage()
{
    for (int i = 0; i < ibuffer_size; i++)
    { // check if you've read in the terminator instruction (xffff)
        for (int j = 0; j < cache_line_size; j++)
        {
            if (j != cache_line_size - 1)
            {
                if (ibuffer[i][j] == 0x0ff && ibuffer[i][j + 1] == 0x0ff)
                {
                    end_of_file = 1;
                }
            }
        }
    }

    if (regren_stall == TRUE)
    {
        printf("fstall\n");
        return; // *** maybe some fetch stuff can still happen during the stall? worth discussing
    }

    int offset = EIP & 0x3F;
    int current_sector = offset / cache_line_size;
    // int current_sector = offset / ibuffer_size;
    int line_offset = offset % cache_line_size;
    printf("current: %d\n", current_sector);
    // printf("length %d\n", length);
    //  oldEIP = EIP;
    //  EIP += length;
    int tempEIP = EIP + length;
    int temp_offset = tempEIP & 0x3F;
    int temp_current_sector = temp_offset / cache_line_size;
    // int current_sector = offset / ibuffer_size;
    int temp_line_offset = temp_offset % cache_line_size;
    // new_pipeline.predecode_EIP = EIP;
    if (length >= (16 - line_offset))
    {

        ibuffer_valid[current_sector] = FALSE;
    }

    if (temp_line_offset <= 1 || end_of_file)
    {
        if (oldIbufferValid[temp_current_sector] == TRUE)
        {
            // latch predecode valid and ibuffer
            new_pipeline.predecode_valid = TRUE;
            new_pipeline.predecode_offset = temp_offset;
            new_pipeline.predecode_current_sector = temp_current_sector;
            new_pipeline.predecode_line_offset = temp_line_offset;
            for (int i = 0; i < max_instruction_length; i++)
            {
                new_pipeline.predecode_potential[i] = oldIbuffer[temp_current_sector][temp_line_offset];
                temp_line_offset++;
            }
        }
        else
        {

            new_pipeline.predecode_valid = FALSE;
        }
    }
    else
    {
        if ((oldIbufferValid[temp_current_sector] == TRUE) && (oldIbufferValid[(temp_current_sector + 1) % ibuffer_size] == TRUE))
        {
            // latch predecode valid and ibuffer
            new_pipeline.predecode_valid = TRUE;
            new_pipeline.predecode_offset = temp_offset;
            new_pipeline.predecode_current_sector = temp_current_sector;
            new_pipeline.predecode_line_offset = temp_line_offset;
            int tempVal = temp_line_offset;
            int i;
            for (i = 0; i < 16 - temp_line_offset; i++)
            {
                new_pipeline.predecode_potential[i] = oldIbuffer[temp_current_sector][tempVal];
                tempVal++;
            }
            int j;
            int k = 0;
            for (int j = i; j < 15 - (16 - temp_line_offset); j++)
            {
                new_pipeline.predecode_potential[j] = oldIbuffer[temp_current_sector + 1][k];
                k++;
            }
        }
        else
        {
            new_pipeline.predecode_valid = FALSE;
        }
    }

    int bank_aligned = FALSE;
    int bank_offset = FALSE;
    int b, c, d;
    int *dataBits0[cache_line_size], *dataBits1[cache_line_size], *tlb_hit = &c, *tlb_physical_tag = &d;
    I$_TagStoreEntry *icache_tag_metadata = nullptr;
    if (ibuffer_valid[(current_sector)] == FALSE)
    {
        icache_access(EIP, &icache_tag_metadata);
        tlb_access(EIP, tlb_physical_tag, tlb_hit);
        bool condition1 = (*tlb_hit && icache_tag_metadata->valid[0]);
        bool condition2 = (icache_tag_metadata->tag[0] == *tlb_physical_tag);
        bool condition3 = (*tlb_hit && icache_tag_metadata->valid[1]);
        bool condition4 = (icache_tag_metadata->tag[1] == *tlb_physical_tag);
        // printf("tlb hit %d, i$ hit %d\n", condition1, condition2);
        // printf("stuff %d %d %d %d\n", *tlb_hit, icache_tag_metadata->valid[0], icache_tag_metadata->tag[0], *tlb_physical_tag);
        if ((condition1) && (condition2))
        {
            ibuffer_valid[current_sector] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                // printf("hi %d\n", i);
                // printf("%d \n", data_way0[i]);
                // printf("%d \n", ibuffer[current_sector][i]);
                ibuffer[current_sector][i] = data_way0[i];
            }
        }
        else if ((condition3) && (condition4))
        {
            ibuffer_valid[current_sector] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                // printf("hi %d\n", i);
                ibuffer[current_sector][i] = data_way1[i];
            }
        }
        else
        {
            // printf("preinserter1\n");
            mshr_preinserter(EIP, 0, 0);
        }
        bank_aligned = TRUE;
    }
    if (ibuffer_valid[(current_sector + 1) % ibuffer_size] == FALSE)
    {
        icache_access(EIP + 16, &icache_tag_metadata);
        tlb_access(EIP + 16, tlb_physical_tag, tlb_hit);
        bool condition1 = (*tlb_hit && icache_tag_metadata->valid[0]);
        bool condition2 = (icache_tag_metadata->tag[0] == *tlb_physical_tag);
        bool condition3 = (*tlb_hit && icache_tag_metadata->valid[1]);
        bool condition4 = (icache_tag_metadata->tag[1] == *tlb_physical_tag);
        if ((condition1) && (condition2))
        {
            ibuffer_valid[(current_sector + 1) % ibuffer_size] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                ibuffer[(current_sector + 1) % ibuffer_size][i] = data_way0[i];
            }
        }
        else if ((condition3) && (condition4))
        {
            ibuffer_valid[(current_sector + 1) % ibuffer_size] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                ibuffer[(current_sector + 1) % ibuffer_size][i] = data_way1[i];
            }
        }
        else
        {
            // printf("preinserter2\n");
            mshr_preinserter(EIP + 16, 0, 0);
        }
        bank_offset = TRUE;
    }
    if (ibuffer_valid[(current_sector + 2) % ibuffer_size] == FALSE && bank_aligned == FALSE)
    {
        icache_access(EIP + 32, &icache_tag_metadata);
        tlb_access(EIP + 32, tlb_physical_tag, tlb_hit);
        bool condition1 = (*tlb_hit && icache_tag_metadata->valid[0]);
        bool condition2 = (icache_tag_metadata->tag[0] == *tlb_physical_tag);
        bool condition3 = (*tlb_hit && icache_tag_metadata->valid[1]);
        bool condition4 = (icache_tag_metadata->tag[1] == *tlb_physical_tag);
        if ((condition1) && (condition2))
        {
            ibuffer_valid[(current_sector + 2) % ibuffer_size] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                ibuffer[(current_sector + 2) % ibuffer_size][i] = data_way0[i];
            }
        }
        else if ((condition3) && (condition4))
        {
            ibuffer_valid[(current_sector + 2) % ibuffer_size] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                ibuffer[(current_sector + 2) % ibuffer_size][i] = data_way1[i];
            }
        }
        else
        {
            // printf("preinserter3\n");
            mshr_preinserter(EIP + 32, 0, 0);
        }
        bank_aligned = TRUE;
    }
    if (ibuffer_valid[(current_sector + 3) % ibuffer_size] == FALSE && bank_offset == FALSE)
    {
        icache_access(EIP + 48, &icache_tag_metadata);
        tlb_access(EIP + 48, tlb_physical_tag, tlb_hit);
        bool condition1 = (*tlb_hit && icache_tag_metadata->valid[0]);
        bool condition2 = (icache_tag_metadata->tag[0] == *tlb_physical_tag);
        bool condition3 = (*tlb_hit && icache_tag_metadata->valid[1]);
        bool condition4 = (icache_tag_metadata->tag[1] == *tlb_physical_tag);
        if ((condition1) && (condition2))
        {
            ibuffer_valid[(current_sector + 3) % ibuffer_size] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                ibuffer[(current_sector + 3) % ibuffer_size][i] = data_way0[i];
            }
        }
        else if ((condition3) && (condition4))
        {
            ibuffer_valid[(current_sector + 3) % ibuffer_size] = TRUE;
            for (int i = 0; i < cache_line_size; i++)
            {
                ibuffer[(current_sector + 3) % ibuffer_size][i] = data_way1[i];
            }
        }
        else
        {
            // printf("preinserter4\n");
            mshr_preinserter(EIP + 48, 0, 0);
        }
        bank_offset = TRUE;
    }
    // oldEIP = EIP;
    // EIP += length;
    // new_pipeline.predecode_EIP = EIP;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            oldIbuffer[i][j] = ibuffer[i][j];
        }
        oldIbufferValid[i] = ibuffer_valid[i];
    }
    oldEIP = EIP;
    EIP = tempEIP;
}

void mshr_inserter()
{
    // treat serializer as victim cache first for i/dcache
    // printf("inserter\n");
    for (int i = 0; i < pre_mshr_size; i++)
    {
        if (mshr.pre_entries[i].valid && (mshr.pre_entries[i].origin == 0 || mshr.pre_entries[i].origin == 1))
        {
            // iterate through serializer
            for (int j = 0; j < num_serializer_entries; j++)
            {
                if (serializer.entries[j].valid && (serializer.entries[j].address == mshr.pre_entries[i].address))
                {
                    serializer.entries[j].rescue_lock = 1;
                    serializer.entries[j].rescue_destination = mshr.pre_entries[i].origin;
                }
            }
        }
    }
    // insert icache requests
    for (int i = 0; i < pre_mshr_size; i++)
    {
        if ((mshr.pre_entries[i].valid == TRUE) && (mshr.pre_entries[i].origin == 0))
        {
            bool skip = false;
            for (int j = 0; j < mshr_size; j++)
            {
                if ((mshr.entries[j].valid == TRUE) && (mshr.entries[j].address == mshr.pre_entries[i].address))
                {
                    skip = true;
                    mshr.pre_entries[i].valid = FALSE;
                    // printf("skip\n");
                    break;
                }
            }
            if (skip == false)
            {
                for (int j = 0; j < mshr_size; j++)
                {
                    if (mshr.occupancy < mshr_size)
                    {
                        if (mshr.entries[j].valid == FALSE)
                        {
                            // printf("setting i:%d j:%d\n", i,j);
                            mshr.entries[j].valid = TRUE;
                            mshr.entries[j].old_bits = mshr.occupancy;
                            mshr.occupancy++;
                            mshr.entries[j].origin = mshr.pre_entries[i].origin;
                            mshr.entries[j].address = mshr.pre_entries[i].address;
                            mshr.entries[j].request_ID = mshr.pre_entries[i].request_ID;
                            mshr.pre_entries[i].valid = FALSE;
                            mshr.pre_occupancy--;
                            // printf("val %d", mshr.entries[j].valid);
                            return;
                        }
                    }
                }
            }
            else
            {
                // printf("skipping\n");
            }
        }
    }
    // insert dcache requests
    for (int i = 0; i < pre_mshr_size; i++)
    {
        if (mshr.pre_entries[i].valid && mshr.pre_entries[i].origin == 1)
        {
            bool skip = false;
            for (int j = 0; j < mshr_size; j++)
            {
                if ((mshr.entries[j].valid == TRUE) && (mshr.entries[j].address == mshr.pre_entries[i].address))
                {
                    skip = true;
                    break;
                }
            }
            if (skip == false)
            {
                for (int j = 0; j < mshr_size; j++)
                {
                    if (mshr.occupancy < mshr_size)
                    {
                        if (mshr.entries[j].valid == FALSE)
                        {
                            mshr.entries[j].valid = TRUE;
                            mshr.entries[j].old_bits = mshr.occupancy;
                            mshr.occupancy++;
                            mshr.entries[j].origin = mshr.pre_entries[i].origin;
                            mshr.entries[j].address = mshr.pre_entries[i].address;
                            mshr.entries[j].request_ID = mshr.pre_entries[i].request_ID;
                            mshr.pre_entries[i].valid = FALSE;
                            mshr.pre_occupancy--;
                            return;
                        }
                    }
                }
            }
        }
    }
    // insert tlb requests
    for (int i = 0; i < pre_mshr_size; i++)
    {
        if (mshr.pre_entries[i].valid && mshr.pre_entries[i].origin == 2)
        {
            bool skip = false;
            for (int j = 0; j < mshr_size; j++)
            {
                if ((mshr.entries[j].valid) == TRUE && (mshr.entries[j].address == mshr.pre_entries[j].address))
                {
                    skip = true;
                    break;
                }
            }
            if (skip == false)
            {
                for (int j = 0; j < mshr_size; j++)
                {
                    if (mshr.occupancy < mshr_size)
                    {
                        if (mshr.entries[j].valid == FALSE)
                        {
                            mshr.entries[j].valid = TRUE;
                            mshr.entries[j].old_bits = mshr.occupancy;
                            mshr.occupancy++;
                            mshr.entries[j].origin = mshr.pre_entries[i].origin;
                            mshr.entries[j].address = mshr.pre_entries[i].address;
                            mshr.entries[j].request_ID = mshr.pre_entries[i].request_ID;
                            mshr.pre_entries[i].valid = FALSE;
                            mshr.pre_occupancy--;
                            return;
                        }
                    }
                }
            }
        }
    }
}

void mshr_printer()
{
    for (int i = 0; i < pre_mshr_size; i++)
    {
        printf("MSHR Pre-entry %d\n", i);
        printf("Valid: %d | Address: 0x%x | Age: %d | Origin: %d | ReqID: %d\n", mshr.pre_entries[i].valid, mshr.pre_entries[i].address, mshr.pre_entries[i].old_bits, mshr.pre_entries[i].origin, mshr.pre_entries[i].request_ID);
    }
    printf("\n\n\n");
    for (int i = 0; i < mshr_size; i++)
    {
        printf("MSHR Entry %d\n", i);
        printf("Valid: %d | Address: 0x%x | Age: %d | Origin: %d | ReqID: %d\n", mshr.entries[i].valid, mshr.entries[i].address, mshr.entries[i].old_bits, mshr.entries[i].origin, mshr.entries[i].request_ID);
    }
    printf("\n\n\n");

    for (int i = 0; i < max_serializer_occupancy; i++)
    {
        printf("Serializer Entry%d\n", i);
        printf("Valid: %d | Address: 0x%x | Age: %d | Sending Data?: %d\n", serializer.entries[i].valid, serializer.entries[i].address, serializer.entries[i].old_bits, serializer.entries[i].sending_data);
    }
    printf("\n\n\n");
    for (int i = 0; i < max_deserializer_occupancy; i++)
    {
        printf("Deserializer Entry%d\n", i);
        printf("Valid: %d | Address: 0x%x | Age: %d\n", deserializer.entries[i].valid, deserializer.entries[i].address, deserializer.entries[i].old_bits);
    }
    printf("\n\n\n");
}

void rat_printer()
{
    printf("RAT\n");
    for (int i = 0; i < GPR_Count; i++)
    {
        printf("Valid: %d Tag: %d Value: %d \n", rat.valid[i], rat.tag[i], rat.val[i]);
    }
    printf("CAT\n");
    for (int i = 0; i < Flag_Count; i++)
    {
        printf("Valid: %d Tag: %d Value: %d \n", cat.valid[i], cat.tag[i], cat.val[i]);
    }
}

void cache_printer()
{
    printf("ICACHE\n");
    for (int i = 0; i < icache_banks; i++)
    {
        for (int j = 0; j < icache_sets; j++)
        {
            for (int k = 0; k < icache_ways; k++)
            {
                printf("Bank %d Set %d Way %d\n", i, j, k);
                printf("Valid: %d | Tag: 0x%x | LRU: %d | Dirty: %d\n", icache.tag.icache_tagstore[i][j].valid[k], icache.tag.icache_tagstore[i][j].tag[k], icache.tag.icache_tagstore[i][j].lru, icache.tag.icache_tagstore[i][j].dirty[k]);
                printf("Data: 0x");
                for (int l = 0; l < cache_line_size; l++)
                {
                    printf("%x:", icache.data.icache_datastore[i][j][k][l]);
                }
                printf("\n");
            }
            printf("\n");
        }
    }
}

void station_printer()
{
    printf("RESERVATION STATIONS\n");
    for (int i = 0; i < num_stations; i++)
    {
        printf("Station %d\n", i);
        for (int j = 0; j < num_entries_per_RS; j++)
        {
            printf("Valid: %d %d %d 0x%x 0x%x %d ID: %d", stations[i].entries[j].entry_valid, stations[i].entries[j].op1_ready, stations[i].entries[j].op2_ready, stations[i].entries[j].op1_combined_val, stations[i].entries[j].op2_combined_val, stations[i].entries[j].store_tag, stations[i].entries[j].instruction_ID);
            if (j != num_entries_per_RS - 1)
            {
                printf("| ");
            }
        }
        printf("\n");
    }
}

void lsqueue_printer(){
    printf("LOAD QUEUE\n");
    for(int i = 0; i < max_lq_size;i++){
        printf("EV %d AV %d AT %d A 0x%x \n", lq.entries[i].entry_valid, lq.entries[i].address_valid, lq.entries[i].address_tag, lq.entries[i].address);
        printf("O %d V %d T %d D:\n", lq.entries[i].old_bits, lq.entries[i].valid, lq.entries[i].tag);
        for(int j=0;j<bytes_on_data_bus;j++){
            printf("0x%x ", lq.entries[i].data[j]);
        }
        printf("\n");
    }
    printf("STORE QUEUE\n");
    for(int i = 0; i < max_sq_size;i++){
        printf("EV %d AV %d AT %d A 0x%x \n", sq.entries[i].entry_valid, sq.entries[i].address_valid, sq.entries[i].address_tag, sq.entries[i].address);
        printf("MA %x O %d W %d V %d T %d D:\n", sq.entries[i].matchAddress, sq.entries[i].old_bits, sq.entries[i].written, sq.entries[i].valid, sq.entries[i].tag);
        for(int j=0;j<bytes_on_data_bus;j++){
            printf("0x%x ", sq.entries[i].data[j]);
        }
        printf("\n");
    }
}

void rob_printer()
{
    printf("REORDER BUFFER\n");
    for (int i = 0; i < rob_size; i++)
    {
        printf("V:%d O:%d R:%d E:%d Val: 0x%x T:%d S:%d ST:%d ID:%d",
               rob.entries[i].valid, rob.entries[i].old_bits, rob.entries[i].retired,
               rob.entries[i].executed, rob.entries[i].value, rob.entries[i].store_tag,
               rob.entries[i].speculative, rob.entries[i].speculation_tag, rob.entries[i].instruction_ID);
        printf("\n");
    }
}

void bus_arbiter()
{
    // approach: probe metadata bus and do casework

    if (metadata_bus.burst_counter == 3 && metadata_bus.is_serializer_sending_data == TRUE)
    {
        metadata_bus.is_serializer_sending_data = FALSE;
        metadata_bus.burst_counter = 0;

        serializer.entries[serializer_entry_to_send].valid = 0;
        for (int i = 0; i < num_serializer_entries; i++)
        {
            serializer.entries[i].old_bits--;
        }
        serializer.occupancy--;
    }

    // search mshr for oldest entry
    int start_age = 0;
    int mshr_entry_to_send;
    int found_mshr_entry = FALSE;
    for (int i = 0; i < mshr_size; i++)
    {
        if ((mshr.entries[i].valid == TRUE) && (mshr.entries[i].old_bits == start_age))
        {
            if (metadata_bus.bank_status[get_bank_bits(mshr.entries[i].address)] == 0)
            {
                // probing bank status on metadata bus to see if that bank is available
                mshr_entry_to_send = i;
                found_mshr_entry = TRUE;
                break;
            }
            else
            {
                // if it isn't available check the next oldest mshr entry
                start_age++;
                i = -1;
            }
        }
    }
    int found_serializer_entry = FALSE;
    int is_data_bus_active = metadata_bus.is_serializer_sending_data || metadata_bus.receive_enable;
    if (is_data_bus_active == FALSE)
    {
        for (int i = 0; i < num_serializer_entries; i++)
        {
            if ((serializer.entries[i].valid == TRUE) && (serializer.entries[i].old_bits == 0) && (serializer.entries[i].sending_data != TRUE))
            {
                serializer_entry_to_send = i;
                found_serializer_entry = TRUE;
                break;
            }
        }
    }
    // considerations already accounted for: busy banks
    // considerations accounted for here: full deserializers
    // active when serializer has stuff it can send; initiates bursts
    if (found_serializer_entry == TRUE && metadata_bus.deserializer_full == FALSE)
    {
        metadata_bus.serializer_address = serializer.entries[serializer_entry_to_send].address;
        metadata_bus.burst_counter = 0;
        metadata_bus.is_serializer_sending_data = TRUE;

        data_bus.byte_wires[0] = serializer.entries[serializer_entry_to_send].data[0];
        data_bus.byte_wires[1] = serializer.entries[serializer_entry_to_send].data[1];
        data_bus.byte_wires[2] = serializer.entries[serializer_entry_to_send].data[2];
        data_bus.byte_wires[3] = serializer.entries[serializer_entry_to_send].data[3];
        return;
    }
    // active when MSHR has stuff it can send
    if (found_mshr_entry == TRUE)
    {
        metadata_bus.mshr_address = mshr.entries[mshr_entry_to_send].address;
        metadata_bus.is_mshr_sending_addr = TRUE;
        // destination stuff
        metadata_bus.origin = mshr.entries[mshr_entry_to_send].origin;
        metadata_bus.to_mem_request_ID = mshr.entries[mshr_entry_to_send].request_ID;
        metadata_bus.to_mem_request_ID = mshr.entries[mshr_entry_to_send].request_ID;

        mshr.entries[mshr_entry_to_send].valid = FALSE;
        for (int i = 0; i < mshr_size; i++)
        {
            mshr.entries[i].old_bits--;
        }
        mshr.occupancy--;
    }
    // keep in mind that both the serializer and mshr can send stuff in the same cycle

    // account for serializer subsequent bursts
    if (metadata_bus.is_serializer_sending_data == TRUE)
    {
        metadata_bus.burst_counter = metadata_bus.next_burst_counter;

        data_bus.byte_wires[0] = serializer.entries[serializer_entry_to_send].data[metadata_bus.burst_counter * 4 + 0];
        data_bus.byte_wires[1] = serializer.entries[serializer_entry_to_send].data[metadata_bus.burst_counter * 4 + 1];
        data_bus.byte_wires[2] = serializer.entries[serializer_entry_to_send].data[metadata_bus.burst_counter * 4 + 2];
        data_bus.byte_wires[3] = serializer.entries[serializer_entry_to_send].data[metadata_bus.burst_counter * 4 + 3];
    }
}

// architectural registers for the memory controller to keep whats happening happening
int addresses_to_banks[banks_in_DRAM];
int reqID_to_bank[banks_in_DRAM];
int addr_to_bank_cycle[banks_in_DRAM];
int entry_to_bank[banks_in_DRAM];
#define cycles_to_access_bank 5

void bus_printer()
{
    printf("BUS PRINTER\n");
    printf("DATA BUS\n");
    printf("Data: 0x");
    for (int i = 3; i >= 0; i--)
    {
        printf("%x", data_bus.byte_wires[i]);
    }
    printf("\n");
    printf("META BUS\n");
    printf("MSHR address: 0x%x\n", metadata_bus.mshr_address);
    printf("Serializer address: 0x%x\n", metadata_bus.serializer_address);
    printf("Store address: 0x%x\n", metadata_bus.store_address);
    printf("Is MSHR sending address: %d\n", metadata_bus.is_mshr_sending_addr);
    printf("To mem request ID: %d\n", metadata_bus.to_mem_request_ID);
    printf("To CPU request ID: %d\n", metadata_bus.to_cpu_request_ID);
    printf("From mem request ID: %d\n", metadata_bus.from_mem_request_ID);
    printf("FROM CPU request ID: %d\n", metadata_bus.from_cpu_request_ID);
    printf("Is serializer sending address: %d\n", metadata_bus.is_serializer_sending_data);
    printf("Burst counter: %d\n", metadata_bus.burst_counter);
    printf("Next burst counter: %d\n", metadata_bus.next_burst_counter);
    printf("Receive enable: %d\n", metadata_bus.receive_enable);
    printf("Revival in progress: %d\n", metadata_bus.revival_in_progress);
    printf("Current revival entry: %d\n", metadata_bus.current_revival_entry);
    printf("Bus origin: %d\n", metadata_bus.origin);
    printf("Bus destination: %d\n", metadata_bus.destination);
    printf("Deserializer full: %d\n", metadata_bus.deserializer_full);
    printf("Deserializer can store: %d\n", metadata_bus.deserializer_can_store);
    printf("Deserializer bank 0 target: %d\n", metadata_bus.deserializer_bank_targets[0]);
    printf("Deserializer bank 1 target: %d\n", metadata_bus.deserializer_bank_targets[1]);
    printf("Deserializer bank 2 target: %d\n", metadata_bus.deserializer_bank_targets[2]);
    printf("Deserializer bank 3 target: %d\n", metadata_bus.deserializer_bank_targets[3]);
    printf("Deserializer target: %d\n", metadata_bus.deserializer_target);
    printf("Deserializer next entry: %d\n", metadata_bus.deserializer_next_entry);
    printf("Serializer available: %d\n", metadata_bus.serializer_available);
    printf("Bank 0 Status: %d\n", metadata_bus.bank_status[0]);
    printf("Bank 1 Status: %d\n", metadata_bus.bank_status[1]);
    printf("Bank 2 Status: %d\n", metadata_bus.bank_status[2]);
    printf("Bank 3 Status: %d\n", metadata_bus.bank_status[3]);
    printf("Bank 0 Destinations: %d\n", metadata_bus.bank_destinations[0]);
    printf("Bank 1 Destinations: %d\n", metadata_bus.bank_destinations[1]);
    printf("Bank 2 Destinations: %d\n", metadata_bus.bank_destinations[2]);
    printf("Bank 3 Destinations: %d\n", metadata_bus.bank_destinations[3]);
    printf("BANK INFORMATION\n");
    printf("Bank 0 Address: %d\n", addresses_to_banks[0]);
    printf("Bank 1 Address: %d\n", addresses_to_banks[1]);
    printf("Bank 2 Address: %d\n", addresses_to_banks[2]);
    printf("Bank 3 Address: %d\n", addresses_to_banks[3]);
    printf("Bank 0 Cycle: %d\n", addr_to_bank_cycle[0]);
    printf("Bank 1 Cycle: %d\n", addr_to_bank_cycle[1]);
    printf("Bank 2 Cycle: %d\n", addr_to_bank_cycle[2]);
    printf("Bank 3 Cycle: %d\n", addr_to_bank_cycle[3]);
}

void memory_controller()
{
    // will need to probe the metadatabus to see the current state to see what itll need to do this cycle

    // if serializer is sending data, call the deserializer inserter
    if (metadata_bus.is_serializer_sending_data == TRUE)
    {
        deserializer_inserter();
    }

    // handle incoming read requests
    if (metadata_bus.is_mshr_sending_addr == TRUE)
    {
        int bkbits = get_bank_bits(metadata_bus.mshr_address);
        addresses_to_banks[bkbits] = metadata_bus.mshr_address;
        addr_to_bank_cycle[bkbits] = 0;
        reqID_to_bank[bkbits] = metadata_bus.to_mem_request_ID;
        metadata_bus.bank_status[bkbits] = 3;
        metadata_bus.bank_destinations[bkbits] = metadata_bus.origin;
    }

    // check deserializer to start any stores if possible
    if (metadata_bus.deserializer_can_store == TRUE &&
        deserializer.entries[metadata_bus.deserializer_next_entry].receiving_data_from_data_bus == FALSE &&
        deserializer.entries[metadata_bus.deserializer_next_entry].writing_data_to_DRAM == FALSE && deserializer.entries[metadata_bus.deserializer_next_entry].valid == TRUE)
    {
        int bkbits = get_bank_bits(deserializer.entries[metadata_bus.deserializer_next_entry].address);
        deserializer.entries[metadata_bus.deserializer_next_entry].writing_data_to_DRAM = TRUE;
        metadata_bus.bank_status[bkbits] = 2;
        addr_to_bank_cycle[bkbits] = 0;
        entry_to_bank[bkbits] = metadata_bus.deserializer_next_entry;
        deserializer.entries[metadata_bus.deserializer_next_entry].old_bits = -1; // this causes a new next entry to be selected so a not busy bank can be stored into if need be
    }

    // iterate through the banks
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        // handle in progress read requests
        if (metadata_bus.bank_status[i] == 1)
        {
            if (addr_to_bank_cycle[i] == cycles_to_access_bank)
            {
                // initiate load send to cpu
                if ((metadata_bus.is_serializer_sending_data == FALSE) && (metadata_bus.serializer_available == TRUE) && (metadata_bus.receive_enable == FALSE))
                {
                    metadata_bus.receive_enable = TRUE;
                    metadata_bus.burst_counter = 0;
                    metadata_bus.next_burst_counter = 0;
                    metadata_bus.store_address = addresses_to_banks[i];
                    metadata_bus.to_cpu_request_ID = reqID_to_bank[i];
                    metadata_bus.destination = metadata_bus.bank_destinations[i];

                    data_bus.byte_wires[0] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[0];
                    data_bus.byte_wires[1] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[1];
                    data_bus.byte_wires[2] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[2];
                    data_bus.byte_wires[3] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[3];

                    addr_to_bank_cycle[i]++;
                }
                else
                {
                    continue; // wait until you can send stuff back
                }
            } // continue load send to cpu
            else if (addr_to_bank_cycle[i] > cycles_to_access_bank)
            {
                metadata_bus.burst_counter = metadata_bus.next_burst_counter;

                data_bus.byte_wires[0] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 0];
                data_bus.byte_wires[1] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 1];
                data_bus.byte_wires[2] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 2];
                data_bus.byte_wires[3] = dram.banks[get_bank_bits(addresses_to_banks[i])].rows[get_row_bits(addresses_to_banks[i])].columns[get_column_bits(addresses_to_banks[i])].bytes[metadata_bus.burst_counter * 4 + 3];
                if (metadata_bus.next_burst_counter == 4)
                {
                    // end it
                    metadata_bus.bank_status[i] = 0;
                    metadata_bus.burst_counter = 0;
                    metadata_bus.receive_enable = FALSE;
                    continue;
                }
            }
            else
            {
                addr_to_bank_cycle[i]++;
            }
        }
        else if (metadata_bus.bank_status[i] == 2)
        { // handle in progress store requests, assume it takes 5 cycles to open the row and then just paste the data in it
            if (addr_to_bank_cycle[i] == cycles_to_access_bank)
            {
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[0] = deserializer.entries[entry_to_bank[i]].data[0];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[1] = deserializer.entries[entry_to_bank[i]].data[1];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[2] = deserializer.entries[entry_to_bank[i]].data[2];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address)].bytes[3] = deserializer.entries[entry_to_bank[i]].data[3];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[0] = deserializer.entries[entry_to_bank[i]].data[4];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[1] = deserializer.entries[entry_to_bank[i]].data[5];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[2] = deserializer.entries[entry_to_bank[i]].data[6];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 1].bytes[3] = deserializer.entries[entry_to_bank[i]].data[7];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[0] = deserializer.entries[entry_to_bank[i]].data[8];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[1] = deserializer.entries[entry_to_bank[i]].data[9];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[2] = deserializer.entries[entry_to_bank[i]].data[10];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 2].bytes[3] = deserializer.entries[entry_to_bank[i]].data[11];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[0] = deserializer.entries[entry_to_bank[i]].data[12];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[1] = deserializer.entries[entry_to_bank[i]].data[13];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[2] = deserializer.entries[entry_to_bank[i]].data[14];
                dram.banks[i].rows[get_row_bits(deserializer.entries[entry_to_bank[i]].address)].columns[get_column_bits(deserializer.entries[entry_to_bank[i]].address) + 3].bytes[3] = deserializer.entries[entry_to_bank[i]].data[15];

                metadata_bus.bank_status[i] = 0;
                deserializer.entries[entry_to_bank[i]].valid = FALSE;
            }
            else
            {
                addr_to_bank_cycle[i]++;
            }
        }
        else if (metadata_bus.bank_status[i] == 3)
        {
            // attempt revival by searching through deserializer for an address match
            // if revival succeeds, modify deserializer entry such that it'll send the data through the data bus
            // if revival fails, change status to 1
            bool revival_success = false;
            for (int j = 0; j < num_deserializer_entries; j++)
            {
                if (deserializer.entries[j].valid == TRUE && (deserializer.entries[j].address == addresses_to_banks[i]))
                {
                    deserializer.entries[j].revival_destination = metadata_bus.bank_destinations[i];
                    deserializer.entries[j].revival_lock = 1;
                    deserializer.entries[j].revival_reqID = reqID_to_bank[i];
                    metadata_bus.bank_status[i] = 0;
                    revival_success = true;
                    break;
                }
            }
            if (revival_success == false)
            {
                metadata_bus.bank_status[i] = 1;
            }
        }
    }

    // stage revivals here, only one revival can be staged in a singular cycle and they take 4 cycles to burst
    for (int i = 0; i < num_deserializer_entries; i++)
    {
        if ((metadata_bus.revival_in_progress == FALSE) &&
            (deserializer.entries[i].revival_lock == TRUE) &&
            (metadata_bus.is_serializer_sending_data == FALSE) &&
            (metadata_bus.serializer_available == TRUE) &&
            (metadata_bus.receive_enable == FALSE))
        {
            metadata_bus.revival_in_progress = TRUE;
            metadata_bus.receive_enable = TRUE;
            metadata_bus.current_revival_entry = i;
            metadata_bus.burst_counter = 0;
            metadata_bus.destination = deserializer.entries[metadata_bus.current_revival_entry].revival_destination;
            metadata_bus.to_cpu_request_ID = deserializer.entries[metadata_bus.current_revival_entry].revival_reqID;

            metadata_bus.store_address = deserializer.entries[metadata_bus.current_revival_entry].address;
            data_bus.byte_wires[0] = deserializer.entries[metadata_bus.current_revival_entry].data[0];
            data_bus.byte_wires[1] = deserializer.entries[metadata_bus.current_revival_entry].data[1];
            data_bus.byte_wires[2] = deserializer.entries[metadata_bus.current_revival_entry].data[2];
            data_bus.byte_wires[3] = deserializer.entries[metadata_bus.current_revival_entry].data[3];

            return;
        }
        else if (metadata_bus.revival_in_progress == TRUE)
        {

            metadata_bus.burst_counter = metadata_bus.next_burst_counter;

            data_bus.byte_wires[0] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 0];
            data_bus.byte_wires[1] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 1];
            data_bus.byte_wires[2] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 2];
            data_bus.byte_wires[3] = deserializer.entries[metadata_bus.current_revival_entry].data[metadata_bus.burst_counter * 4 + 3];

            if (metadata_bus.burst_counter == 3)
            {
                metadata_bus.burst_counter = 0;
                metadata_bus.receive_enable = FALSE;
                metadata_bus.revival_in_progress = FALSE;
                deserializer.entries[metadata_bus.current_revival_entry].revival_lock = 0;
                continue;
            }
        }
    }
}

// called upon eviction from cache/tlb
void serializer_inserter(int address, int data[cache_line_size])
{
    for (int i = 0; i < num_serializer_entries; i++)
    {
        if (serializer.entries[i].valid == FALSE)
        {
            serializer.entries[i].valid = TRUE;
            serializer.entries[i].address = address;
            for (int j = 0; j < cache_line_size; j++)
            {
                serializer.entries[i].data[j] = data[j];
            }
            serializer.entries[i].old_bits = serializer.occupancy;
            break;
        }
        else
        {
            continue;
        }
    }

    serializer.occupancy++;

    if (serializer.occupancy < max_serializer_occupancy)
    {
        metadata_bus.serializer_available = 1;
    }
    else
    {
        metadata_bus.serializer_available = 0;
    }
}

// called when the serializer is sending data
void deserializer_inserter()
{
    if (metadata_bus.burst_counter == 0)
    {
        deserializer.entries[metadata_bus.deserializer_target].address = metadata_bus.serializer_address;
        deserializer.entries[metadata_bus.deserializer_target].receiving_data_from_data_bus = TRUE;
    }
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 0] = data_bus.byte_wires[0];
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 1] = data_bus.byte_wires[1];
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 2] = data_bus.byte_wires[2];
    deserializer.entries[metadata_bus.deserializer_target].data[metadata_bus.burst_counter * 4 + 3] = data_bus.byte_wires[3];
    metadata_bus.next_burst_counter = metadata_bus.burst_counter++;
    if (metadata_bus.burst_counter == 3)
    {
        deserializer.entries[metadata_bus.deserializer_target].old_bits = deserializer.occupancy;
        deserializer.occupancy++;
        deserializer.entries[metadata_bus.deserializer_target].valid = 1;
        deserializer.entries[metadata_bus.deserializer_target].receiving_data_from_data_bus = FALSE;
    }
}

// needs to be called every cycle, will set availability of deserializer on metadata bus
void deserializer_handler()
{
    // iterate through deserializers and find the first available one, use that as the target
    // also indicate which banks the deserializer wants to store into
    // also indicate which deserializer entry will store next
    bool found_deserializer_target = false;
    int temp_deserializer_bank_targets[banks_in_DRAM];
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        temp_deserializer_bank_targets[i] = 0;
    }
    for (int i = 0; i < max_deserializer_occupancy; i++)
    {
        if (deserializer.entries[i].valid == FALSE && deserializer.entries[i].revival_lock == FALSE)
        {
            // target selection
            metadata_bus.deserializer_target = i;
            metadata_bus.deserializer_full = 0;
            found_deserializer_target = true;
            // which banks are targetted for stores
            temp_deserializer_bank_targets[get_bank_bits(deserializer.entries[i].address)] = 1;
        }
    }
    int search_age = 0;
    for (int i = 0; i < max_deserializer_occupancy; i++)
    {
        if (deserializer.entries[i].valid == TRUE)
        {
            // next deserializer entry that gets to store
            if (deserializer.entries[i].old_bits == search_age)
            {
                if (metadata_bus.bank_status[get_bank_bits(deserializer.entries[i].address)] == 0)
                {
                    metadata_bus.deserializer_next_entry = i;
                    metadata_bus.deserializer_can_store = 1;
                    break;
                }
                else if (search_age == num_deserializer_entries)
                {
                    metadata_bus.deserializer_can_store = 0;
                    break;
                }
                else
                {
                    search_age++;
                    i = -1;
                }
            }
        }
    }
    for (int i = 0; i < banks_in_DRAM; i++)
    {
        metadata_bus.deserializer_bank_targets[i] = temp_deserializer_bank_targets[i];
    }
    if (!found_deserializer_target)
    {
        metadata_bus.deserializer_full = 1;
    }
}

void rescue_stager()
{
    // if the destination is available for a write, stage the rescue
    // for this, a burst isn't needed
    // assume only one rescue allowed per cycle for now?
    bool success = false;
    for (int i = 0; i < num_serializer_entries; i++)
    {
        if (serializer.entries[i].rescue_lock == TRUE)
        {
            if (serializer.entries[i].rescue_destination == 0)
            {
                if (icache.bank_status[get_bank_bits(serializer.entries[i].address)] == 0)
                {
                    icache.bank_status[get_bank_bits(serializer.entries[i].address)] = 1;
                    success = icache_paste(serializer.entries[i].address, serializer.entries[i].data); // for now we'll assume that this is fast
                    if (success)
                        serializer.entries[i].rescue_lock == FALSE;
                    return;
                }
            }
            else if (serializer.entries[i].rescue_destination == 1)
            {
                if (dcache.bank_status[get_bank_bits(serializer.entries[i].address)] == 0)
                {
                    dcache.bank_status[get_bank_bits(serializer.entries[i].address)] = 1;
                    success = dcache_paste(serializer.entries[i].address, serializer.entries[i].data); // for now we'll assume that this is fast
                    if (success)
                        serializer.entries[i].rescue_lock == FALSE;
                    return;
                }
            }
        }
    }
}
