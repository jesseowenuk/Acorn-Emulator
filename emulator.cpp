#include <stdint.h>
#include <stdio.h>

#define DEBUG 1

#define MEMORY_SIZE 0x100000            // 1MB of memory
uint8_t memory[MEMORY_SIZE];            // create an array to store our 1MB of RAM

typedef struct
{
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

// MAIN ////////////////////////////////////////
int main()
{
    // Create a CPU and set all the registers to 0
    CPU16 cpu = {0};

    // Set up IP and CS
    cpu.CS = 0x0000;
    cpu.IP = 0x2000;
    cpu.SS = 0x0000;
    cpu.SP = 0xFFFE;                    // top of the stack near end of the memory segment  

    // Write our program to memory
    uint32_t address = 0x2000;

    // MOV AX, 1234
    write8(address++, 0xB8); write16(address, 0x1234); address += 2;

    // PUSH AX
    write8(address++, 0x50); 

    // MOV AX, 0
    write8(address++, 0xB8); write16(address, 0x0000); address += 2;

    // POP AX
    write8(address++, 0x58); 

    // Fetch / Decode Loop
    while(1)
    {
        // Step 1: Fetch
        uint16_t physical_address = cpu.CS * 16 + cpu.IP;
        uint8_t opcode = read8(physical_address);
        cpu.IP++;               // move past the opcode

        // Step 2: Decode & Execute
        switch(opcode)
        {
            // MOV AX, 16_bit_value
            case 0xB8:
            {
                // Fetch the next two bytes as immediate value
                uint16_t imm = read16(cpu.CS * 16 + cpu.IP);
                cpu.AX = imm;
                cpu.IP += 2; 
                #if DEBUG 
                printf("Executed MOV AX, 0x%04X\n", cpu.AX);
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

            default:
            {
                #if DEBUG
                printf("Unknown opcode: 0x%02X\n", opcode);
                #endif
                return 0;
            }
        }

        #if DEBUG
        printf("AX=%04X  SP=%04X  IP=%04X\n", cpu.AX, cpu.SP, cpu.IP);
        #endif
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