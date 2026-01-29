#pragma once
#include <stdint.h>
#include "bus.h"

enum Flags6502{
    C = (1 << 0), // Carry Bit
    Z = (1 << 1), // Zero
    I = (1 << 2), // Disable Interrupts
    D = (1 << 3), // Decimal Mode (unused in this implementation)
    B = (1 << 4), // Break
    U = (1 << 5), // Unused
    V = (1 << 6), // Overflow
    N = (1 << 7), // Negative
};

typedef struct CPU{
    // Registers
    uint8_t  a;      // Accumulator Register
    uint8_t  x;      // X Register
    uint8_t  y;      // Y Register
    uint8_t  stkp;     // Stack Pointer(指向 0x0100 - 0x01FF)
    uint16_t pc;     // Program Counter
    uint8_t  status; // Status Register

    //连接总线
    Bus* bus;

    //模拟辅助变量
    uint8_t fetched_data; //当前指令取到的数据
    uint16_t addr_abs;   //当前指令计算出的绝对地址
    uint16_t addr_rel;  //分支指令的相对地址偏移
    uint8_t opcode;   //当前指令的操作码
    uint8_t  cycles;         // 当前指令剩余的执行周期数

} CPU;