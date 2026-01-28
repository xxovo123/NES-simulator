#include <stdio.h>
#include "ines.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <rom_file.nes>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    printf("Attempting to load: %s\n", filename);

    // 调用你写的加载函数
    NesRom* rom = load_nes_rom(filename);

    if (!rom) {
        printf("Failed to load ROM.\n");
        return 1;
    }

    // 打印读取到的信息进行验证
    printf("=== ROM Info ===\n");
    printf("Mapper ID: %d\n", rom->mapper_id);
    printf("Mirroring: %s\n", rom->mirroring ? "Vertical" : "Horizontal");
    printf("Has Trainer: %s\n", rom->has_trainer ? "Yes" : "No");
    
    // 验证 PRG 大小 (Header 中的值 vs 实际计算)
    printf("PRG-ROM Size: %d units (Total %d bytes)\n", 
           rom->header.prg_size, 
           rom->header.prg_size * 16 * 1024);

    // 验证 CHR 大小
    printf("CHR-ROM Size: %d units (Total %d bytes)\n", 
           rom->header.chr_size, 
           rom->header.chr_size * 8 * 1024);

    if (rom->chr_rom == NULL) {
        printf("Note: Using CHR-RAM (CHR-ROM size is 0)\n");
    }

    // 简单验证一下数据是否读进去了 (打印 PRG 的前几个字节)
    if (rom->prg_rom) {
        printf("PRG Hex Dump (first 16 bytes): ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", rom->prg_rom[i]);
        }
        printf("\n");
    }

    // 清理内存
    free_nes_rom(rom);
    printf("ROM freed successfully.\n");

    return 0;
}