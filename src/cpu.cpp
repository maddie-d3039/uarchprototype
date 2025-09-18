#include "../inc/config.h"
#include "../inc/cpu.h"

void cpu::fetch_stage(){
    the_ibuffer->operate(EIP, &new_pipeline);
    new_pipeline.predecode_EIP = EIP;
    oldEIP = EIP;
    EIP += length;
    the_ibuffer->invalidate();
}

void cpu::predecode_stage(){ //add enums to this
   
    if (new_pipeline.predecode_valid)
    {
        new_pipeline.decode_immSize = 0;
        unsigned char instruction[max_instruction_length];
        int index = new_pipeline.predecode_EIP & 0x003F;
        int part1 = -1;
        int part2 = -1;
        int len = 0;
        unsigned char prefix = -1;
        unsigned char opcode;
        if (index < cache_line_size)
        {
            part1 = 0;
        }
        else if (index < 32)
        {
            part1 = 1;
        }
        else if (index < 48)
        {
            part1 = 2;
        }
        else if (index < 64)
        {
            part1 = 3;
        }
        if ((index % 16) > 1)
        { // need next cache line as well if the offset is over 1
            part2 = (part1 + 1) % 4;
        }
        index = index % 16;
        int curPart = part1;
        for (int i = 0; i < 15; i++)
        { // copy over instruction, using bytes from the second cache line if needed
            instruction[i] = new_pipeline.predecode_ibuffer[curPart][index];
            index++;
            if (index > 15)
            {
                index = 0;
                curPart = part2;
            }
        }
        int instIndex = 0;
        if (instruction[instIndex] == 0x66 || instruction[instIndex] == 0x67)
        { // check prefix
            len++;
            prefix = instruction[instIndex];
            instIndex++;
            opcode = instruction[instIndex];
        }
        else
        {
            opcode = instruction[instIndex];
        }
        if (!(instruction[instIndex] == 0x05 || instruction[instIndex] == 0x04))
        { // if an instruction tha requires a mod/rm byte
            len += 2;
            instIndex++;
            unsigned char mod_rm = instruction[instIndex];
            if ((mod_rm & 0b11000000) == 0 && (mod_rm & 0b0100) != 0)
            { // uses SIB byte
                len++;
            }
            int displacement = mod_rm & 0b11000000;
            if (displacement == 1)
            {
                len++;
            }
            if (displacement == 2)
            {
                len += 2;
            }
        }
        else
        {
            len++;
        }
        if (((opcode == 0x81) && prefix == 0x66) || ((opcode == 0x05) && prefix == 0x66))
        { // determine size of immediates
            len += 2;
            new_pipeline.decode_immSize = 1;
        }
        else if (opcode == 0x81 || opcode == 0x05)
        {
            len += 4;
        }
        else if ((opcode == 04) || (opcode == 80) || (opcode == 83))
        { // all instructions with 1 byte immediate
            len++;
        }
        new_pipeline.decode_instruction_length = len;
        new_pipeline.decode_valid = 1;
        for (int i = 0; i < max_instruction_length; i++)
        {
            new_pipeline.decode_instruction_register[i] = instruction[i];
        }
        new_pipeline.decode_EIP = new_pipeline.predecode_EIP;
        length = len;
    }
}

