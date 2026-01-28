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

NesRom* load_nes_rom(const char* filename){
    FILE*  fp = fopen(filename,"rb");// 复习点：为什么是 "rb"？(二进制读模式)
    if(!fp){
        perror("Failed to open ROM"); // 复习点：标准错误输出
        return NULL;
    }
    //1.读取 Header
    INesHeader header;
    if(fread(&header,sizeof(INesHeader),1,fp)!=1){
        fclose(fp);
        return NULL;
    }

    // 2. 验证 Magic Number
    if(!validate_header(&header)){
        fprintf(stderr,"Invalid NES ROM format\n");
        fclose(fp);
        return NULL;
    }

    // 3. 初始化 NesRom 对象
    NesRom* rom = (NesRom*)malloc(sizeof(NesRom));
    if(!rom){
        fclose(fp);
        return NULL;
    }

    // 拷贝 header 内容 (复习 struct 赋值或 memcpy)
    rom->header = header;
    rom->mapper_id = get_mapper_id(&header);
    rom->mirroring = get_mirroring(&header);
    rom->has_trainer = has_trainer(&header);

    //4. 处理 Trainer (跳过垃圾数据)
    if(rom->has_trainer){
        fseek(fp,512,SEEK_CUR);// 复习点：文件流指针偏移
    }

    // 5. 分配并读取 PRG-ROM (程序代码)
    size_t prg_bytes = header.prg_size * 16 *1024;
    rom->prg_rom = (uint8_t*)malloc(prg_bytes);
    if(!rom->prg_rom){
        free(rom);
        fclose(fp);
        return NULL;
    }
    fread(rom->prg_rom,1,prg_bytes,fp);

    //6. 分配并读取 CHR-ROM (图像数据)
    size_t chr_bytes = header.chr_size * 8 *1024;
    if(chr_bytes > 0){
        rom->chr_rom = (uint8_t*)malloc(chr_bytes);
        if(!rom->chr_rom){
            free(rom->prg_rom);
            free(rom);
            fclose(fp);
            return NULL;
        }
        fread(rom->chr_rom,1,chr_bytes,fp);
    } else {
        rom->chr_rom = NULL; // 使用 CHR-RAM 的情况
    }
    fclose(fp);
    printf("ROM Loaded: PRG=%zu KB, Mapper=%d\n", prg_bytes/1024, rom->mapper_id);
    return rom;
}

void free_nes_rom(NesRom* rom) {
    if (rom) {
        if (rom->prg_rom) free(rom->prg_rom);
        if (rom->chr_rom) free(rom->chr_rom);
        free(rom);
    }
}