// bus.c
#include "bus.h"
#include <string.h> // for memset

void bus_init(Bus* bus, NesRom* rom){
    // 1. 初始化 RAM 为 0
    memset(bus->ram, 0, sizeof(bus->ram));

    // 连接卡带 ("插入卡带")
    bus->cartridge = rom;
}

// 模拟 CPU 读取内存
uint8_t bus_read(Bus* bus, uint16_t addr){
    //1. CPU RAM 范围: $0000 - $1FFF
    if(addr >= 0x0000 && addr <= 0x1FFF){
        // 0x0000-0x07FF 是实际 RAM
        // 0x0800-0x1FFF 是镜像 (Mirrors)，实际上还是读写前 2KB
        // 使用 & 0x07FF 可以把地址限制在 0-2047 之间 为什么 与运算讲解
        return bus->ram[addr & 0x07FF];
    }
    //2. PPU 寄存器范围: $2000 - $3FFF (暂时留空)
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // TODO: return ppu_read_register(addr & 0x0007);
        return 0;
    }
    // 3. 卡带/ROM 范围: $8000 - $FFFF (通常用于 PRG-ROM)
    // 注意：$4020-$7FFF 也属于卡带空间，但通常用于 Mapper 寄存器或 Save RAM
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        // 这里需要读取你的 NesRom 中的数据
        // 这是一个简单的 Mapper 0 (NROM) 实现逻辑示例：
        uint32_t offset = addr - 0x8000;
        
        // 处理 Mapper 0 的镜像：
        // 如果 PRG-ROM 只有 16KB (prg_size == 1)，但 CPU 访问 $C000 以上
        // 它应该读取前半部分的数据
        uint32_t prg_size_bytes = bus->cartridge->header.prg_size * 16 * 1024;

        //使用取模运算处理镜像(对于 16KB ROM，% 16384)
        // 对于 32KB ROM，% 32768 (即原样访问)
        return bus->cartridge->prg_rom[offset % prg_size_bytes];

    
    }

    return 0x00; // 默认返回0
}

void bus_write(Bus* bus,uint16_t addr, uint8_t data){
    //1. CPU RAM 范围: $0000 - $1FFF
    if(addr >= 0x0000 && addr <= 0x1FFF){
        bus->ram[addr & 0x07FF] = data;
    } 
    // 2. PPU 寄存器写入
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // TODO: ppu_write_register(addr & 0x0007, data);
    }
    // 3. 卡带区域写入
    else if (addr >= 0x8000 && addr <= 0xFFFF) {
        // 对于 ROM 来说，"写入"通常意味着配置 Mapper 寄存器
        // 或者是写 Save RAM。
        // 对于 Mapper 0 (NROM)，这里是不可写的。
    }
}
