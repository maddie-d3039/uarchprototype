// todo: 
/*
RAT
FAT
MAT
ALIAS POOLS
LOAD/STORE QUEUE
BRANCH PREDICTOR
SPECULATIVE EXECUTED BUFFER
Remove read write from mshr and mshr handling functions and pre mshr entries
ROB needs to account for speculatively executed so that wrong entries get cleared properly
*/





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BP.cpp"
#include <cassert>

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
void deserializer_handler();

#define control_store_rows 20 //arbitrary
#define history_length 8;
#define table_length 16;
#define TRUE  1
#define FALSE 0

enum CS_BITS {
  operation,
  updated_flags,
  op1_addr_mode,
  op2_addr_mode,
  is_control_instruction,
  control_instruction_type,
  is_op1_needed,
  is_op2_needed,
  num_control_store_bits
} CS_BITS;

int CONTROL_STORE[control_store_rows][num_control_store_bits];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/

#define WORDS_IN_MEM    0x01000
//simple memory array but its obsolete, use the dram struct
int MEMORY[WORDS_IN_MEM][4];

//address mapping
// RRR RRRR CCBkBk CCBoBBoB
//actual structures
#define byes_per_column 4
#define columns_per_row 16
#define rows_per_bank 128
#define banks_in_DRAM 4
//a column has 4 bytes
typedef struct Column_Struct{
    int bytes[byes_per_column];
} Column;
typedef struct Row_Struct{
    Column columns[columns_per_row];
} Row;
typedef struct Bank_Struct{
    Row rows[rows_per_bank];
} Bank;
typedef struct DRAM_Struct{
    Bank banks[banks_in_DRAM];
} DRAM;
DRAM dram;

//mask helper functions

int get_row_bits(int addr){
    return (addr>>8)&0x7F;
}

int get_bank_bits(int addr){
    return (addr>>4)&0x3;
}

int get_column_bits(int addr){
    return ((addr>>6)&(0x3)<<2) + (addr>>2)&(0x3);
}


//BUS

//make data bus of 32 bits
//make metadata bus capable of holding handshake and activation signals, address, etc. Figure out all that needs to be in it

#define bytes_on_data_bus 32
typedef struct Data_Bus_Struct{
    int byte_wires[bytes_on_data_bus];
} Data_Bus;

Data_Bus data_bus;

typedef struct Metadata_Bus_Struct{
    int mshr_address;
    int serializer_address;
    int is_mshr_sending_addr;
    int is_serializer_sending_data;
    int is_serializer_sending_address;
    int burst_counter;
    int receive_enable;
    int destination;
    int bank_status[banks_in_DRAM]; //0 available, 1 performing load, 2 performing store
    int bank_destinations[banks_in_DRAM];
}Metadata_Bus;

Metadata_Bus metadata_bus;

//architectural registers

int EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP;
int EIP, EFLAGS;
int oldEIP;
int tempEIP; //used for branch resteering
int tempOffset;
//virtual memory specifications
int SBR = 0x500;
#define pte_size 4


// Number of entries per pool
#define REGISTER_ALIAS_POOL_ENTRIES 64
#define FLAG_ALIAS_POOL_ENTRIES     64
#define MEMORY_ALIAS_POOL_ENTRIES   64

// Base offsets for each alias range
#define REGISTER_ALIAS_BASE  0
#define FLAG_ALIAS_BASE      (REGISTER_ALIAS_BASE + REGISTER_ALIAS_POOL_ENTRIES)  // 64
#define MEMORY_ALIAS_BASE    (FLAG_ALIAS_BASE     + FLAG_ALIAS_POOL_ENTRIES)      // 128

// --------------------------------------------------------------
// Struct for Register Alias Pool (global IDs 0..63)
// --------------------------------------------------------------
struct RegisterAliasPool {
    int  aliases[REGISTER_ALIAS_POOL_ENTRIES];
    bool valid[REGISTER_ALIAS_POOL_ENTRIES];

    RegisterAliasPool() {
        // Initialize each slot so aliases[i] = (base + i), and mark all as unused
        for (int i = 0; i < REGISTER_ALIAS_POOL_ENTRIES; ++i) {
            aliases[i] = REGISTER_ALIAS_BASE + i;  
            valid[i]   = false;
        }
    }

    // Returns one free alias in [0..63], or -1 if none left
    int get() {
        for (int i = 0; i < REGISTER_ALIAS_POOL_ENTRIES; ++i) {
            if (!valid[i]) {
                valid[i] = true;
                return aliases[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [0..63] or wasn't actually allocated, an assert fires.
    void free(int alias) {
        // Compute local index
        int index = alias - REGISTER_ALIAS_BASE;

        // 1) Assert that alias is within this pool's global range
        assert(index >= 0 && index < REGISTER_ALIAS_POOL_ENTRIES
               && "RegisterAliasPool::free(): alias out of range!");

        // 2) Assert that this slot was previously allocated (valid == true)
        assert(valid[index] && "RegisterAliasPool::free(): alias was not allocated!");

        // Finally, mark it free again
        valid[index] = false;
    }
};

// --------------------------------------------------------------
// Struct for Flag Alias Pool (global IDs 64..127)
// --------------------------------------------------------------
struct FlagAliasPool {
    int  aliases[FLAG_ALIAS_POOL_ENTRIES];
    bool valid[FLAG_ALIAS_POOL_ENTRIES];

    FlagAliasPool() {
        for (int i = 0; i < FLAG_ALIAS_POOL_ENTRIES; ++i) {
            aliases[i] = FLAG_ALIAS_BASE + i;  // 64 + i
            valid[i]   = false;
        }
    }

    // Returns one free alias in [64..127], or -1 if none left
    int get() {
        for (int i = 0; i < FLAG_ALIAS_POOL_ENTRIES; ++i) {
            if (!valid[i]) {
                valid[i] = true;
                return aliases[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [64..127] or wasn't allocated, an assert fires.
    void free(int alias) {
        int index = alias - FLAG_ALIAS_BASE;

        assert(index >= 0 && index < FLAG_ALIAS_POOL_ENTRIES
               && "FlagAliasPool::free(): alias out of range!");
        assert(valid[index] && "FlagAliasPool::free(): alias was not allocated!");

        valid[index] = false;
    }
};

// --------------------------------------------------------------
// Struct for Memory Alias Pool (global IDs 128..191)
// --------------------------------------------------------------
struct MemoryAliasPool {
    int  aliases[MEMORY_ALIAS_POOL_ENTRIES];
    bool valid[MEMORY_ALIAS_POOL_ENTRIES];

    MemoryAliasPool() {
        for (int i = 0; i < MEMORY_ALIAS_POOL_ENTRIES; ++i) {
            aliases[i] = MEMORY_ALIAS_BASE + i;  // 128 + i
            valid[i]   = false;
        }
    }

    // Returns one free alias in [128..191], or -1 if none left
    int get() {
        for (int i = 0; i < MEMORY_ALIAS_POOL_ENTRIES; ++i) {
            if (!valid[i]) {
                valid[i] = true;
                return aliases[i];
            }
        }
        return -1;
    }

    // Frees a previously allocated alias.
    // If `alias` is out of [128..191] or wasn't allocated, an assert fires.
    void free(int alias) {
        int index = alias - MEMORY_ALIAS_BASE;

        assert(index >= 0 && index < MEMORY_ALIAS_POOL_ENTRIES
               && "MemoryAliasPool::free(): alias out of range!");
        assert(valid[index] && "MemoryAliasPool::free(): alias was not allocated!");

        valid[index] = false;
    }
};



#define ibuffer_size 4
#define cache_line_size 16
int ibuffer[ibuffer_size][cache_line_size];
int ibuffer_valid[ibuffer_size];

typedef struct PipeState_Entry_Struct{
  int predecode_valid,predecode_ibuffer[ibuffer_size][cache_line_size], predecode_EIP,
      predecode_offset, predecode_current_sector, predecode_line_offset,
      decode_valid, decode_instruction_length, decode_EIP, decode_immSize,
      agbr_valid, agbr_cs[num_control_store_bits], agbr_NEIP,
      agbr_op1_base, agbr_op1_index, agbr_op1_scale, agbr_op1_disp,
      agbr_op2_base, agbr_op2_index, agbr_op2_scale, agbr_op2_disp, agbr_offset,
      rr_valid, rr_operation, rr_updated_flags, 
      rr_op1_base, rr_op1_index, rr_op1_scale, rr_op1_disp, rr_op1_addr_mode,
      rr_op2_base, rr_op2_index, rr_op2_scale, rr_op2_disp, rr_op2_addr_mode;

    char decode_instruction_register[15];
    char agbr_instruction_register[15];
} PipeState_Entry;

PipeState_Entry pipeline, new_pipeline;
BP* branch_predictor = new BP();
int cycle_count;

int RUN_BIT;

void cycle(){ 
    new_pipeline = pipeline;
    //stages
    writeback_stage();
    memory_stage();
    execute_stage();
    register_rename_stage();
    addgen_branch_stage();
    decode_stage();
    predecode_stage();
    fetch_stage();
    //memory structures
    memory_controller();
    bus_arbiter();
    mshr_inserter();
    pipeline=new_pipeline;
    cycle_count++;
}

void run(int num_cycles) {
    int i;
    
    if (RUN_BIT == FALSE) {
      printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (EIP == 0x0000) {
	    cycle();
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

void go() {
    if ((RUN_BIT == FALSE) || (EIP == 0x0000)) {
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

void mdump(FILE * dumpsim_file, int start, int stop) {
    int address; /* this is a byte address */

    printf("\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%04x..0x%04x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%04x (%d) : 0x%02x%02x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

void rdump(FILE * dumpsim_file) {
    int k; 

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count : %d\n", cycle_count);
    printf("PC          : 0x%04x\n", EIP);
    printf("Flags: %d\n", EFLAGS);
    printf("Registers:\n");
	printf("EAX: %d EBX: %d ECX: %d EDX: %d ESI: %d EDI: %d EBP: %d ESP: %d",EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent architectural state :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count : %d\n", cycle_count);
    fprintf(dumpsim_file, "PC          : 0x%04x\n", EIP);
    fprintf(dumpsim_file, "Flags: %d\n", EFLAGS);
    fprintf(dumpsim_file, "Registers:\n");
	fprintf(dumpsim_file, "EAX: %d EBX: %d ECX: %d EDX: %d ESI: %d EDI: %d EBP: %d ESP: %d",EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

void idump(FILE * dumpsim_file) {
    int k; 

    printf("\nCurrent architectural state :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count     : %d\n", cycle_count);
    printf("PC              : 0x%04x\n", EIP);
    printf("Flags: %d\n", EFLAGS);
    printf("Registers:\n");
	printf("EAX: %d EBX: %d ECX: %d EDX: %d ESI: %d EDI: %d EBP: %d ESP: %d",EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP);
    printf("\n");

    printf("------------- PREDECODE   Latches --------------\n");
    printf("PD V          :  0x%04x\n", pipeline.predecode_valid );
    printf("\n");
    printf("PD EIP        :  %d\n", pipeline.predecode_EIP);
    printf("\n");
    
    printf("------------- DECODE Latches --------------\n");
    printf("DC V        :  0x%04x\n", pipeline.decode_valid );
    printf("\n");
    // printf("DC IR       :  0x%04x\n", pipeline.decode_instruction_register );
    printf("DC IL       :  0x%04x\n", pipeline.decode_instruction_length );
    printf("DC EIP      :  %d\n", pipeline.decode_EIP);
    printf("\n");

    printf("------------- AGEN_BR  Latches --------------\n");
    printf("AGBR V         :  0x%04x\n", pipeline.agbr_valid );
    printf("\n");
    printf("AGBR NEIP  :  0x%04x\n", pipeline.agbr_NEIP );
    printf("AGBR OP1 B     :  0x%04x\n", pipeline.agbr_op1_base ); 
    printf("AGBR OP1 I          :  %d\n", pipeline.agbr_op1_index );
    printf("AGBR OP1 S          :  0x%04x\n", pipeline.agbr_op1_scale );
    printf("AGBR OP1 D        :  %d\n", pipeline.agbr_op1_disp);
    printf("AGBR OP2 B     :  0x%04x\n", pipeline.agbr_op2_base ); 
    printf("AGBR OP2 I          :  %d\n", pipeline.agbr_op2_index );
    printf("AGBR OP2 S          :  0x%04x\n", pipeline.agbr_op2_scale );
    printf("AGBR OP2 D        :  %d\n", pipeline.agbr_op2_disp);
    printf("AGBR_CS          :  ");
    for ( k = 0 ; k < num_control_store_bits; k++) {
      printf("%d",pipeline.agbr_cs[k]);
    }

    printf("------------- REG RENAME   Latches --------------\n");
    printf("RR V          :  0x%04x\n", pipeline.rr_valid );
    printf("\n");
    printf("RR OP         :  0x%04x\n", pipeline.rr_operation );
    printf("RR UF   :  0x%04x\n", pipeline.rr_updated_flags );
    printf("RR OP1 B      :  0x%04x\n", pipeline.rr_op1_base );
    printf("RR OP1 B      :  0x%04x\n", pipeline.rr_op1_index );
    printf("RR OP1 B      :  0x%04x\n", pipeline.rr_op1_scale );
    printf("RR OP1 B      :  0x%04x\n", pipeline.rr_op1_disp );
    printf("RR OP1 B      :  0x%04x\n", pipeline.rr_op1_addr_mode );
    printf("RR OP2 B      :  0x%04x\n", pipeline.rr_op2_base );
    printf("RR OP2 B      :  0x%04x\n", pipeline.rr_op2_index );
    printf("RR OP2 B      :  0x%04x\n", pipeline.rr_op2_scale );
    printf("RR OP2 B      :  0x%04x\n", pipeline.rr_op2_disp );
    printf("RR OP2 B      :  0x%04x\n", pipeline.rr_op2_addr_mode );
    
    printf("\n");
}

void init_control_store(char *ucode_filename) {
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < control_store_rows; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < num_control_store_bits; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}
	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

void init_memory() {
    int i;
    
     for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

void init_state() {
  memset(&pipeline, 0 ,sizeof(PipeState_Entry)); 
  memset(&new_pipeline, 0 , sizeof(PipeState_Entry));
}

void get_command(FILE * dumpsim_file) {
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
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

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
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

void load_program(char *program_filename) {
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1 ;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}
	
	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (EIP == 0) EIP  = program_base << 1 ;
    printf("Read %d words from program into memory.\n\n", ii);
}

void initialize(char *ucode_filename, char *program_filename, int num_prog_files) {
    int i;
    init_control_store(ucode_filename);

    init_memory();

    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(program_filename);
	while(*program_filename++ != '\0');
    }
    init_state();
    
    oldEIP=0;

    RUN_BIT = TRUE;
}

int main(int argc, char *argv[]) {
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("x86 Simulator\n\n");

    initialize(argv[1], argv[2], argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);
}

//other structures

//DCACHE
//address mapping: TTT TTT TTI IBO OOO
#define dcache_banks 2
#define dcache_sets 4
#define dcache_ways 4
typedef struct D$_TagStoreEntry_Struct{
    int valid_way0, tag_way0, dirty_way0,
        valid_way1, tag_way1, dirty_way1,
        valid_way2, tag_way2, dirty_way2,
        valid_way3, tag_way3, dirty_way3,
        lru; 
} D$_TagStoreEntry;
typedef struct D$_TagStore_Struct{
    D$_TagStoreEntry dcache_tagstore[dcache_banks][dcache_sets];
} D$_TagStore;
typedef struct D$_DataStore_Struct{
    int dcache_datastore[dcache_banks][dcache_sets][dcache_ways][cache_line_size];
} D$_DataStore;
typedef struct D$_Struct{
    D$_DataStore data;
    D$_TagStore tag;
} D$;

D$ dcache;

//ICACHE
//address mapping: TTT TTT TII IBO OOO
#define icache_banks 2
#define icache_sets 8
#define icache_ways 2
typedef struct I$_TagStoreEntry_Struct{
    int valid_way0, tag_way0, dirty_way0,
        valid_way1, tag_way1, dirty_way1,
        lru; 
} I$_TagStoreEntry;
typedef struct I$_TagStore_Struct{
    I$_TagStoreEntry icache_tagstore[icache_banks][icache_sets];
} I$_TagStore;
typedef struct I$_DataStore_Struct{
    int icache_datastore[icache_banks][icache_sets][icache_ways][cache_line_size];
} I$_DataStore;
typedef struct I$_Struct{
    I$_DataStore data;
    I$_TagStore tag;
} I$;

I$ icache;

//tlb
#define tlb_entries 8
typedef struct TLBEntry_Struct{
    int valid, present, permissions, vpn, pfn;
} TLBEntry;
typedef struct TLB_Struct{
    TLBEntry entries[tlb_entries];
} TLB;

TLB tlb;

//lsq

//rat
typedef struct RAT_MetadataEntry_Struct{
    int valid, alias;
} RAT_MetadataEntry;

//fat

//reservation_stations

//mshr
//entries are the actual entries waiting for the data to come back or waiting to send their address to the memory controller
//pre-entries are what just got inserted this cycle and need to be sorted
#define mshr_size 16
#define pre_mshr_size 8
typedef struct MSHR_Entry_Struct{
    int valid, old_bits, origin, address, read_or_write, requested, sending_data, data_to_send[cache_line_size];
} MSHR_Entry;
typedef struct MSHR{
    MSHR_Entry entries[mshr_size];
    MSHR_Entry pre_entries[pre_mshr_size];
    int occupancy;
    int pre_occupancy;
} MSHR;
MSHR mshr;
//btb

//SpecExeTracker

//ROB
#define rob_size 16
typedef struct ROB_Entry_Struct{
  int valid, old_bits, retired, executed, value, store_tag;
} ROB_Entry;
typedef struct ROB_Struct{
    ROB_Entry entries[rob_size];
    int occupancy;
} ROB;
ROB rob;

//functionality

void mshr_preinserter(int address, int origin, int read_or_write){
    //origin 0 is icache
    //origin 1 is dcache
    //origin 2 is tlb
    for(int i =0;i<pre_mshr_size;i++){
        if(mshr.pre_entries[i].valid==FALSE){
            mshr.pre_entries[i].valid=TRUE;
            mshr.pre_entries[i].old_bits=mshr.pre_occupancy;
            mshr.pre_occupancy++;
            mshr.pre_entries[i].origin=origin;
            mshr.pre_entries[i].address=address;
            mshr.pre_entries[i].requested=FALSE;
            mshr.pre_entries[i].sending_data=FALSE;
            mshr.pre_entries[i].read_or_write = read_or_write;
            return;
        }
    }
}

void translate_miss(int vpn){
    int phys_addr = SBR + vpn * pte_size;
    mshr_preinserter(phys_addr, 2, 0);
}


void icache_access(int addr, int *data_way0[cache_line_size],int *data_way1[cache_line_size], I$_TagStoreEntry *tag_metadata){
    int bank = (addr>>3)&0x1;
    int set = (addr>>4)&0x7;
    *data_way0 = icache.data.icache_datastore[bank][set][0];
    *data_way1 = icache.data.icache_datastore[bank][set][1];
    *tag_metadata=icache.tag.icache_tagstore[bank][set];
    return;
}

void dcache_access(int addr, int *data_way0[cache_line_size],int *data_way1[cache_line_size], int *data_way2[cache_line_size],int *data_way3[cache_line_size], D$_TagStoreEntry *tag_metadata){
    int bank = (addr>>3)&0x1;
    int set = (addr>>4)&0x3;
    *data_way0 =dcache.data.dcache_datastore[bank][set][0];
    *data_way1 =dcache.data.dcache_datastore[bank][set][1];
    *data_way2 =dcache.data.dcache_datastore[bank][set][2];
    *data_way3 =dcache.data.dcache_datastore[bank][set][3];
    *tag_metadata =dcache.tag.dcache_tagstore[bank][set];
    return;
}

void tlb_access(int addr, int *physical_tag, int *tlb_hit){
    int incoming_vpn = (addr>>12)&0x7;
    for(int i =0;i<tlb_entries;i++){
        if(tlb.entries[i].valid && tlb.entries[i].present){
            if(incoming_vpn==tlb.entries[i].vpn){
                *tlb_hit=1;
                *physical_tag = tlb.entries[i].pfn;
                return;
            }
        }
    }
    *tlb_hit=0;
    translate_miss(incoming_vpn);
    return;
}

void tlb_write(){
    
}



//stages

int rob_broadcast_value, rob_broadcast_tag;
void writeback_stage(){
    // update to account for resteering/clearing on mispredicts
    for(int i =0;i<rob_size;i++){
        if((rob.entries[i].valid==1) && (rob.entries[i].old_bits==0) && (rob.entries[i].retired!=1) && (rob.entries[i].executed==1)){
            rob_broadcast_value = rob.entries[i].value;
            rob_broadcast_tag = rob.entries[i].store_tag;
            rob.entries[i].valid=0;
            rob.entries[i].retired=1;
            for(int j =0;j<rob_size;j++){
                if((rob.entries[j].valid==TRUE) && (rob.entries[j].retired==FALSE)){
                    rob.entries[j].old_bits--;
                }
            }
            break;
        }
    }
}

void memory_stage(){

}

void execute_stage(){

}

void register_rename_stage(){
    if(pipeline.rr_valid == 1){

    }else{
        return;
    }
}

void addgen_branch_stage(){
if(new_pipeline.agbr_valid && need_prediction){ //add control store bit for this
    bool prediction = branch_predictor->predict();
     tempEIP = EIP; //store this EIP in case need to resteer
     tempOffset = new_pipeline.agen_offset;
    if(prediction){
        EIP = NEIP + new_pipeline.agen_offset; 
    }
    //something to let us know we are spec executing  
}
}

void decode_stage(){

}

#define max_instruction_length 15
int length;
void predecode_stage(){
    if(new_pipeline.predecode_valid){
        new_pipeline.decode_immSize = 0;
        unsigned char instruction[max_instruction_length];
        int index = new_pipeline.predecode_EIP & 0x003F;
        int part1 = -1;
        int part2 = -1;
        int len = 0;
        unsigned char prefix = -1;
        unsigned char opcode;
        if(index < cache_line_size){
            part1 = 0;
        }
        else if(index < 32){
            part1 = 1;
        }
        else if(index < 48){
            part1 = 2;
        }
        else if(index < 64){
            part1 = 3;
        }
        if((index % 16) > 1){ // need next cache line as well if the offset is over 1
            part2 = (part1 + 1) % 4;
        }
        index = index % 16;
        int curPart = part1;
        for(int i = 0; i < 15; i++){ //copy over instruction, using bytes from the second cache line if needed
            instruction[i] = new_pipeline.predecode_ibuffer[curPart][index];
            index++;
            if(index > 15){
                index = 0;
                curPart = part2;
            }
        }
        int instIndex = 0;
        if(instruction[instIndex] == 0x66 || instruction[instIndex] == 0x67){ //check prefix
            len++;
            prefix = instruction[instIndex];
            instIndex++;
            opcode = instruction[instIndex];
        }
        else{
            opcode = instruction[instIndex];
        }
        if(!(instruction[instIndex] == 0x05 || instruction[instIndex] == 0x04)){ // if an instruction tha requires a mod/rm byte
            len+=2;
            instIndex++;
            unsigned char mod_rm = instruction[instIndex];
            if((mod_rm & 0b11000000) == 0 && (mod_rm & 0b0100) != 0){ //uses SIB byte
                len++;
            }
            int displacement = mod_rm & 0b11000000;
            if(displacement == 1){
                len++;
            }
            if(displacement == 2){
                len+=2;
            }
        }
        else{
            len++;
        }
        if(((opcode == 0x81) && prefix == 0x66) || ((opcode == 0x05) && prefix == 0x66)){ //determine size of immediates
            len +=2;
            new_pipeline.decode_immSize = 1;
        }
        else if(opcode == 0x81 || opcode == 0x05){
            len+=4;
        }
        else if((opcode == 04) || (opcode == 80) || (opcode == 83)){ // all instructions with 1 byte immediate
            len++;
        }
        new_pipeline.decode_instruction_length = len;
        new_pipeline.decode_valid = 1;
        for(int i =0;i<max_instruction_length;i++){
            new_pipeline.decode_instruction_register[i] = instruction[i];
        }
        new_pipeline.decode_EIP = new_pipeline.predecode_EIP;
        length = len;
    }
}

void fetch_stage(){
    int offset = EIP & 0x3F;
    int current_sector = offset/ibuffer_size;
    int line_offset = offset%cache_line_size;
    if(line_offset<=1){
        if(ibuffer_valid[current_sector]==TRUE){
            //latch predecode valid and ibuffer
            new_pipeline.predecode_valid=TRUE;
            new_pipeline.predecode_offset = offset;
            new_pipeline.predecode_current_sector = current_sector;
            new_pipeline.predecode_line_offset = line_offset;
            for(int i =0;i<ibuffer_size;i++){
                for(int j=0;j<cache_line_size;j++){
                    new_pipeline.predecode_ibuffer[i][j]=ibuffer[i][j];
                }
            }
        }else{
            new_pipeline.predecode_valid=FALSE;
        }
    }else{
        if((ibuffer_valid[current_sector]==TRUE)&&(ibuffer_valid[(current_sector+1)%ibuffer_size]==TRUE)){
            //latch predecode valid and ibuffer
            new_pipeline.predecode_valid=TRUE;
            new_pipeline.predecode_offset = offset;
            new_pipeline.predecode_current_sector = current_sector;
            new_pipeline.predecode_line_offset = line_offset;
            for(int i =0;i<ibuffer_size;i++){
                for(int j=0;j<cache_line_size;j++){
                    new_pipeline.predecode_ibuffer[i][j]=ibuffer[i][j];
                }
            }
        }else{
            new_pipeline.predecode_valid=FALSE;
        }
    }

    int bank_aligned=FALSE;
    int bank_offset=FALSE;
    int* dataBits0[cache_line_size],dataBits1[cache_line_size], *tlb_hit, *tlb_physical_tag;
    I$_TagStoreEntry* icache_tag_metadata;
    if(ibuffer_valid[(current_sector)]==FALSE){
        icache_access(EIP,dataBits0,dataBits0,icache_tag_metadata);
        tlb_access(EIP,tlb_physical_tag,tlb_hit);
        if((tlb_hit&&icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0== *tlb_physical_tag)){
            ibuffer_valid[current_sector]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[current_sector][i] = *dataBits0[i];
            }
        }else if((tlb_hit&&icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1== *tlb_physical_tag)){
            ibuffer_valid[current_sector]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[current_sector][i]=dataBits1[i];
            }
        }else if(tlb_hit || (((icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 != *tlb_physical_tag)) && ((icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 != *tlb_physical_tag)))){
            mshr_preinserter(EIP, 0, 0);
        }
        bank_aligned=TRUE;
    }
    if(ibuffer_valid[(current_sector+1)%ibuffer_size]==FALSE){
        icache_access(EIP+16,dataBits0,dataBits0,icache_tag_metadata);
        tlb_access(EIP+16,tlb_physical_tag,tlb_hit);
        if((tlb_hit&&icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 == *tlb_physical_tag)){
            ibuffer_valid[(current_sector+1)%ibuffer_size]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[(current_sector+1)%ibuffer_size][i] = *dataBits0[i];
            }
        }else if((tlb_hit&&icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 == *tlb_physical_tag)){
            ibuffer_valid[(current_sector+1)%ibuffer_size]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[(current_sector+1)%ibuffer_size][i]=dataBits1[i];
            }
        }else if(tlb_hit || (((icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 != *tlb_physical_tag)) && ((icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 != *tlb_physical_tag)))){
            mshr_preinserter(EIP+16, 0, 0);
        }
        bank_offset=TRUE;
    }
    if(ibuffer_valid[(current_sector+2)%ibuffer_size]==FALSE && bank_aligned==FALSE){
        icache_access(EIP+32,dataBits0,dataBits0,icache_tag_metadata);
        tlb_access(EIP+32,tlb_physical_tag,tlb_hit);
        if((tlb_hit&&icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 == *tlb_physical_tag)){
            ibuffer_valid[(current_sector+2)%ibuffer_size]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[(current_sector+2)%ibuffer_size][i] = *dataBits0[i];
            }
        }else if((tlb_hit&&icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 == *tlb_physical_tag)){
            ibuffer_valid[(current_sector+2)%ibuffer_size]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[(current_sector+2)%ibuffer_size][i]=dataBits1[i];
            }
        }else if(tlb_hit || (((icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 != *tlb_physical_tag)) && ((icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 != *tlb_physical_tag)))){
            mshr_preinserter(EIP+32, 0, 0);
        }
        bank_aligned=TRUE;
    }
    if(ibuffer_valid[(current_sector+3)%ibuffer_size]==FALSE && bank_offset==FALSE){
        icache_access(EIP+48,dataBits0,dataBits0,icache_tag_metadata);
        tlb_access(EIP+48,tlb_physical_tag,tlb_hit);
        if((tlb_hit&&icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 == *tlb_physical_tag)){
            ibuffer_valid[(current_sector+3)%ibuffer_size]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[(current_sector+3)%ibuffer_size][i] = *dataBits0[i];
            }
        }else if((tlb_hit&&icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 == *tlb_physical_tag)){
            ibuffer_valid[(current_sector+3)%ibuffer_size]=TRUE;
            for(int i =0;i<cache_line_size;i++){
                ibuffer[(current_sector+3)%ibuffer_size][i]=dataBits1[i];
            }
        }else if(tlb_hit || (((icache_tag_metadata->valid_way0) && (icache_tag_metadata->tag_way0 != *tlb_physical_tag)) && ((icache_tag_metadata->valid_way1) && (icache_tag_metadata->tag_way1 != *tlb_physical_tag)))){
            mshr_preinserter(EIP+48, 0, 0);
        }
        bank_offset=TRUE;
    }
    
    new_pipeline.predecode_EIP = EIP;
    oldEIP = EIP;
    EIP +=length;
    if(length>=(16-line_offset)){
        ibuffer_valid[current_sector]=TRUE;
    }
}

void mshr_inserter(){
    //insert icache requests
    for(int i = 0;i<pre_mshr_size;i++){
        if(mshr.pre_entries[i].valid && mshr.pre_entries[i].origin ==0){
            for(int j =0;j<mshr_size;j++){
                if(mshr.occupancy<mshr_size){
                    if(mshr.entries[i].valid==FALSE){
                        mshr.entries[i].valid=TRUE;
                        mshr.entries[i].old_bits=mshr.occupancy;
                        mshr.occupancy++;
                        mshr.entries[i].origin=mshr.pre_entries[j].origin;
                        mshr.entries[i].address=mshr.pre_entries[j].address;
                        mshr.entries[i].read_or_write=mshr.pre_entries[j].read_or_write;
                        mshr.entries[i].requested=FALSE;
                        mshr.entries[i].sending_data=FALSE;
                    }
                }
            }
        }
    }
    //insert dcache requests
    for(int i = 0;i<pre_mshr_size;i++){
        if(mshr.pre_entries[i].valid && mshr.pre_entries[i].origin ==1){
            for(int j =0;j<mshr_size;j++){
                if(mshr.occupancy<mshr_size){
                    if(mshr.entries[i].valid==FALSE){
                        mshr.entries[i].valid=TRUE;
                        mshr.entries[i].old_bits=mshr.occupancy;
                        mshr.occupancy++;
                        mshr.entries[i].origin=mshr.pre_entries[j].origin;
                        mshr.entries[i].address=mshr.pre_entries[j].address;
                        mshr.entries[i].read_or_write=mshr.pre_entries[j].read_or_write;
                        mshr.entries[i].requested=FALSE;
                        mshr.entries[i].sending_data=FALSE;
                    }
                }
            }
        }
    }
    //insert tlb requests
    for(int i = 0;i<pre_mshr_size;i++){
        if(mshr.pre_entries[i].valid && mshr.pre_entries[i].origin ==2){
            for(int j =0;j<mshr_size;j++){
                if(mshr.occupancy<mshr_size){
                    if(mshr.entries[i].valid==FALSE){
                        mshr.entries[i].valid=TRUE;
                        mshr.entries[i].old_bits=mshr.occupancy;
                        mshr.occupancy++;
                        mshr.entries[i].origin=mshr.pre_entries[j].origin;
                        mshr.entries[i].address=mshr.pre_entries[j].address;
                        mshr.entries[i].read_or_write=mshr.pre_entries[j].read_or_write;
                        mshr.entries[i].requested=FALSE;
                        mshr.entries[i].sending_data=FALSE;
                    }
                }
            }
        }
    }
}

void bus_arbiter(){
    //approach: probe metadata bus and do casework
    //search mshr for oldest entry
    int start_age=0;
    MSHR_Entry entry_to_send;
    int found_entry = FALSE;
    for(int i =0;i<mshr_size;i++){
        if(mshr.entries[i].valid && !(mshr.entries[i].requested) && (mshr.entries[i].old_bits==start_age)){
            if(metadata_bus.bank_status[get_bank_bits(mshr.entries[i].address)]==0){ 
                //probing bank status on metadata bus to see if that bank is available
                entry_to_send = mshr.entries[i];
                found_entry=true;
                break;
            }else{
                start_age++;
                i=-1;
            }
        }
    }
    int is_data_bus_active = metadata_bus.is_serializer_sending_data && metadata_bus.receive_enable;
    //INACTIVE MEMORY CASES
    //case 1: MSHR has stuff, serializers dont

    //case 2: MSHR doesnt have stuff, serializers do

    //case 3: MSHR doesn't have stuff, serializers dont

    //case 4: MSHR has stuff, serializers have stuff

    //ACTIVE MEMORY CASES
}

void memory_controller(){

}
<<<<<<< HEAD

void deserializer_handler(){

}
=======
>>>>>>> 2ef23aa07e250d9420d151b285f53947786f80f3
