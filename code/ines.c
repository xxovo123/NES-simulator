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
    
}
