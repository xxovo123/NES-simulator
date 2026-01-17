//ines.h
#pragma once

#include<stdio.h>
#include<stdint.h>


typedef struct __attribute__((packed)) INesHeader{
    uint8_t magic[4];// 应该是 "NES" 后跟 0x1A
    uint8_t prg_size;// PRG-ROM 大小，单位 16KB
    uint8_t chr_size;// CHR-ROM 大小，单位 8KB
    uint8_t flags6;// Mapper 低 4 位、镜像方式、电池、Trainer
    uint8_t flags7;//Mapper 高 4 位、VS/Playchoice、NES 2.0 标识
    uint8_t prg_ram_size;//PRG RAM 大小（罕见扩展）
    uint8_t flags9;//电视制式（罕见扩展）
    uint8_t flags10;//电视制式、PRG RAM 存在标志（非官方，极罕见）
    uint8_t padding[5];//填充位（应为 0，但某些工具会在此写入标识，如 "DiskDude\!"）
} INesHeader;


typedef struct NesRom{
    INesHeader header;
    uint8_t* prg_rom;
    uint8_t* chr_rom;
    int mapper_id;
    int mirroring;
    int has_trainer;
    //... 其他运行时状态
} NesRom;


NesRom* load_nes_rom(const char* filename);
void free_nes_rom(NesRom* rom);
int validate_header(const INesHeader* hdr);



