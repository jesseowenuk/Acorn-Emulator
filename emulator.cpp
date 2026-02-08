#include <stdint.h>
#include <stdio.h>

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

// MAIN ////////////////////////////////////////
int main()
{
    // Create a CPU and set all the registers to 0
    CPU16 cpu = {0};

    // Set up IP and CS
    cpu.CS = 0x0000;
    cpu.IP = 0x2000;        // For this test we'll start with the memory we wrote too

    ///////// TESTING /////////////

    // Write our program to memory
    write8(0x2000, 0xB8);           // MOV AX, imm16
    write16(0x2001, 0x1234);        // immediate value 0x1234

    // Fetch a single instruction
    uint16_t physical_address = cpu.CS * 16 + cpu.IP;
    uint8_t opcode = read8(physical_address);
    cpu.IP++;               // move to the next instruction

    // Minimal decode
    if(opcode == 0xB8)  // MOV AX, imm16
    {
        // Fetch the next two bytes as immediate value
        uint16_t imm = read16(cpu.CS * 16 + cpu.IP);
        cpu.AX = imm;
        cpu.IP += 2;    // move past the immediate bytes
        printf("Executed MOV AX, 0x%04X\n", cpu.AX);
    }
    else
    {
        printf("Unknown opcode: 0x%02X\n", opcode);
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