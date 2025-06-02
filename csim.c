#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fetch_stage();
void predecode_stage();
void decode_stage();
void addgen_branch_stage();
void register_rename_stage();
void execute_stage();
void memory_stage();
void writeback_stage();

#define control_store_rows 20 //arbitrary
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
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x01000 
int MEMORY[WORDS_IN_MEM][4];

//architectural registers

int EAX, EBX, ECX, EDX, ESI, EDI, EBP, ESP;
int EIP, EFLAGS;
int oldEIP;

#define cache_line_size 16
int ibuffer[4][cache_line_size];
int ibuffer_sector0_valid, ibuffer_sector1_valid, ibuffer_sector2_valid, ibuffer_sector3_valid;


typedef struct PipeState_Entry_Struct{
  int predecode_valid, predecode_ibuffer[4][cache_line_size], predecode_EIP,
      decode_valid, decode_instruction_register, decode_instruction_length, decode_EIP,
      agbr_valid, agbr_cs[num_control_store_bits], agbr_NEIP,
      agbr_op1_base, agbr_op1_index, agbr_op1_scale, agbr_op1_disp,
      agbr_op2_base, agbr_op2_index, agbr_op2_scale, agbr_op2_disp,
      rr_valid, rr_operation, rr_updated_flags, 
      rr_op1_base, rr_op1_index, rr_op1_scale, rr_op1_disp, rr_op1_addr_mode,
      rr_op2_base, rr_op2_index, rr_op2_scale, rr_op2_disp, rr_op2_addr_mode
} PipeState_Entry;

PipeState_Entry pipeline, new_pipeline;

int cycle_count;

int RUN_BIT;

void cycle(){
    new_pipeline = pipeline;
    writeback_stage();
    memory_stage();
    execute_stage();
    register_rename_stage();
    addgen_branch_stage();
    decode_stage();
    predecode_stage();
    fetch_stage();
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
    printf("DC IR       :  0x%04x\n", pipeline.decode_instruction_register );
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
	help();
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

//functionality

void icache_access(){

}

void dcache_access(){

}

void tlb_access(){

}

void busarb_logic(){

}

void translation_logic(){

}

//

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
typedef struct D$_TagStoreEntry_Struct{
    int valid_way0, tag_way0, dirty_way0,
        valid_way1, tag_way1, dirty_way1,
        valid_way1, tag_way1, dirty_way2,
        valid_way1, tag_way1, dirty_way3,
        lru 
} D$_TagStoreEntry;
typedef struct D$_TagStore_Struct{
    D$_TagStoreEntry dcache_tagstore[dcache_banks][dcache_sets];
} D$_TagStore;
typedef struct D$_DataStore_Struct{
    long dcache_datastore[dcache_banks][dcache_sets][2];
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
typedef struct I$_TagStoreEntry_Struct{
    int valid_way0, tag_way0, dirty_way0,
        valid_way1, tag_way1, dirty_way1,
        lru 
} I$_TagStoreEntry;
typedef struct I$_TagStore_Struct{
    I$_TagStoreEntry icache_tagstore[icache_banks][icache_sets];
} I$_TagStore;
typedef struct I$_DataStore_Struct{
    long icache_datastore[icache_banks][icache_sets][2];
} I$_DataStore;
typedef struct I$_Struct{
    I$_DataStore data;
    I$_TagStore tag;
} I$;

I$ icache;

//tlb
#define tlb_entries 8
typedef struct TLBEntry_Struct{
    int valid, present, permissions, vpn, pfn
} TLBEntry;
typedef struct TLB_Struct{
    TLBEntry entries[tlb_entries];
} TLB;

TLB tlb;

//lsq

//rat
typedef struct RAT_MetadataEntry_Struct{
    int valid, alias
} RAT_MetadataEntry;

//fat

//reservation_stations

//mshr

//btb

//SpecExeTracker

//ROB
#define rob_size 16
typedef struct ROB_Entry_Struct{
  int valid, old_bits, retired, executed, value, store_tag
} ROB_Entry;
typedef struct ROB_Struct{
    ROB_Entry entries[rob_size];
} ROB;
ROB rob;

//stages

int rob_broadcast_value, rob_broadcast_tag;
void writeback_stage(){
    for(int i =0;i<rob_size;i++){
        if((rob.entries[i].valid==1) && (rob.entries[i].old_bits==0) && (rob.entries[i].retired!=1) && (rob.entries[i].executed==1)){
            rob_broadcast_value = rob.entries[i].value;
            rob_broadcast_tag = rob.entries[i].store_tag;
            rob.entries[i].valid=0;
            rob.entries[i].retired=1;
            for(int j =0;j<rob_size;j++){
                if((rob.entries[j].valid==1) && (rob.entries[j].retired!=1)){
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

}

void decode_stage(){

}

void predecode_stage(){

}

void fetch_stage(){
    
    //manage ibuffer, icache accesses

    //latch valid
    new_pipeline.predecode_EIP = EIP;
    oldEIP = EIP;
}
