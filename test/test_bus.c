#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// 注意：因为我们在 test 目录下，引用 code 目录的头文件需要用 "../code/"
#include "../code/bus.h"
#include "../code/ines.h"

// 辅助打印函数：绿色显示通过，红色显示失败
void print_result(const char* test_name, int passed) {
    if (passed) {
        printf("[\033[32mPASS\033[0m] %s\n", test_name);
    } else {
        printf("[\033[31mFAIL\033[0m] %s\n", test_name);
        exit(1); // 遇到错误直接退出
    }
}

int main() {
    printf("=== Starting Bus Tests ===\n");

    // ---------------------------------------------------------
    // 测试 1: 系统 RAM 读写与镜像 (无需插卡带)
    // ---------------------------------------------------------
    Bus bus;
    // 初始化总线，暂时不插卡带 (传 NULL)，测试纯 RAM 功能
    // 前提：你的 bus_read/write 处理了 cartridge 为 NULL 的情况
    bus_init(&bus, NULL);

    // 写入地址 0x0000
    bus_write(&bus, 0x0000, 0xA5); // 0xA5 是一个经典的测试值 (10100101)
    
    // 验证基本读写
    uint8_t val = bus_read(&bus, 0x0000);
    print_result("RAM Read/Write at 0x0000", val == 0xA5);

    // 验证镜像 (Mirroring)
    // NES 的 RAM 只有 2KB (0x0000-0x07FF)，但占据了 0x0000-0x1FFF 的空间
    // 所以 0x0000, 0x0800, 0x1000, 0x1800 应该指向同一个物理内存单元
    uint8_t mirror1 = bus_read(&bus, 0x0800);
    uint8_t mirror2 = bus_read(&bus, 0x1800);
    
    print_result("RAM Mirror at 0x0800", mirror1 == 0xA5);
    print_result("RAM Mirror at 0x1800", mirror2 == 0xA5);

    // 反向测试：写入镜像地址，读取原地址
    bus_write(&bus, 0x1FFF, 0x42); // 0x1FFF 是 RAM 区域的最后一个字节
    uint8_t base_val = bus_read(&bus, 0x07FF); // 0x07FF 是物理 RAM 的最后一个字节
    print_result("RAM Reverse Mirror (Write 0x1FFF, Read 0x07FF)", base_val == 0x42);


    // ---------------------------------------------------------
    // 测试 2: 卡带 (ROM) 读取测试
    // ---------------------------------------------------------
    // 注意：这里假设你在项目根目录运行程序，所以路径是 "test/nestest.nes"
    // 如果你是在 test 目录里运行，路径应改为 "nestest.nes"
    NesRom* rom = load_nes_rom("test/nestest.nes");
    
    if (!rom) {
        printf("[\033[33mSKIP\033[0m] ROM Test (Could not load test/nestest.nes)\n");
        printf("       Please ensure you run this from the project root folder.\n");
    } else {
        // 将卡带插入总线
        bus_init(&bus, rom);

        // nestest.nes 是一个 16KB PRG-ROM 的游戏 (Mapper 0)
        // 这意味着它的数据位于 0xC000 - 0xFFFF
        // 并且在 0x8000 - 0xBFFF 会有一个镜像
        
        // 读取 Reset Vector (复位向量)，这是 CPU 启动后读取的第一对地址
        // 通常指向程序的入口点
        uint8_t lo = bus_read(&bus, 0xFFFC);
        uint8_t hi = bus_read(&bus, 0xFFFD);
        uint16_t reset_vector = (hi << 8) | lo;

        printf("       Reset Vector found: 0x%04X\n", reset_vector);
        // nestest 的入口通常是 C004，但不同版本可能不同，只要读出有效值即可
        print_result("Read Reset Vector from ROM", reset_vector != 0x0000);

        // 验证 ROM 镜像 (Mapper 0 NROM-128 特性)
        // 只有当 PRG 大小为 16KB 时才测试这个
        if (rom->header.prg_size == 1) {
            uint8_t val_low = bus_read(&bus, 0x8000); // 镜像区
            uint8_t val_high = bus_read(&bus, 0xC000); // 实际区
            print_result("Mapper 0 Mirroring (0x8000 == 0xC000)", val_low == val_high);
        }

        // 清理
        free_nes_rom(rom);
    }

    printf("=== All Tests Completed ===\n");
    return 0;
}