//bus.h 
#pragma once
#include <stdint.h>
#include "ines.h"

typedef struct Bus{
    //1. 系统自带的2KB RAM
    // NES 的 RAM 只有 2KB (0x800)，范围是 0x0000-0x07FF
    uint8_t ram[2048];

    //2.插在总线上的卡带
    NesRom* cartridge;

    // 3. 未来还需要加入 PPU, APU, 手柄状态等
    // struct PPU* ppu; 
    // uint8_t controller_state[2];

} Bus;

// 初始化总线，把卡带插上去
void bus_init(Bus* bus, NesRom* rom);

// CPU 通过这两个函数与总线交互
uint8_t bus_read(Bus* bus, uint16_t addr);
void bus_write(Bus* bus, uint16_t addr, uint8_t data);


