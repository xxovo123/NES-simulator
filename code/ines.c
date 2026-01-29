// ines.c
#include "ines.h"
#include <stdlib.h>
#include <string.h>

#define NES_MAGIC_CMP "\x4E\x45\x53\x1A" // "NES\x1A"

int validate_header(const INesHeader* hdr){
    return memcmp(hdr->magic,NES_MAGIC_CMP,4) == 0;
}

int get_mapper_id(const INesHeader* hdr){
    uint8_t low = (hdr->flags6>>4) & 0x0f;
    uint8_t high = (hdr->flags7>>4) & 0x0f;
    return (high<<4) | low;
}

int get_mirroring(const INesHeader* hdr) {
    return (hdr->flags6 & 0x01);// 0 = horizontal, 1 = vertical
}

int has_trainer(const INesHeader* hdr) {
    return ((hdr->flags6>>2) & 0x01);
}


// 内部逻辑：根据 Header 初始化 NesRom 对象（不涉及 IO）
static NesRom* create_nes_rom_struct(const INesHeader* header){ //为什么要用static
    NesRom* rom = (NesRom*)malloc(sizeof(NesRom));
    if(!rom) return NULL;

    rom->header = *header; //如何理解这里的赋值
    rom->mapper_id = get_mapper_id(header);
    rom->mirroring = get_mirroring(header);
    rom->has_trainer = has_trainer(header);
    rom->prg_rom = NULL;
    rom->chr_rom = NULL;
    return rom;
}

// 核心解耦函数：从内存缓冲区解析 ROM
NesRom* load_nes_rom_from_buffer(const uint8_t* data,size_t size){
    if(size < sizeof(INesHeader)) return NULL;

    const INesHeader* header = (const INesHeader*)data; //这里的data看上去包含了整个rom，但是强制类型转换为INesHeader指针，INesHeader后面的内存会怎么样，是否会导致内存泄漏
    if(!validate_header(header)) return NULL;

    NesRom* rom = create_nes_rom_struct(header);
    if(!rom) return NULL;

    size_t offset = sizeof(INesHeader);
    if(rom->has_trainer){
        offset += 512;
    }

    // 分配并拷贝 PRG-ROM
    size_t prg_bytes = header->prg_size * 16 * 1024;
    if(offset + prg_bytes > size){
        /* 错误处理 */
        free(rom);
        return NULL;
    }

    rom->prg_rom = (uint8_t*)malloc(prg_bytes);
    if(!rom->prg_rom){
        free(rom);
        return NULL;
    }
    memcpy(rom->prg_rom,data + offset,prg_bytes);
    offset += prg_bytes;

    // 分配并拷贝 CHR-ROM
    size_t chr_bytes = header->chr_size * 8 *1024;
    if(chr_bytes > 0){
        if (offset + chr_bytes > size){
            /* 错误处理 */
            free(rom);
            free(rom->prg_rom);
            return NULL;
        }
        rom->chr_rom = (uint8_t*)malloc(chr_bytes);
        if(!rom->chr_rom){
            free(rom);
            free(rom->prg_rom);
            return NULL;
        }
        memcpy(rom->chr_rom,data + offset,chr_bytes);
    }

    return rom;

}


NesRom* load_nes_rom(const char* filename){
    FILE*  fp = fopen(filename,"rb");// 复习点：为什么是 "rb"？(二进制读模式)
    if(!fp){
        perror("Failed to open ROM"); // 复习点：标准错误输出
        return NULL;
    }

    //获取文件大小
    fseek(fp,0,SEEK_END);
    long size = ftell(fp);
    fseek(fp,0,SEEK_SET);

    // 读取文件内容到内存
    uint8_t* buffer = (uint8_t*)malloc(size);
    if(!buffer){
        fclose(fp);
        return NULL;
    }
    fread(buffer,1,size,fp);
    fclose(fp);
    // 从内存缓冲区解析 ROM
    NesRom* rom = load_nes_rom_from_buffer(buffer,size);
    free(buffer);
    return rom;
}

void free_nes_rom(NesRom* rom) {
    if (rom) {
        if (rom->prg_rom) free(rom->prg_rom);
        if (rom->chr_rom) free(rom->chr_rom);
        free(rom);
    }
}