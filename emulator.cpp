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

    ///////// TESTING /////////////
    // Step A: Put numbers into memory
    write8(0x1000, 42);         // Put 42 in memory 0x1000
    write16(0x2000, 0x1234);    // Put 1234 in memory 0x2000 & 0x2001

    // Step B: Read them back
    uint8_t a = read8(0x1000);
    uint16_t b = read16(0x2000);

    // Step C: Print to test
    printf("Memory[0x1000] = %u\n", a);     // should print 42
    printf("Memory[0x2000] = 0x%04X\n", b);     // should print 0x1234

    // Set up IP and CS
    cpu.CS = 0x0000;
    cpu.IP = 0x2000;        // For this test we'll start with the memory we wrote too

    // Fetch a single instruction
    uint16_t physical_address = cpu.CS * 16 + cpu.IP;
    uint8_t opcode = read8(physical_address);
    cpu.IP++;               // move to the next instruction

    printf("Fetched opcode: 0x%02X from memory[0x%04X]\n", opcode, physical_address);

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