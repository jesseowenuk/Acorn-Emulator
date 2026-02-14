#include <stdint.h>
#include <stdio.h>

#define DEBUG 1

#define MEMORY_SIZE 0x100000            // 1MB of memory

// FLAGS defines
#define FLAG_CF 0x0001                  // Carry flag
#define FLAG_ZF 0x0040                  // Zero flag
#define FLAG_SF 0x0080                  // Sign flag
#define FLAG_OF 0x0800                  // Overflow flag

uint8_t memory[MEMORY_SIZE];            // create an array to store our 1MB of RAM

typedef struct
{
    int running;

    // 16-bit registers (general purpose)
    // These can also be addressed as high or low
    // eg AX low bytes AL and high bytes AH
    uint16_t AX;    // Accumulator
    uint16_t BX;    // Base
    uint16_t CX;    // Counter
    uint16_t DX;    // Data

    // These registers are used as offsets into the data space
    uint16_t SI;    // SI is usually an offset from the DS segment
    uint16_t DI;    // DI is usually an offset from the ES segment
    uint16_t BP;    // Stack base pointer - usually an offset from the stack segment (SS)
    uint16_t SP;    // The stack pointer - usually an offset from the stack segment (SS)

    // The instruction pointer
    uint16_t IP;

    // The segments
    uint16_t CS;
    uint16_t DS;
    uint16_t ES;
    uint16_t SS;

    // Flags
    uint16_t FLAGS;
} CPU16;

// The Functions
uint8_t read8(uint32_t address);
void write8(uint32_t address, uint8_t value);
uint16_t read16(uint32_t address);
void write16(uint32_t address, uint16_t value);
void push16(CPU16 *cpu, uint16_t value);
uint16_t pop16(CPU16 *cpu);
void debug_state(CPU16 *cpu, int show_stack);

// MAIN ////////////////////////////////////////
int main()
{
    // Create a CPU and set all the registers to 0
    CPU16 cpu = {0};
    cpu.running = 1;

    // Set up IP and CS
    cpu.CS = 0x0000;
    cpu.IP = 0x2000;
    cpu.SS = 0x0000;
    cpu.SP = 0xFFFE;                    // top of the stack near end of the memory segment  

    // Write our program to memory
    uint32_t address = 0x2000;

    // MOV AX, 3
    write8(0x2000, 0xB8); write16(0x2001, 0x0003);

    // CMP AX, 5
    write8(0x2003, 0x3D); write16(0x2004, 0x0005);

    // JL +4 (jump to 0x200C)
    write8(0x2006, 0x7C); write8(0x2007, 0x04);

    // MOV AX, 0x9999
    write8(0x2008, 0xB8); write16(0x2009, 0x9999);

    // HLT
    write8(0x200B, 0xF4);

    // + 5
    write8(0x200C, 0xB8); write16(0x200D, 0x1111);

    // HLT
    write8(0x200F, 0xF4);
   

    // Fetch / Decode Loop
    while(cpu.running)
    {
        // Step 1: Fetch
        uint16_t physical_address = cpu.CS * 16 + cpu.IP;
        uint8_t opcode = read8(physical_address);
        cpu.IP++;               // move past the opcode

        // Step 2: Decode & Execute
        switch(opcode)
        {
            // All the MOV's
            // AX, BX, CX, DX, SP, BP, SI & DI
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            {
                // Fetch the next two bytes as immediate value
                uint16_t value = read16(cpu.CS * 16 + cpu.IP);
                cpu.IP += 2; 

                switch(opcode)
                {
                    case 0xB8: cpu.AX = value; break;
                    case 0xB9: cpu.CX = value; break;
                    case 0xBA: cpu.DX = value; break;
                    case 0xBB: cpu.BX = value; break;
                    case 0xBC: cpu.SP = value; break;
                    case 0xBD: cpu.BP = value; break;
                    case 0xBE: cpu.SI = value; break;
                    case 0xBF: cpu.DI = value; break;
                }
                #if DEBUG 
                printf("Executed MOV reg, 0x%04X\n", value);
                #endif
                break;
            }

            // MOV AH, 8_bit_value
            case 0xB4:
            {
                uint8_t imm = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;
                cpu.AX = (imm << 8) | (cpu.AX & 0x00FF);    // keep AL
                #if DEBUG
                printf("Executed MOV AH, 0x%02X\n", imm);
                #endif
                break;
            }

            // MOV AL, 8_bit_value
            case 0xB0:
            {
                uint8_t imm = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;
                cpu.AX = (cpu.AX & 0xFF00) | imm;       // keep AH
                #if DEBUG
                printf("Exeecuted MOV AL, 0x%02X\n", imm);
                #endif
                break;
            }

            // INT, 8_bit_value
            case 0xCD:
            {
                uint8_t int_num = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;

                if(int_num == 0x10 && (cpu.AX >> 8) == 0x0E) // AH = high byte of AX  
                {
                    char c = cpu.AX & 0xFF;     // AL = low byte of AX
                    putchar(c);
                }
                else
                {
                    #if DEBUG
                    printf("\nUnknown interrupt 0x%02X with AH=0x%02X\n", int_num, cpu.AX >> 8);
                    #endif
                }
                break;
            }

            // PUSH AX
            case 0x50:
            {
                push16(&cpu, cpu.AX);
                #if DEBUG
                printf("Executed PUSH AX\n");
                #endif
                break;
            }

            // POP AX
            case 0x58:
            {
                cpu.AX = pop16(&cpu);
                #if DEBUG
                printf("Executed POP AX\n");
                #endif
                break;
            }

            // CALL rel16
            case 0xE8:
            {
                // 1. read 16-bit relative offset after opcode
                uint16_t offset = read16(cpu.CS * 16 + cpu.IP);
                cpu.IP += 2;    // move past the operand

                // 2. Push current IP (the return address)
                push16(&cpu, cpu.IP);

                // 3. Jump to new address
                cpu.IP += offset;

                #if DEBUG
                printf("Exeecuted CALL 0x%04X\n", offset);
                #endif
                break;
            }

            // RET
            case 0xC3:
            {
                cpu.IP = pop16(&cpu);
                #if DEBUG
                printf("Exeecuted RET\n");
                #endif
                break;
            }

            // HLT
            case 0xF4:
            {
                #if DEBUG
                printf("CPU halted\n");
                #endif
                cpu.running = 0;
                break;
            }

            // CMP AX, imm16
            case 0x3D:
            {
                uint16_t value = read16(cpu.CS * 16 + cpu.IP);
                cpu.IP += 2;            // Move past the number

                uint32_t result = cpu.AX - value;

                // Clear old flag values (only the ones we are using)
                cpu.FLAGS &= ~(FLAG_CF | FLAG_ZF | FLAG_SF | FLAG_OF);

                // Zero flag
                if((result & 0xFFFF) == 0)
                {
                    cpu.FLAGS |= FLAG_ZF;
                }

                // Sign flag (bits 15 of 16-bit result)
                if(result & 0x8000)
                {
                    cpu.FLAGS |= FLAG_SF;
                }

                // Carry flag (borrow happened)
                if(cpu.AX < value)
                {
                    cpu.FLAGS |= FLAG_CF;
                }

                // Overflow detection
                uint16_t result16 = (uint16_t)result;

                if(((cpu.AX ^ value) & (cpu.AX ^ result16) & 0x8000)!= 0)
                {
                    cpu.FLAGS |= FLAG_OF;
                }

                #ifdef DEBUG
                printf("Executed CMP AX, 0x%04X\n", value);
                #endif
                break;
            }

            // JE rel8
            case 0x74:
            {
                int8_t offset = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;

                if(cpu.FLAGS & FLAG_ZF)
                {
                    cpu.IP += offset;
                    #ifdef DEBUG
                    printf("Executed JE (taken) %d\n", offset);
                    #endif
                }
                else
                {
                    #ifdef DEBUG
                    printf("Executed JE (not taken)\n");
                    #endif
                }
                break;
            }

            // DEC CX
            case 0x49:
            {
                cpu.CX--;

                // Clear the flags we manage
                cpu.FLAGS &= ~(FLAG_ZF | FLAG_SF);

                if(cpu.CX == 0)
                {
                    cpu.FLAGS |= FLAG_ZF;
                }

                if(cpu.CX & 0x8000)
                {
                    cpu.FLAGS |= FLAG_SF;
                }

                #ifdef DEBUG
                printf("Executed DEC CX\n");
                #endif
                break;
            }

            // JNE rel8
            case 0x75:
            {
                int8_t offset = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;

                if(!(cpu.FLAGS & FLAG_ZF))
                {
                    cpu.IP += offset;
                    #ifdef DEBUG
                    printf("Executed JNE (taken) %d\n", offset);
                    #endif
                }
                else
                {
                    #ifdef DEBUG
                    printf("Executed JNE (not taken)\n");
                    #endif
                }

                break;
            }

            // JMP rel8
            case 0xEB:
            {
                int8_t offset = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;           // move past the offset byte

                cpu.IP += offset;

                #ifdef DEBUG
                printf("Executed JMP %d\n", offset);
                #endif
                break;
            }

            // JL rel8
            case 0x7C:
            {
                int8_t offset = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;

                int sign_flag = (cpu.FLAGS & FLAG_SF) ? 1 : 0;
                int overflow_flag = (cpu.FLAGS & FLAG_OF) ? 1 : 0;

                if(sign_flag != overflow_flag)
                {
                    cpu.IP += offset;
                    #ifdef DEBUG
                    printf("Executed JL %d (taken)\n", offset);
                    #endif
                }
                #ifdef DEBUG
                else
                {
                    printf("Executed JL %d (not taken)\n", offset);
                }
                #endif

                break;
            }

            // JG rel8
            case 0x7F:
            {
                int8_t offset = read8(cpu.CS * 16 + cpu.IP);
                cpu.IP++;

                int sign_flag = (cpu.FLAGS & FLAG_SF) ? 1 : 0;
                int overflow_flag = (cpu.FLAGS & FLAG_OF) ? 1 : 0;
                int zero_flag = (cpu.FLAGS & FLAG_ZF) ? 1 : 0;

                if(!zero_flag && (sign_flag == overflow_flag))
                {
                    cpu.IP += offset;

                    #ifdef DEBUG
                    printf("Executed JG %d (taken)\n", offset);
                    #endif
                }
                #ifdef DEBUG
                else
                {
                    printf("Executed JG %d (not taken)\n", offset);
                }
                #endif

                break;
            }

            default:
            {
                #if DEBUG
                printf("Unknown opcode: 0x%02X\n", opcode);
                #endif
                return 0;
            }
        }

        debug_state(&cpu, 1);
    }

    return 0;
}

// Function to return whatever 8-bit value is stored 
// in memory at the address specified
uint8_t read8(uint32_t address)
{
    return memory[address];
}

// Function to write a specified 8-bit value
// in memory at the address specified
void write8(uint32_t address, uint8_t value)
{
    memory[address] = value;
}

// Function to return whatever 16-bit value is stored 
// in memory at the address specified
uint16_t read16(uint32_t address)
{
    return memory[address] | (memory[address + 1] << 8);
}

// Function to write a specified 16-bit value
// in memory at the address specified
void write16(uint32_t address, uint16_t value)
{
    memory[address] = value & 0xFF;
    memory[address + 1] = (value >> 8) & 0xFF;
}

// Push (add) a value onto the stack
void push16(CPU16 *cpu, uint16_t value)
{
    cpu->SP -= 2;           // stack grows downwards
    uint32_t address = cpu->SS * 16 + cpu->SP;
    write16(address, value);
}

// Pop (return) a value from the stack 
uint16_t pop16(CPU16 *cpu)
{
    uint32_t address = cpu->SS * 16 + cpu->SP;
    uint16_t value = read16(address);
    cpu->SP += 2;
    return value;
}

// Print debugging information
void debug_state(CPU16 *cpu, int show_stack)
{
    #if DEBUG
        printf("AX=%04X  BX=%04X  CX=%04X  DX=%04X\n", cpu->AX, cpu->BX, cpu->CX, cpu->DX);
        printf("CS:IP=%04X:%04X  DS=%04X  ES=%04X  SS:SP=%04X:%04X  FLAGS=%04X\n", cpu->CS, cpu->IP, cpu->DS, cpu->ES, cpu->SS, cpu->SP, cpu->FLAGS);
        printf("FLAGS=%04X (OF=%d ZF=%d SF=%d CF=%d)\n",
            cpu->FLAGS,
            cpu->FLAGS & FLAG_OF ? 1 : 0,
            cpu->FLAGS & FLAG_ZF ? 1 : 0,
            cpu->FLAGS & FLAG_SF ? 1 : 0,
            cpu->FLAGS & FLAG_CF ? 1 : 0
        );

        if(show_stack)
        {
            printf("[STACK] Top 4 bytes:");
            uint32_t address = cpu->SS * 16 + cpu->SP;

            for(int i = 0; i < 4; i++)
            {
                printf(" %02X", memory[address + i]);
            }

            printf("\n");
        }

        printf("\n");
    #endif
}