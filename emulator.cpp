#include <stdint.h>
#include <stdio.h>

typedef struct
{
    // 16-bit registers (general purpose)
    // These can also be addressed as high or low
    // eg AX low bytes AL and high bytes AH
    uint16_t AX;
    uint16_t BX;
    uint16_t CX;
    uint16_t DX;

    // These registers are used as offsets into the data space
    uint16_t SI;    // SI is usually an offset from the DS segment
    uint16_t DI;    // DI is usually an offset from the ES segment
    uint16_t BP;    // The stack frame - usually an offset from the stack segment (SS)
    uint16_t SP;    // The stack pointer - usually an offset from the stack segment (SS)
} CPU16;

int main()
{
    return 0;
}