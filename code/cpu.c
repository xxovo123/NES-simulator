// cpu.c
#include "cpu.h"

// 1. 定义函数指针类型
// 这两个函数都会返回 1 或 0，代表是否产生了“额外时钟周期”（比如跨页访问）
typedef uint8_t (*OpcodeFunc)(CPU*);
typedef uint8_t (*AddrModeFunc)(CPU*);

// 2. 定义指令结构体 (Instruction)
typedef struct {
    char* name; // 汇编助记符 (用于调试打印，如 "LDA")
    OpcodeFunc operate; //干活的函数 (如 op_lda)
    AddrModeFunc addrmode; //找数据的函数 (如 addr_imm)
    uint8_t cycles; //基础消耗周期
}Instruction;

// --- 寻址模式函数声明 (Addressing Modes) ---
static uint8_t addr_imp(CPU* cpu); // 隐含寻址 (Implied)
static uint8_t addr_acc(CPU* cpu); // 累加器寻址 (Accumulator)
static uint8_t addr_imm(CPU* cpu); // 立即寻址 (Immediate)
static uint8_t addr_zp0(CPU* cpu); // 零页寻址 (Zero Page)
static uint8_t addr_zpx(CPU* cpu); // 零页 X 变址 (Zero Page, X)
static uint8_t addr_zpy(CPU* cpu); // 零页 Y 变址 (Zero Page, Y)
static uint8_t addr_rel(CPU* cpu); // 相对寻址 (Relative)
static uint8_t addr_abs(CPU* cpu); // 绝对寻址 (Absolute)
static uint8_t addr_abx(CPU* cpu); // 绝对 X 变址 (Absolute, X)
static uint8_t addr_aby(CPU* cpu); // 绝对 Y 变址 (Absolute, Y)
static uint8_t addr_ind(CPU* cpu); // 间接寻址 (Indirect)
static uint8_t addr_izx(CPU* cpu); // 间接 X 变址 (Indexed Indirect, X)
static uint8_t addr_izy(CPU* cpu); // 间接 Y 变址 (Indirect Indexed, Y)

// --- 指令操作函数声明 (Opcodes) ---
static uint8_t op_adc(CPU* cpu);
static uint8_t op_and(CPU* cpu);
static uint8_t op_asl(CPU* cpu);
static uint8_t op_bcc(CPU* cpu);
static uint8_t op_bcs(CPU* cpu);
static uint8_t op_beq(CPU* cpu);
static uint8_t op_bit(CPU* cpu);
static uint8_t op_bmi(CPU* cpu);
static uint8_t op_bne(CPU* cpu);
static uint8_t op_bpl(CPU* cpu);
static uint8_t op_brk(CPU* cpu);
static uint8_t op_bvc(CPU* cpu);
static uint8_t op_bvs(CPU* cpu);
static uint8_t op_clc(CPU* cpu);
static uint8_t op_cld(CPU* cpu);
static uint8_t op_cli(CPU* cpu);
static uint8_t op_clv(CPU* cpu);
static uint8_t op_cmp(CPU* cpu);
static uint8_t op_cpx(CPU* cpu);
static uint8_t op_cpy(CPU* cpu);
static uint8_t op_dec(CPU* cpu);
static uint8_t op_dex(CPU* cpu);
static uint8_t op_dey(CPU* cpu);
static uint8_t op_eor(CPU* cpu);
static uint8_t op_inc(CPU* cpu);
static uint8_t op_inx(CPU* cpu);
static uint8_t op_iny(CPU* cpu);
static uint8_t op_jmp(CPU* cpu);
static uint8_t op_jsr(CPU* cpu);
static uint8_t op_lda(CPU* cpu);
static uint8_t op_ldx(CPU* cpu);
static uint8_t op_ldy(CPU* cpu);
static uint8_t op_lsr(CPU* cpu);
static uint8_t op_nop(CPU* cpu);
static uint8_t op_ora(CPU* cpu);
static uint8_t op_pha(CPU* cpu);
static uint8_t op_php(CPU* cpu);
static uint8_t op_pla(CPU* cpu);
static uint8_t op_plp(CPU* cpu);
static uint8_t op_rol(CPU* cpu);
static uint8_t op_ror(CPU* cpu);
static uint8_t op_rti(CPU* cpu);
static uint8_t op_rts(CPU* cpu);
static uint8_t op_sbc(CPU* cpu);
static uint8_t op_sec(CPU* cpu);
static uint8_t op_sed(CPU* cpu);
static uint8_t op_sei(CPU* cpu);
static uint8_t op_sta(CPU* cpu);
static uint8_t op_stx(CPU* cpu);
static uint8_t op_sty(CPU* cpu);
static uint8_t op_tax(CPU* cpu);
static uint8_t op_tay(CPU* cpu);
static uint8_t op_tsx(CPU* cpu);
static uint8_t op_txa(CPU* cpu);
static uint8_t op_txs(CPU* cpu);
static uint8_t op_tya(CPU* cpu);
//非官方指令操作函数
static uint8_t op_jam(CPU* cpu);
static uint8_t op_slo(CPU* cpu);
static uint8_t op_nop(CPU* cpu);
static uint8_t op_anc(CPU* cpu);
static uint8_t op_rla(CPU* cpu);
static uint8_t op_sre(CPU* cpu);
static uint8_t op_alr(CPU* cpu);
static uint8_t op_rra(CPU* cpu);
static uint8_t op_arr(CPU* cpu);
static uint8_t op_sax(CPU* cpu);
static uint8_t op_ane(CPU* cpu);
static uint8_t op_sha(CPU* cpu);
static uint8_t op_tas(CPU* cpu);
static uint8_t op_shy(CPU* cpu);
static uint8_t op_shx(CPU* cpu);
static uint8_t op_lax(CPU* cpu);
static uint8_t op_lxa(CPU* cpu);
static uint8_t op_las(CPU* cpu);
static uint8_t op_dcp(CPU* cpu);
static uint8_t op_sbx(CPU* cpu);
static uint8_t op_isc(CPU* cpu);
static uint8_t op_usbc(CPU* cpu);


Instruction lookup[256] = {
    { "BRK", &op_brk, &addr_imp, 7 },                   // 0x00
    { "ORA", &op_ora, &addr_izx, 6 },                   // 0x01
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x02 (JAM/KIL)
    { "SLO", &op_slo, &addr_izx, 8 },                   // 0x03
    { "NOP", &op_nop, &addr_zp0, 3 },                   // 0x04
    { "ORA", &op_ora, &addr_zp0, 3 },                   // 0x05
    { "ASL", &op_asl, &addr_zp0, 5 },                   // 0x06
    { "SLO", &op_slo, &addr_zp0, 5 },                   // 0x07
    { "PHP", &op_php, &addr_imp, 3 },                   // 0x08
    { "ORA", &op_ora, &addr_imm, 2 },                   // 0x09
    { "ASL", &op_asl, &addr_acc, 2 },                   // 0x0A
    { "ANC", &op_anc, &addr_imm, 2 },                   // 0x0B
    { "NOP", &op_nop, &addr_abs, 4 },                   // 0x0C
    { "ORA", &op_ora, &addr_abs, 4 },                   // 0x0D
    { "ASL", &op_asl, &addr_abs, 6 },                   // 0x0E
    { "SLO", &op_slo, &addr_abs, 6 },                   // 0x0F
    { "BPL", &op_bpl, &addr_rel, 2 },                   // 0x10
    { "ORA", &op_ora, &addr_izy, 5 },                   // 0x11
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x12 (JAM/KIL)
    { "SLO", &op_slo, &addr_izy, 8 },                   // 0x13
    { "NOP", &op_nop, &addr_zpx, 4 },                   // 0x14
    { "ORA", &op_ora, &addr_zpx, 4 },                   // 0x15
    { "ASL", &op_asl, &addr_zpx, 6 },                   // 0x16
    { "SLO", &op_slo, &addr_zpx, 6 },                   // 0x17
    { "CLC", &op_clc, &addr_imp, 2 },                   // 0x18
    { "ORA", &op_ora, &addr_aby, 4 },                   // 0x19
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0x1A
    { "SLO", &op_slo, &addr_aby, 7 },                   // 0x1B
    { "NOP", &op_nop, &addr_abx, 4 },                   // 0x1C
    { "ORA", &op_ora, &addr_abx, 4 },                   // 0x1D
    { "ASL", &op_asl, &addr_abx, 7 },                   // 0x1E
    { "SLO", &op_slo, &addr_abx, 7 },                   // 0x1F
    { "JSR", &op_jsr, &addr_abs, 6 },                   // 0x20
    { "AND", &op_and, &addr_izx, 6 },                   // 0x21
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x22 (JAM/KIL)
    { "RLA", &op_rla, &addr_izx, 8 },                   // 0x23
    { "BIT", &op_bit, &addr_zp0, 3 },                   // 0x24
    { "AND", &op_and, &addr_zp0, 3 },                   // 0x25
    { "ROL", &op_rol, &addr_zp0, 5 },                   // 0x26
    { "RLA", &op_rla, &addr_zp0, 5 },                   // 0x27
    { "PLP", &op_plp, &addr_imp, 4 },                   // 0x28
    { "AND", &op_and, &addr_imm, 2 },                   // 0x29
    { "ROL", &op_rol, &addr_acc, 2 },                   // 0x2A
    { "ANC", &op_anc, &addr_imm, 2 },                   // 0x2B
    { "BIT", &op_bit, &addr_abs, 4 },                   // 0x2C
    { "AND", &op_and, &addr_abs, 4 },                   // 0x2D
    { "ROL", &op_rol, &addr_abs, 6 },                   // 0x2E
    { "RLA", &op_rla, &addr_abs, 6 },                   // 0x2F
    { "BMI", &op_bmi, &addr_rel, 2 },                   // 0x30
    { "AND", &op_and, &addr_izy, 5 },                   // 0x31
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x32 (JAM/KIL)
    { "RLA", &op_rla, &addr_izy, 8 },                   // 0x33
    { "NOP", &op_nop, &addr_zpx, 4 },                   // 0x34
    { "AND", &op_and, &addr_zpx, 4 },                   // 0x35
    { "ROL", &op_rol, &addr_zpx, 6 },                   // 0x36
    { "RLA", &op_rla, &addr_zpx, 6 },                   // 0x37
    { "SEC", &op_sec, &addr_imp, 2 },                   // 0x38
    { "AND", &op_and, &addr_aby, 4 },                   // 0x39
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0x3A
    { "RLA", &op_rla, &addr_aby, 7 },                   // 0x3B
    { "NOP", &op_nop, &addr_abx, 4 },                   // 0x3C
    { "AND", &op_and, &addr_abx, 4 },                   // 0x3D
    { "ROL", &op_rol, &addr_abx, 7 },                   // 0x3E
    { "RLA", &op_rla, &addr_abx, 7 },                   // 0x3F
    { "RTI", &op_rti, &addr_imp, 6 },                   // 0x40
    { "EOR", &op_eor, &addr_izx, 6 },                   // 0x41
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x42 (JAM/KIL)
    { "SRE", &op_sre, &addr_izx, 8 },                   // 0x43
    { "NOP", &op_nop, &addr_zp0, 3 },                   // 0x44
    { "EOR", &op_eor, &addr_zp0, 3 },                   // 0x45
    { "LSR", &op_lsr, &addr_zp0, 5 },                   // 0x46
    { "SRE", &op_sre, &addr_zp0, 5 },                   // 0x47
    { "PHA", &op_pha, &addr_imp, 3 },                   // 0x48
    { "EOR", &op_eor, &addr_imm, 2 },                   // 0x49
    { "LSR", &op_lsr, &addr_acc, 2 },                   // 0x4A
    { "ALR", &op_alr, &addr_imm, 2 },                   // 0x4B
    { "JMP", &op_jmp, &addr_abs, 3 },                   // 0x4C
    { "EOR", &op_eor, &addr_abs, 4 },                   // 0x4D
    { "LSR", &op_lsr, &addr_abs, 6 },                   // 0x4E
    { "SRE", &op_sre, &addr_abs, 6 },                   // 0x4F
    { "BVC", &op_bvc, &addr_rel, 2 },                   // 0x50
    { "EOR", &op_eor, &addr_izy, 5 },                   // 0x51
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x52 (JAM/KIL)
    { "SRE", &op_sre, &addr_izy, 8 },                   // 0x53
    { "NOP", &op_nop, &addr_zpx, 4 },                   // 0x54
    { "EOR", &op_eor, &addr_zpx, 4 },                   // 0x55
    { "LSR", &op_lsr, &addr_zpx, 6 },                   // 0x56
    { "SRE", &op_sre, &addr_zpx, 6 },                   // 0x57
    { "CLI", &op_cli, &addr_imp, 2 },                   // 0x58
    { "EOR", &op_eor, &addr_aby, 4 },                   // 0x59
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0x5A
    { "SRE", &op_sre, &addr_aby, 7 },                   // 0x5B
    { "NOP", &op_nop, &addr_abx, 4 },                   // 0x5C
    { "EOR", &op_eor, &addr_abx, 4 },                   // 0x5D
    { "LSR", &op_lsr, &addr_abx, 7 },                   // 0x5E
    { "SRE", &op_sre, &addr_abx, 7 },                   // 0x5F
    { "RTS", &op_rts, &addr_imp, 6 },                   // 0x60
    { "ADC", &op_adc, &addr_izx, 6 },                   // 0x61
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x62 (JAM/KIL)
    { "RRA", &op_rra, &addr_izx, 8 },                   // 0x63
    { "NOP", &op_nop, &addr_zp0, 3 },                   // 0x64
    { "ADC", &op_adc, &addr_zp0, 3 },                   // 0x65
    { "ROR", &op_ror, &addr_zp0, 5 },                   // 0x66
    { "RRA", &op_rra, &addr_zp0, 5 },                   // 0x67
    { "PLA", &op_pla, &addr_imp, 4 },                   // 0x68
    { "ADC", &op_adc, &addr_imm, 2 },                   // 0x69
    { "ROR", &op_ror, &addr_acc, 2 },                   // 0x6A
    { "ARR", &op_arr, &addr_imm, 2 },                   // 0x6B
    { "JMP", &op_jmp, &addr_ind, 5 },                   // 0x6C
    { "ADC", &op_adc, &addr_abs, 4 },                   // 0x6D
    { "ROR", &op_ror, &addr_abs, 6 },                   // 0x6E
    { "RRA", &op_rra, &addr_abs, 6 },                   // 0x6F
    { "BVS", &op_bvs, &addr_rel, 2 },                   // 0x70
    { "ADC", &op_adc, &addr_izy, 5 },                   // 0x71
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x72 (JAM/KIL)
    { "RRA", &op_rra, &addr_izy, 8 },                   // 0x73
    { "NOP", &op_nop, &addr_zpx, 4 },                   // 0x74
    { "ADC", &op_adc, &addr_zpx, 4 },                   // 0x75
    { "ROR", &op_ror, &addr_zpx, 6 },                   // 0x76
    { "RRA", &op_rra, &addr_zpx, 6 },                   // 0x77
    { "SEI", &op_sei, &addr_imp, 2 },                   // 0x78
    { "ADC", &op_adc, &addr_aby, 4 },                   // 0x79
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0x7A
    { "RRA", &op_rra, &addr_aby, 7 },                   // 0x7B
    { "NOP", &op_nop, &addr_abx, 4 },                   // 0x7C
    { "ADC", &op_adc, &addr_abx, 4 },                   // 0x7D
    { "ROR", &op_ror, &addr_abx, 7 },                   // 0x7E
    { "RRA", &op_rra, &addr_abx, 7 },                   // 0x7F
    { "NOP", &op_nop, &addr_imm, 2 },                   // 0x80
    { "STA", &op_sta, &addr_izx, 6 },                   // 0x81
    { "NOP", &op_nop, &addr_imm, 2 },                   // 0x82
    { "SAX", &op_sax, &addr_izx, 6 },                   // 0x83
    { "STY", &op_sty, &addr_zp0, 3 },                   // 0x84
    { "STA", &op_sta, &addr_zp0, 3 },                   // 0x85
    { "STX", &op_stx, &addr_zp0, 3 },                   // 0x86
    { "SAX", &op_sax, &addr_zp0, 3 },                   // 0x87
    { "DEY", &op_dey, &addr_imp, 2 },                   // 0x88
    { "NOP", &op_nop, &addr_imm, 2 },                   // 0x89
    { "TXA", &op_txa, &addr_imp, 2 },                   // 0x8A
    { "ANE", &op_ane, &addr_imm, 2 },                   // 0x8B
    { "STY", &op_sty, &addr_abs, 4 },                   // 0x8C
    { "STA", &op_sta, &addr_abs, 4 },                   // 0x8D
    { "STX", &op_stx, &addr_abs, 4 },                   // 0x8E
    { "SAX", &op_sax, &addr_abs, 4 },                   // 0x8F
    { "BCC", &op_bcc, &addr_rel, 2 },                   // 0x90
    { "STA", &op_sta, &addr_izy, 6 },                   // 0x91
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0x92 (JAM/KIL)
    { "SHA", &op_sha, &addr_izy, 6 },                   // 0x93
    { "STY", &op_sty, &addr_zpx, 4 },                   // 0x94
    { "STA", &op_sta, &addr_zpx, 4 },                   // 0x95
    { "STX", &op_stx, &addr_zpy, 4 },                   // 0x96
    { "SAX", &op_sax, &addr_zpy, 4 },                   // 0x97
    { "TYA", &op_tya, &addr_imp, 2 },                   // 0x98
    { "STA", &op_sta, &addr_aby, 5 },                   // 0x99
    { "TXS", &op_txs, &addr_imp, 2 },                   // 0x9A
    { "TAS", &op_tas, &addr_aby, 5 },                   // 0x9B
    { "SHY", &op_shy, &addr_abx, 5 },                   // 0x9C
    { "STA", &op_sta, &addr_abx, 5 },                   // 0x9D
    { "SHX", &op_shx, &addr_aby, 5 },                   // 0x9E
    { "SHA", &op_sha, &addr_aby, 5 },                   // 0x9F
    { "LDY", &op_ldy, &addr_imm, 2 },                   // 0xA0
    { "LDA", &op_lda, &addr_izx, 6 },                   // 0xA1
    { "LDX", &op_ldx, &addr_imm, 2 },                   // 0xA2
    { "LAX", &op_lax, &addr_izx, 6 },                   // 0xA3
    { "LDY", &op_ldy, &addr_zp0, 3 },                   // 0xA4
    { "LDA", &op_lda, &addr_zp0, 3 },                   // 0xA5
    { "LDX", &op_ldx, &addr_zp0, 3 },                   // 0xA6
    { "LAX", &op_lax, &addr_zp0, 3 },                   // 0xA7
    { "TAY", &op_tay, &addr_imp, 2 },                   // 0xA8
    { "LDA", &op_lda, &addr_imm, 2 },                   // 0xA9
    { "TAX", &op_tax, &addr_imp, 2 },                   // 0xAA
    { "LXA", &op_lxa, &addr_imm, 2 },                   // 0xAB
    { "LDY", &op_ldy, &addr_abs, 4 },                   // 0xAC
    { "LDA", &op_lda, &addr_abs, 4 },                   // 0xAD
    { "LDX", &op_ldx, &addr_abs, 4 },                   // 0xAE
    { "LAX", &op_lax, &addr_abs, 4 },                   // 0xAF
    { "BCS", &op_bcs, &addr_rel, 2 },                   // 0xB0
    { "LDA", &op_lda, &addr_izy, 5 },                   // 0xB1
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0xB2 (JAM/KIL)
    { "LAX", &op_lax, &addr_izy, 5 },                   // 0xB3
    { "LDY", &op_ldy, &addr_zpx, 4 },                   // 0xB4
    { "LDA", &op_lda, &addr_zpx, 4 },                   // 0xB5
    { "LDX", &op_ldx, &addr_zpy, 4 },                   // 0xB6
    { "LAX", &op_lax, &addr_zpy, 4 },                   // 0xB7
    { "CLV", &op_clv, &addr_imp, 2 },                   // 0xB8
    { "LDA", &op_lda, &addr_aby, 4 },                   // 0xB9
    { "TSX", &op_tsx, &addr_imp, 2 },                   // 0xBA
    { "LAS", &op_las, &addr_aby, 4 },                   // 0xBB
    { "LDY", &op_ldy, &addr_abx, 4 },                   // 0xBC
    { "LDA", &op_lda, &addr_abx, 4 },                   // 0xBD
    { "LDX", &op_ldx, &addr_aby, 4 },                   // 0xBE
    { "LAX", &op_lax, &addr_aby, 4 },                   // 0xBF
    { "CPY", &op_cpy, &addr_imm, 2 },                   // 0xC0
    { "CMP", &op_cmp, &addr_izx, 6 },                   // 0xC1
    { "NOP", &op_nop, &addr_imm, 2 },                   // 0xC2
    { "DCP", &op_dcp, &addr_izx, 8 },                   // 0xC3
    { "CPY", &op_cpy, &addr_zp0, 3 },                   // 0xC4
    { "CMP", &op_cmp, &addr_zp0, 3 },                   // 0xC5
    { "DEC", &op_dec, &addr_zp0, 5 },                   // 0xC6
    { "DCP", &op_dcp, &addr_zp0, 5 },                   // 0xC7
    { "INY", &op_iny, &addr_imp, 2 },                   // 0xC8
    { "CMP", &op_cmp, &addr_imm, 2 },                   // 0xC9
    { "DEX", &op_dex, &addr_imp, 2 },                   // 0xCA
    { "SBX", &op_sbx, &addr_imm, 2 },                   // 0xCB
    { "CPY", &op_cpy, &addr_abs, 4 },                   // 0xCC
    { "CMP", &op_cmp, &addr_abs, 4 },                   // 0xCD
    { "DEC", &op_dec, &addr_abs, 6 },                   // 0xCE
    { "DCP", &op_dcp, &addr_abs, 6 },                   // 0xCF
    { "BNE", &op_bne, &addr_rel, 2 },                   // 0xD0
    { "CMP", &op_cmp, &addr_izy, 5 },                   // 0xD1
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0xD2 (JAM/KIL)
    { "DCP", &op_dcp, &addr_izy, 8 },                   // 0xD3
    { "NOP", &op_nop, &addr_zpx, 4 },                   // 0xD4
    { "CMP", &op_cmp, &addr_zpx, 4 },                   // 0xD5
    { "DEC", &op_dec, &addr_zpx, 6 },                   // 0xD6
    { "DCP", &op_dcp, &addr_zpx, 6 },                   // 0xD7
    { "CLD", &op_cld, &addr_imp, 2 },                   // 0xD8
    { "CMP", &op_cmp, &addr_aby, 4 },                   // 0xD9
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0xDA
    { "DCP", &op_dcp, &addr_aby, 7 },                   // 0xDB
    { "NOP", &op_nop, &addr_abx, 4 },                   // 0xDC
    { "CMP", &op_cmp, &addr_abx, 4 },                   // 0xDD
    { "DEC", &op_dec, &addr_abx, 7 },                   // 0xDE
    { "DCP", &op_dcp, &addr_abx, 7 },                   // 0xDF
    { "CPX", &op_cpx, &addr_imm, 2 },                   // 0xE0
    { "SBC", &op_sbc, &addr_izx, 6 },                   // 0xE1
    { "NOP", &op_nop, &addr_imm, 2 },                   // 0xE2
    { "ISC", &op_isc, &addr_izx, 8 },                   // 0xE3
    { "CPX", &op_cpx, &addr_zp0, 3 },                   // 0xE4
    { "SBC", &op_sbc, &addr_zp0, 3 },                   // 0xE5
    { "INC", &op_inc, &addr_zp0, 5 },                   // 0xE6
    { "ISC", &op_isc, &addr_zp0, 5 },                   // 0xE7
    { "INX", &op_inx, &addr_imp, 2 },                   // 0xE8
    { "SBC", &op_sbc, &addr_imm, 2 },                   // 0xE9
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0xEA
    { "USBC", &op_usbc, &addr_imm, 2 },                 // 0xEB
    { "CPX", &op_cpx, &addr_abs, 4 },                   // 0xEC
    { "SBC", &op_sbc, &addr_abs, 4 },                   // 0xED
    { "INC", &op_inc, &addr_abs, 6 },                   // 0xEE
    { "ISC", &op_isc, &addr_abs, 6 },                   // 0xEF
    { "BEQ", &op_beq, &addr_rel, 2 },                   // 0xF0
    { "SBC", &op_sbc, &addr_izy, 5 },                   // 0xF1
    { "JAM", &op_jam, &addr_imp, 0 },                 // 0xF2 (JAM/KIL)
    { "ISC", &op_isc, &addr_izy, 8 },                   // 0xF3
    { "NOP", &op_nop, &addr_zpx, 4 },                   // 0xF4
    { "SBC", &op_sbc, &addr_zpx, 4 },                   // 0xF5
    { "INC", &op_inc, &addr_zpx, 6 },                   // 0xF6
    { "ISC", &op_isc, &addr_zpx, 6 },                   // 0xF7
    { "SED", &op_sed, &addr_imp, 2 },                   // 0xF8
    { "SBC", &op_sbc, &addr_aby, 4 },                   // 0xF9
    { "NOP", &op_nop, &addr_imp, 2 },                   // 0xFA
    { "ISC", &op_isc, &addr_aby, 7 },                   // 0xFB
    { "NOP", &op_nop, &addr_abx, 4 },                   // 0xFC
    { "SBC", &op_sbc, &addr_abx, 4 },                   // 0xFD
    { "INC", &op_inc, &addr_abx, 7 },                   // 0xFE
    { "ISC", &op_isc, &addr_abx, 7 },                   // 0xFF
};


// --- 基础工具函数 ---
// 封装总线读取，方便后续添加调试或时钟逻辑
static uint8_t cpu_read(CPU* cpu, uint16_t addr){
    return bus_read(cpu->bus, addr);
}

// 封装总线写入
void cpu_write(CPU* cpu, uint16_t addr, uint8_t data) {
    bus_write(cpu->bus, addr, data);
}

// 获取标志位状态 (返回 1 或 0)
static uint8_t get_flag(CPU* cpu, uint8_t f){
    return (cpu->status & f) > 0 ? 1:0; 
}

// 设置标志位状态 (v 非 0 则置位，v 为 0 则清除)
static void set_flag(CPU* cpu, uint8_t f, uint8_t v){
    if(v){
        cpu->status |= f;// 逻辑或：置位
    } else {
        cpu->status &= ~f;// 逻辑与非：清除
    }
}

// 根据当前寻址模式计算出的地址，读取数据到 fetched_data
uint8_t fetch(CPU* cpu){
    // 只有非隐含寻址模式才需要去内存取数据
    // 注意：这里需要你后续填充 lookup 表，目前先写好逻辑
    // 假设 lookup 表名为 lookup
    if(lookup[cpu->opcode].addrmode != &addr_imp &&
       lookup[cpu->opcode].addrmode != &addr_acc){
        cpu->fetched_data = cpu_read(cpu, cpu->addr_abs);
    }
    return cpu->fetched_data;
}


//cpu寻址模式的实现
static uint8_t addr_imp(CPU* cpu){
    return 0;
}

static uint8_t addr_acc(CPU* cpu){
    cpu->fetched_data = cpu->a;
    return 0;
}

static uint8_t addr_imm(CPU* cpu){
    cpu->addr_abs = cpu->pc++;
    return 0;
}

static uint8_t addr_zp0(CPU* cpu){
    cpu->addr_abs = cpu_read(cpu, cpu->pc++);
    cpu->addr_abs &= 0x00FF; // 确保高位为 0
    return 0;
}
static uint8_t addr_zpx(CPU* cpu){
    cpu->addr_abs = cpu_read(cpu,cpu->pc++) + cpu->x;
    cpu->addr_abs &= 0x00FF; // 强制限制在零页，溢出回卷
    return 0;
}

static uint8_t addr_zpy(CPU* cpu){
    cpu->addr_abs = cpu_read(cpu,cpu->pc++) + cpu->y;
    cpu->addr_abs &= 0x00FF; // 强制限制在零页，溢出回卷
    return 0;
}

static uint8_t addr_abs(CPU* cpu){
    uint16_t lo = cpu_read(cpu,cpu->pc++);
    uint16_t hi = cpu_read(cpu,cpu->pc++);
    cpu->addr_abs = (hi << 8) | lo;
    return 0;
}

static uint8_t addr_abx(CPU* cpu){
    uint16_t lo = cpu_read(cpu,cpu->pc++);
    uint16_t hi = cpu_read(cpu,cpu->pc++);
    cpu->addr_abs = ((hi << 8) | lo) + cpu->x;
    if((cpu->addr_abs & 0xFF00) != (hi<<8)){
        return 1;
    }
    return 0;
}


static uint8_t addr_aby(CPU* cpu){
    uint16_t lo = cpu_read(cpu,cpu->pc++);
    uint16_t hi = cpu_read(cpu,cpu->pc++);
    cpu->addr_abs = ((hi << 8) | lo) + cpu->y;
    if((cpu->addr_abs & 0xFF00) != (hi<<8)){
        return 1;
    }
    return 0;
}

static uint8_t addr_rel(CPU* cpu){
    cpu->addr_rel = cpu_read(cpu,cpu->pc++);
    // 如果读取的是负数（最高位为1），由于 addr_rel 是 uint16_t，
    // 需要通过按位或操作将其扩展为 16 位有符号数的效果（即高 8 位全为 1）。
    if(cpu->addr_rel & 0x80){
        cpu->addr_rel |= 0xFF00;
    }
    return 0;
}

static uint8_t addr_ind(CPU* cpu){
    uint16_t ptr_lo = cpu_read(cpu,cpu->pc++);
    uint16_t ptr_hi = cpu_read(cpu,cpu->pc++);
    uint16_t ptr = ptr_lo | (ptr_hi << 8);
    if (ptr_lo == 0x00FF){
        // 如果指针在页面边界（如 $00FF），读取低位在 $00FF，高位会错误地从 $0000 读取，而不是 $0100
        cpu->addr_abs = (cpu_read(cpu,ptr & 0xFF00)<<8) | cpu_read(cpu,ptr);
    } else {
        // 正常情况，直接读取指针指向的地址
        cpu->addr_abs = (cpu_read(cpu,ptr+1)<<8) | cpu_read(cpu,ptr);
    }
}

static uint8_t addr_izx(CPU* cpu){
    uint16_t t = cpu_read(cpu,cpu->pc++);
    // 强制在零页内回卷：(t + X) & 0xFF
    uint16_t lo = cpu_read(cpu,(t + cpu->x) & 0x00FF);
    uint16_t hi = cpu_read(cpu,(t + cpu->x + 1) & 0x00FF);
    cpu->addr_abs = (hi << 8) | lo;
    return 0;
}

static uint8_t addr_izy(CPU* cpu){
    uint16_t t = cpu_read(cpu, cpu->pc++);

    // 从零页 t 处读取 16 位指针
    uint16_t lo = cpu_read(cpu, t & 0x00FF);
    uint16_t hi = cpu_read(cpu, (t + 1) & 0x00FF);

    cpu->addr_abs = (hi << 8) | lo;
    cpu->addr_abs += cpu->y; // 加上 Y 偏移

    // 检查跨页
    if ((cpu->addr_abs & 0xFF00) != (hi << 8)) {
        return 1;
    }
    return 0;
}


// --- 指令实现函数 ---
// LDA: Load Accumulator
static uint8_t op_lda(CPU* cpu){
    // 步骤 1: 取数
    // fetch() 函数会自动根据之前的寻址模式（比如 Immediate 或 Absolute），
    // 把数据读取到 cpu->fetched_data 中。
    // 这对应图片里的 "Loads a byte of memory"。
    fetch(cpu);

    // 步骤 2: 执行操作
    // 对应图片里的 "into the accumulator" (A = M)
    cpu->a = cpu->fetched_data;

    // 步骤 3: 更新标志位
    // 对应表格里的 "Z: Set if A = 0"
    set_flag(cpu, Z, cpu->a == 0x00);
    // 对应表格里的 "N: Set if bit 7 of A is 1"
    set_flag(cpu, N, cpu->a & 0x80);

    // 步骤 4: 处理时钟周期
    // 图片最下方的表格里，很多模式写着 "+1 if page crossed"。
    // 返回 1 告诉模拟器："如果寻址时发生了跨页，允许增加这个额外周期"。
    return 1;
}

// LDX: Load X Register
static uint8_t op_ldx(CPU* cpu){
    // 步骤 1: 取数
    fetch(cpu);
    // 步骤 2: 执行操作
    // 对应图片里的 "into the X register" (X = M)
    cpu->x = cpu->fetched_data;
    // 步骤 3: 更新标志位
    // 对应表格里的 "Z: Set if X = 0"
    set_flag(cpu, Z, cpu->x == 0x00);
    // 对应表格里的 "N: Set if bit 7 of X is 1"
    set_flag(cpu, N, cpu->x & 0x80);
    // 步骤 4: 处理时钟周期
    // 图片最下方的表格里，很多模式写着 "+1 if page crossed"。
    // 返回 1 告诉模拟器："如果寻址时发生了跨页，允许增加这个额外周期"。
    return 1;
}

//LDY: Load Y Register
static uint8_t op_ldy(CPU* cpu){
    // 步骤 1: 取数
    fetch(cpu);
    // 步骤 2: 执行操作
    // 对应图片里的 "into the Y register" (Y = M)
    cpu->y = cpu->fetched_data;
    // 步骤 3: 更新标志位
    // 对应表格里的 "Z: Set if Y = 0"
    set_flag(cpu, Z, cpu->y == 0x00);
    // 对应表格里的 "N: Set if bit 7 of Y is 1"
    set_flag(cpu, N, cpu->y & 0x80);
    // 步骤 4: 处理时钟周期
    // 图片最下方的表格里，很多模式写着 "+1 if page crossed"。
    // 返回 1 告诉模拟器："如果寻址时发生了跨页，允许增加这个额外周期"。
    return 1;
}

// STA: Store Accumulator
static uint8_t op_sta(CPU* cpu){
    // 步骤 1: 不需要 fetch()
    // 因为我们要把寄存器的数据写出去，而不是读进来。
    // 目标地址 cpu->addr_abs 已经在之前的寻址函数(addrmode)里算好了。
    // 步骤 2: 执行操作
    // 对应图片里的 "stores the accumulator in memory" (M = A)
    cpu_write(cpu,cpu->addr_abs,cpu->a);
    // 步骤 3: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // STA 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//STX: Store X Register
static uint8_t op_stx(CPU* cpu){
    // 步骤 1: 不需要 fetch()
    // 因为我们要把寄存器的数据写出去，而不是读进来。
    // 目标地址 cpu->addr_abs 已经在之前的寻址函数(addrmode)里算好了。
    // 步骤 2: 执行操作
    // 对应图片里的 "stores the X register in memory" (M = X)
    cpu_write(cpu,cpu->addr_abs,cpu->x);
    // 步骤 3: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // STX 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//STY: Store Y Register
static uint8_t op_sty(CPU* cpu){
    // 步骤 1: 不需要 fetch()
    // 因为我们要把寄存器的数据写出去，而不是读进来。
    // 目标地址 cpu->addr_abs 已经在之前的寻址函数(addrmode)里算好了。
    // 步骤 2: 执行操作
    // 对应图片里的 "stores the Y register in memory" (M = Y)
    cpu_write(cpu,cpu->addr_abs,cpu->y);
    // 步骤 3: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // STY 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//TAX: Transfer Accumulator to X Register
static uint8_t op_tax(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "transfers the accumulator to the X register" (X = A)
    cpu->x = cpu->a;
    // 步骤 2: 更新标志位
    // 对应表格里的 "Z: Set if X = 0"
    set_flag(cpu, Z, cpu->x == 0x00);
    // 对应表格里的 "N: Set if bit 7 of X is 1"
    set_flag(cpu, N, cpu->x & 0x80);
    // 步骤 3: 处理时钟周期
    return 0;
}

//TAY: Transfer Accumulator to Y Register
static uint8_t op_tay(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "transfers the accumulator to the Y register" (Y = A)
    cpu->y = cpu->a;
    // 步骤 2: 更新标志位
    // 对应表格里的 "Z: Set if Y = 0"
    set_flag(cpu, Z, cpu->y == 0x00);
    // 对应表格里的 "N: Set if bit 7 of Y is 1"
    set_flag(cpu, N, cpu->y & 0x80);
    // 步骤 3: 处理时钟周期
    return 0;
}

//TSX: Transfer Stack Pointer to X Register
static uint8_t op_tsx(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "transfers the stack pointer to the X register" (X = SP)
    cpu->x = cpu->stkp;
    // 步骤 2: 更新标志位
    // 对应表格里的 "Z: Set if X = 0"
    set_flag(cpu, Z, cpu->x == 0x00);
    // 对应表格里的 "N: Set if bit 7 of X is 1"
    set_flag(cpu, N, cpu->x & 0x80);
    // 步骤 3: 处理时钟周期
    return 0;
}

//TXA: Transfer X Register to Accumulator
static uint8_t op_txa(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "transfers the X register to the accumulator" (A = X)
    cpu->a = cpu->x;
    // 步骤 2: 更新标志位
    // 对应表格里的 "Z: Set if A = 0"
    set_flag(cpu, Z, cpu->a == 0x00);
    // 对应表格里的 "N: Set if bit 7 of A is 1"
    set_flag(cpu, N, cpu->a & 0x80);
    // 步骤 3: 处理时钟周期
    return 0;
}

//TXS: Transfer X Register to Stack Pointer
static uint8_t op_txs(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "transfers the X register to the stack pointer" (SP = X)
    cpu->stkp = cpu->x;
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // TXS 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//TYA: Transfer Y Register to Accumulator
static uint8_t op_tya(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "transfers the Y register to the accumulator" (A = Y)
    cpu->a = cpu->y;
    // 步骤 2: 更新标志位
    // 对应表格里的 "Z: Set if A = 0"
    set_flag(cpu, Z, cpu->a == 0x00);
    // 对应表格里的 "N: Set if bit 7 of A is 1"
    set_flag(cpu, N, cpu->a & 0x80);
    // 步骤 3: 处理时钟周期
    return 0;
}

//CLC: Clear Carry Flag
static uint8_t op_clc(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "clears the carry flag" (C = 0)
    set_flag(cpu, C, 0);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // CLC 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//CLD: Clear Decimal Mode Flag
static uint8_t op_cld(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "clears the decimal mode flag" (D = 0)
    set_flag(cpu, D, 0);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // CLD 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//CLI: Clear Interrupt Disable Flag
static uint8_t op_cli(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "clears the interrupt disable flag" (I = 0)
    set_flag(cpu, I, 0);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // CLI 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}


//CLV: Clear Overflow Flag
static uint8_t op_clv(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "clears the overflow flag" (V = 0)
    set_flag(cpu, V, 0);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // CLV 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//SEC: Set Carry Flag
static uint8_t op_sec(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "sets the carry flag" (C = 1)
    set_flag(cpu, C, 1);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // SEC 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//SED: Set Decimal Mode Flag
static uint8_t op_sed(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "sets the decimal mode flag" (D = 1)
    set_flag(cpu, D, 1);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // SED 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

//SEI: Set Interrupt Disable Flag
static uint8_t op_sei(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "sets the interrupt disable flag" (I = 1)
    set_flag(cpu, I, 1);
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // SEI 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

static uint8_t op_nop(CPU* cpu){
    // 步骤 1: 执行操作
    // 对应图片里的 "does nothing" (NOP)
    // 没有改变任何寄存器或内存的操作，所以不需要执行任何操作。
    // 步骤 2: 更新标志位
    // 没有影响标志位的操作，所以不需要更新
    // 图片里没有 "(+1 if page crossed)"。
    // 这意味着即使 addr_abx() 因为跨页返回了 1，
    // NOP 指令也不允许增加额外周期（写入操作必须稳定，不能贪快）。
    // 所以强制返回 0。
    return 0;
}

// AND: Logical AND with Accumulator
static uint8_t op_and(CPU* cpu){
    // 步骤 1: 取数
    // 对应图片 Description: "using the contents of a byte of memory"
    fetch(cpu);

    // 步骤 2: 执行运算
    // 对应图片 Operation: "A,Z,N = A & M"
    // C语言中按位与操作符是 '&'
    cpu->a = cpu->a & cpu->fetched_data;

    // 步骤 3: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if A = 0"
    set_flag(cpu, Z, cpu->a == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, cpu->a & 0x80);

    // 步骤 4: 时钟周期
    // 对应图片 Cycles 表格: 很多模式都有 "(+1 if page crossed)"
    // 这意味着如果寻址时跨页了，AND 指令允许系统增加 1 个周期。
    return 1;
}

//ORA: Logical OR with Accumulator
static uint8_t op_ora(CPU* cpu){
    // 步骤 1: 取数
    // 对应图片 Description: "using the contents of a byte of memory"
    fetch(cpu);

    // 步骤 2: 执行运算
    // 对应图片 Operation: "A,Z,N = A | M"
    // C语言中按位或操作符是 '|'
    cpu->a = cpu->a | cpu->fetched_data;

    // 步骤 3: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if A = 0"
    set_flag(cpu, Z, cpu->a == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, cpu->a & 0x80);

    // 步骤 4: 时钟周期
    // 对应图片 Cycles 表格: 很多模式都有 "(+1 if page crossed)"
    // 这意味着如果寻址时跨页了，ORA 指令允许系统增加 1 个周期。
    return 1;
}

//EOR: Exclusive OR with Accumulator
static uint8_t op_eor(CPU* cpu){
    // 步骤 1: 取数
    // 对应图片 Description: "using the contents of a byte of memory"
    fetch(cpu);

    // 步骤 2: 执行运算
    // 对应图片 Operation: "A,Z,N = A ^ M"
    // C语言中按位异或操作符是 '^'
    cpu->a = cpu->a ^ cpu->fetched_data;

    // 步骤 3: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if A = 0"
    set_flag(cpu, Z, cpu->a == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, cpu->a & 0x80);

    // 步骤 4: 时钟周期
    // 对应图片 Cycles 表格: 很多模式都有 "(+1 if page crossed)"
    // 这意味着如果寻址时跨页了，EOR 指令允许系统增加 1 个周期。
    return 1;

}

// CMP: Compare Accumulator with Memory
/*
对于 CMP (比较 A), CPX (比较 X), CPY (比较 Y) 这三个指令，请记住这个黄金法则：

C (Carry)：不需要做减法看结果，直接看 寄存器 >= 内存数 吗？是就是 1，否则 0。

Z (Zero)：直接看 寄存器 == 内存数 吗？

N (Negative)：只有这个才需要看 (寄存器 - 内存数) 结果的最高位。
*/
static uint8_t op_cmp(CPU* cpu){
    fetch(cpu);

    // 【修正1】：使用 uint16_t 来容纳结果，避免 int8_t 的溢出误判
    // 逻辑上相当于做减法： A - M
    uint16_t temp = (uint16_t)cpu->a - (uint16_t)cpu->fetched_data;

    // 【修正2】：C (Carry) 标志位
    // 官方定义：Set if A >= M
    // 直接用原始数值比较最安全，绝对不会错
    set_flag(cpu, C, cpu->a >= cpu->fetched_data);

    // Z (Zero) 标志位
    // Set if A == M (意味着减法结果低8位为0)
    set_flag(cpu, Z, (temp & 0x00FF) == 0x0000);

    // N (Negative) 标志位
    // 取结果的第 7 位 (bit 7)
    set_flag(cpu, N, temp & 0x0080);

    return 1;
}

//CPX: Compare X Register with Memory
static uint8_t op_cpx(CPU* cpu){
    fetch(cpu);

    // 【修正1】：使用 uint16_t 来容纳结果，避免 int8_t 的溢出误判
    // 逻辑上相当于做减法： X - M
    uint16_t temp = (uint16_t)cpu->x - (uint16_t)cpu->fetched_data;

    // 【修正2】：C (Carry) 标志位
    // 官方定义：Set if X >= M
    // 直接用原始数值比较最安全，绝对不会错
    set_flag(cpu, C, cpu->x >= cpu->fetched_data);

    // Z (Zero) 标志位
    // Set if X == M (意味着减法结果低8位为0)
    set_flag(cpu, Z, (temp & 0x00FF) == 0x0000);

    // N (Negative) 标志位
    // 取结果的第 7 位 (bit 7)
    set_flag(cpu, N, temp & 0x0080);

    return 1;
}

//CPY: Compare Y Register with Memory
static uint8_t op_cpy(CPU* cpu){
    fetch(cpu);

    // 【修正1】：使用 uint16_t 来容纳结果，避免 int8_t 的溢出误判
    // 逻辑上相当于做减法： Y - M
    uint16_t temp = (uint16_t)cpu->y - (uint16_t)cpu->fetched_data;

    // 【修正2】：C (Carry) 标志位
    // 官方定义：Set if X >= M
    // 直接用原始数值比较最安全，绝对不会错
    set_flag(cpu, C, cpu->y >= cpu->fetched_data);

    // Z (Zero) 标志位
    // Set if X == M (意味着减法结果低8位为0)
    set_flag(cpu, Z, (temp & 0x00FF) == 0x0000);

    // N (Negative) 标志位
    // 取结果的第 7 位 (bit 7)
    set_flag(cpu, N, temp & 0x0080);

    return 1;
}

// BIT: Bit Test
static uint8_t op_bit(CPU* cpu){
    // 步骤 1: 取数 (M)
    fetch(cpu); 

    // 步骤 2: 执行 AND 运算（为了设置 Z）
    // 注意：结果存临时变量，不要修改 cpu->a
    uint8_t temp = cpu->a & cpu->fetched_data;

    // 步骤 3: 设置 Z (Zero) 标志
    // 规则：如果 (A & M) == 0，则 Z = 1
    set_flag(cpu, Z, temp == 0x00);

    // 步骤 4: 设置 N (Negative) 标志
    // 规则：直接取内存数据 (fetched_data) 的第 7 位
    // 注意：这里是用 cpu->fetched_data 判断，而不是 temp！
    set_flag(cpu, N, cpu->fetched_data & (1 << 7)); // 0x80

    // 步骤 5: 设置 V (Overflow) 标志
    // 规则：直接取内存数据 (fetched_data) 的第 6 位
    set_flag(cpu, V, cpu->fetched_data & (1 << 6)); // 0x40

    // 步骤 6: 返回周期
    // BIT 指令通常不涉及跨页奖励，返回 0
    return 0;

}

//INC: Increment Memory by One
static uint8_t op_inc(CPU* cpu){
    // 步骤 1: 取数
    fetch(cpu);

    // 步骤 2: 执行递增
    // 对应图片 Operation: "M = M + 1"
    // 注意：结果存临时变量，不要修改 cpu->fetched_data
    uint8_t temp = cpu->fetched_data + 1;
    cpu_write(cpu, cpu->addr_abs, temp);
    // 步骤 3: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if M = 0"
    set_flag(cpu, Z, temp == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, temp & 0x80);

    // 步骤 4: 时钟周期
    return 0;
}

//INX: Increment X Register by One
static uint8_t op_inx(CPU* cpu){
    // 步骤 1: 执行递增
    // 对应图片 Operation: "X = X + 1"
    // 注意：结果存临时变量，不要修改 cpu->x
    uint8_t temp = cpu->x + 1;
    cpu->x = temp;

    // 步骤 2: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if X = 0"
    set_flag(cpu, Z, temp == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, temp & 0x80);

    // 步骤 3: 时钟周期
    return 0;
}

//INY: Increment Y Register by One
static uint8_t op_iny(CPU* cpu){
    // 步骤 1: 执行递增
    // 对应图片 Operation: "Y = Y + 1"
    // 注意：结果存临时变量，不要修改 cpu->y
    uint8_t temp = cpu->y + 1;
    cpu->y = temp;

    // 步骤 2: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if Y = 0"
    set_flag(cpu, Z, temp == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, temp & 0x80);

    // 步骤 3: 时钟周期
    return 0;
}

//DEC: Decrement Memory by One
static uint8_t op_dec(CPU* cpu){
    // 步骤 1: 取数
    fetch(cpu);

    // 步骤 2: 执行递减
    // 对应图片 Operation: "M = M - 1"
    // 注意：结果存临时变量，不要修改 cpu->fetched_data
    uint8_t temp = cpu->fetched_data - 1;
    cpu_write(cpu, cpu->addr_abs, temp);
    // 步骤 3: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if M = 0"
    set_flag(cpu, Z, temp == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, temp & 0x80);

    // 步骤 4: 时钟周期
    return 0;
}
//DEX: Decrement X Register by One
static uint8_t op_dex(CPU* cpu){
    // 步骤 1: 执行递减
    // 对应图片 Operation: "X = X - 1"
    // 注意：结果存临时变量，不要修改 cpu->x
    uint8_t temp = cpu->x - 1;
    cpu->x = temp;

    // 步骤 2: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if X = 0"
    set_flag(cpu, Z, temp == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, temp & 0x80);

    // 步骤 3: 时钟周期
    return 0;
}

//DEY: Decrement Y Register by One
static uint8_t op_dey(CPU* cpu){
    // 步骤 1: 执行递减
    // 对应图片 Operation: "Y = Y - 1"
    // 注意：结果存临时变量，不要修改 cpu->y
    uint8_t temp = cpu->y - 1;
    cpu->y = temp;

    // 步骤 2: 更新标志位
    // 对应图片 Flags 表格: "Z: Set if Y = 0"
    set_flag(cpu, Z, temp == 0x00);
    
    // 对应图片 Flags 表格: "N: Set if bit 7 set"
    set_flag(cpu, N, temp & 0x80);

    // 步骤 3: 时钟周期
    return 0;
}

//PHA: Push Accumulator on Stack
static uint8_t op_pha(CPU* cpu){
    // 步骤 1: 计算堆栈的物理地址并写入
    // 6502 堆栈固定在 0x0100 页面
    cpu_write(cpu, 0x0100 + cpu->stkp, cpu->a);

    // 步骤 2: 移动堆栈指针
    // 6502 是"向下生长"的：写入后，指针减小
    cpu->stkp--;

    // 步骤 3: 返回周期
    // 隐含寻址，无跨页
    return 0;
}

// PHP: Push Processor Status to Stack
static uint8_t op_php(CPU* cpu){
    // 步骤 1: 计算堆栈地址并写入
    // 关键修正：在压入堆栈时，强制将 Break (B) 和 Unused (U) 标志位置为 1
    // cpu.h 中定义: B = (1<<4), U = (1<<5)
    // 所以 (1<<4) | (1<<5) = 0x10 | 0x20 = 0x30
    cpu_write(cpu, 0x0100 + cpu->stkp, cpu->status | B | U);

    // 步骤 2: 移动堆栈指针
    // 标志位 Z 和 N 不受影响
    set_flag(cpu, B, 0); // 可选：保持内部状态干净，虽然实际上不应该读它
    set_flag(cpu, U, 1); // 可选：保持内部状态
    
    // 指针减小
    cpu->stkp--;

    // 步骤 3: 返回周期
    return 0;
}

// PLA: Pull Accumulator from Stack
static uint8_t op_pla(CPU* cpu){
    // 步骤 1: 必须先移动堆栈指针！
    // 6502 是"向下生长"的：数据在指针的"上面"
    cpu->stkp++;

    // 步骤 2: 读取数据
    cpu->a = cpu_read(cpu, 0x0100 + cpu->stkp);

    // 步骤 3: 更新标志位
    set_flag(cpu, Z, cpu->a == 0x00);
    set_flag(cpu, N, cpu->a & 0x80);

    // 步骤 4: 返回周期
    return 0;
}

// PLP: Pull Processor Status from Stack
static uint8_t op_plp(CPU* cpu){
    // 步骤 1: 先移动指针
    cpu->stkp++;

    // 步骤 2: 读取状态
    uint8_t fetched_status = cpu_read(cpu, 0x0100 + cpu->stkp);

    // 步骤 3: 赋值给 Status，但要进行位掩码处理
    // 规则：
    // 1. 覆盖除了 B (Bit 4) 和 U (Bit 5) 之外的所有位
    // 2. 强制 U (Bit 5) 为 1
    // 3. 强制 B (Bit 4) 为 0 (或者保持原状，因为 B 不在物理寄存器中)
    
    // cpu.h 中定义：U = (1<<5), B = (1<<4)
    // 逻辑：(读取值 且上 "非B") 或上 "U"
    cpu->status = fetched_status;
    
    set_flag(cpu, U, 1); // 必须是 1
    set_flag(cpu, B, 0); // 必须是 0 (Break 标志不在 CPU 寄存器里存活)

    // 另一种更专业的写法是直接位运算：
    // cpu->status = (fetched_status & ~B) | U;

    return 0;
}

// BCC: Branch if Carry Clear
static uint8_t op_bcc(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BCC 的条件是：Carry Flag 为 0
    if(get_flag(cpu, C) == 0){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}


//BCS: Branch if Carry Set
static uint8_t op_bcs(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BCS 的条件是：Carry Flag 为 1
    if(get_flag(cpu, C) == 1){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}

//BEQ: Branch if Equal
static uint8_t op_beq(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BEQ 的条件是：Zero Flag 为 1
    if(get_flag(cpu, Z) == 1){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}

//BNE: Branch if Not Equal
static uint8_t op_bne(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BNE 的条件是：Zero Flag 为 0
    if(get_flag(cpu, Z) == 0){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}

//BMI: Branch if Minus
static uint8_t op_bmi(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BMI 的条件是：Negative Flag 为 1
    if(get_flag(cpu, N) == 1){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}

//BPL: Branch if Plus
static uint8_t op_bpl(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BPL 的条件是：Negative Flag 为 0
    if(get_flag(cpu, N) == 0){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}

//BVC: Branch if Overflow Clear
static uint8_t op_bvc(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BVC 的条件是：Overflow Flag 为 0
    if(get_flag(cpu, V) == 0){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}

//BVS: Branch if Overflow Set
static uint8_t op_bvs(CPU* cpu){
    // 步骤 1: 检查跳转条件
    // BVS 的条件是：Overflow Flag 为 1
    if(get_flag(cpu, V) == 1){
        // --- 分支成立，开始计费 ---
        // 费用 1: 只要发生了跳转，就增加 1 个时钟周期
        cpu->cycles++;

        // 步骤 2: 计算目标地址
        // addr_rel 是一个有符号的 16 位数 (例如 -5 或 +10)
        // 这里的 cpu->pc 已经是下一条指令的地址了（在 fetch 阶段被增加了）
        cpu->addr_abs = cpu->pc + cpu->addr_rel;

        // 步骤 3: 检查跨页 (Page Crossing)
        // 费用 2: 如果跳转后的地址和当前地址不在同一页 (高 8 位不同)，再加 1 周期      
        // 比如从 0x10FE 跳到了 0x1105
        if ((cpu->addr_abs & 0xFF00) != (cpu->pc & 0xFF00)) {
            cpu->cycles++;
        }

        // 步骤 4: 执行跳转 (修改 PC)
        cpu->pc = cpu->addr_abs;
    }

    // 分支指令不需要外部的额外周期反馈，内部已经加完了
    return 0;
}   

// JMP: Jump to Location
static uint8_t op_jmp(CPU* cpu) {
    // 步骤 1: 直接修改 PC
    // cpu->addr_abs 已经在 addrmode 阶段（addr_abs 或 addr_ind）被计算为"目标跳转地址"
    cpu->pc = cpu->addr_abs;

    // 步骤 2: 返回周期
    // JMP 不支持跨页奖励，也没有额外的条件判断，直接返回 0
    return 0;
}

// JSR: Jump to Subroutine
static uint8_t op_jsr(CPU* cpu){
    // 步骤 1: 计算返回地址
    // 6502 的 JSR 将"JSR 指令的最后一个字节的地址"压入堆栈
    // 此时 cpu->pc 指向的是下一条指令的开头
    // 所以减 1 正好就是 JSR 指令的最后一个字节
    uint16_t return_addr = cpu->pc - 1;

    // 步骤 2: 压栈 (关键修正：必须先高后低！)
    
    // 2.1 压入高 8 位 (High Byte)
    cpu_write(cpu, 0x0100 + cpu->stkp, (return_addr >> 8) & 0xFF);
    cpu->stkp--;

    // 2.2 压入低 8 位 (Low Byte)
    cpu_write(cpu, 0x0100 + cpu->stkp, return_addr & 0xFF);
    cpu->stkp--;

    // 步骤 3: 跳转
    cpu->pc = cpu->addr_abs;

    // 步骤 4: 返回周期
    return 0;
}

//RTS: Return from Subroutine
static uint8_t op_rts(CPU* cpu){
    // 步骤 1: 从栈弹出返回地址 (先高后低！)
    
    // 1.1 弹出低 8 位 (Low Byte)
    cpu->stkp++;
    uint8_t low_byte = cpu_read(cpu, 0x0100 + cpu->stkp);
    

    // 1.2 弹出高 8 位 (High Byte)
    cpu->stkp++;
    uint8_t high_byte = cpu_read(cpu, 0x0100 + cpu->stkp);


    // 1.3 合并为 16 位返回地址
    uint16_t return_addr = (high_byte << 8) | low_byte;

    // 步骤 2: 跳转
    cpu->pc = return_addr + 1; // 6502 的 RTS 会将返回地址加 1 后跳转

    // 步骤 3: 返回周期
    return 0;
}


// ASL: Arithmetic Shift Left
static uint8_t op_asl(CPU* cpu){
    // 步骤 1: 取数
    // fetch() 会自动处理：
    // - 如果是 Accumulator 模式，把 cpu->a 放入 cpu->fetched_data
    // - 如果是内存模式，把内存值放入 cpu->fetched_data
    fetch(cpu);
    
    // 步骤 2: 获取操作数并移位
    uint16_t temp = (uint16_t)cpu->fetched_data << 1;

    // 步骤 3: 设置标志位
    // C (Carry): 如果移出的一位是1 (即原数据的第7位是1，或者结果 > 255)
    set_flag(cpu, C, (temp & 0xFF00) > 0);
    
    // Z (Zero): 低 8 位是否为 0
    set_flag(cpu, Z, (temp & 0x00FF) == 0x00);
    
    // N (Negative): 第 7 位是否为 1
    set_flag(cpu, N, temp & 0x80);

    // 步骤 4: 写回结果 (关键修正点！)
    // 需要判断当前的寻址模式是 "累加器" 还是 "内存"
    if (lookup[cpu->opcode].addrmode == &addr_acc) {
        // 如果是 ASL A，写回累加器
        cpu->a = temp & 0x00FF;
    } else {
        // 如果是 ASL $xxxx，写回内存
        cpu_write(cpu, cpu->addr_abs, temp & 0x00FF);
    }

    // 步骤 5: 返回周期
    return 0;
}


// LSR: Logical Shift Right
static uint8_t op_lsr(CPU* cpu){
    // 步骤 1: 取数
    // fetch() 会自动处理：
    // - 如果是 Accumulator 模式，把 cpu->a 放入 cpu->fetched_data
    // - 如果是内存模式，把内存值放入 cpu->fetched_data
    fetch(cpu);
    
    // 步骤 2: 获取操作数并移位
    uint16_t temp = (uint16_t)cpu->fetched_data >> 1;

    // 步骤 3: 设置标志位
    // C (Carry): 如果移出的一位是1 (即原数据的第0位是1)
    set_flag(cpu, C, cpu->fetched_data & 0x01);
    
    // Z (Zero): 低 8 位是否为 0
    set_flag(cpu, Z, (temp & 0x00FF) == 0x00);
    
    // N (Negative): 第 7 位是否为 1
    set_flag(cpu, N, temp & 0x80);

    // 步骤 4: 写回结果 (关键修正点！)
    // 需要判断当前的寻址模式是 "累加器" 还是 "内存"
    if (lookup[cpu->opcode].addrmode == &addr_acc) {
        // 如果是 ASL A，写回累加器
        cpu->a = temp & 0x00FF;
    } else {
        // 如果是 ASL $xxxx，写回内存
        cpu_write(cpu, cpu->addr_abs, temp & 0x00FF);
    }

    // 步骤 5: 返回周期
    return 0;
}

// ROL: Rotate Left
static uint8_t op_rol(CPU* cpu){
    fetch(cpu);
    
    // 逻辑：向左移，旧 Carry 补入最低位
    uint16_t temp = (uint16_t)(cpu->fetched_data << 1) | get_flag(cpu, C);

    // 步骤 3: 设置标志位
    // 【关键修正】左移出的位是 Bit 7。
    // 方法 A：检查原数据的 0x80
    // 方法 B (推荐)：检查 16 位结果的高 8 位是否有值 (temp > 255)
    set_flag(cpu, C, (temp & 0xFF00)); 
    
    set_flag(cpu, Z, (temp & 0x00FF) == 0x00);
    set_flag(cpu, N, temp & 0x80);

    // 步骤 4: 写回结果
    if (lookup[cpu->opcode].addrmode == &addr_acc) {
        cpu->a = temp & 0x00FF;
    } else {
        cpu_write(cpu, cpu->addr_abs, temp & 0x00FF);
    }
    return 0;
}

// ROR: Rotate Right
static uint8_t op_ror(CPU* cpu){
    fetch(cpu);
    
    // 逻辑：向右移，旧 Carry 补入最高位
    uint16_t temp = (uint16_t)(cpu->fetched_data >> 1) | (get_flag(cpu, C) << 7);

    // 步骤 3: 设置标志位
    // ROR 移出的是最低位 (Bit 0)，你的原始写法是对的
    set_flag(cpu, C, cpu->fetched_data & 0x01);
    
    set_flag(cpu, Z, (temp & 0x00FF) == 0x00);
    set_flag(cpu, N, temp & 0x80);

    // 步骤 4: 写回结果
    if (lookup[cpu->opcode].addrmode == &addr_acc) {
        cpu->a = temp & 0x00FF;
    } else {
        cpu_write(cpu, cpu->addr_abs, temp & 0x00FF);
    }
    return 0;
}

// ADC: Add with Carry
static uint8_t op_adc(CPU* cpu) {
    // 步骤 1: 取数
    fetch(cpu);
    
    // 步骤 2: 执行加法
    // 我们必须用 16 位变量来存储结果，以便检测进位 (Carry)
    // 公式: Temp = A + M + C
    uint16_t temp = (uint16_t)cpu->a + (uint16_t)cpu->fetched_data + (uint16_t)get_flag(cpu, C);
    
    // 步骤 3: 设置 Carry (C) 标志
    // 如果结果 > 255 (即 0xFF)，说明发生了进位
    // "Set if overflow in bit 7"
    set_flag(cpu, C, temp > 255);
    
    // 步骤 4: 设置 Zero (Z) 标志
    // 只看结果的低 8 位是否为 0
    set_flag(cpu, Z, (temp & 0x00FF) == 0);
    
    // 步骤 5: 设置 Negative (N) 标志
    // 看结果的第 7 位 (最高位)
    set_flag(cpu, N, temp & 0x80);
    
    // 步骤 6: 设置 Overflow (V) 标志 (这是最难的部分)
    // "Set if sign bit is incorrect"
    // 逻辑：只有当"正数+正数=负数" 或 "负数+负数=正数" 时，才算溢出。
    // 如果是"正+负"，永远不可能溢出。
    // 公式解释见下方。
    set_flag(cpu, V, (~((uint16_t)cpu->a ^ (uint16_t)cpu->fetched_data) & ((uint16_t)cpu->a ^ temp)) & 0x0080);
    
    // 步骤 7: 将结果存回 A
    cpu->a = temp & 0x00FF;
    
    // 步骤 8: 返回周期
    // 图片显示 "+1 if page crossed"，所以返回 1
    return 1;
}

//SBC: Subtract with Carry
static uint8_t op_sbc(CPU* cpu) {
    // 步骤 1: 取数
    fetch(cpu);
    cpu->fetched_data = cpu->fetched_data ^ 0xFF;
    // 步骤 2: 执行加法
    // 我们必须用 16 位变量来存储结果，以便检测进位 (Carry)
    // 公式: Temp = A + M + C
    uint16_t temp = (uint16_t)cpu->a + (uint16_t)cpu->fetched_data + (uint16_t)get_flag(cpu, C);
    
    // 步骤 3: 设置 Carry (C) 标志
    // 如果结果 > 255 (即 0xFF)，说明发生了进位
    // "Set if overflow in bit 7"
    set_flag(cpu, C, temp > 255);
    
    // 步骤 4: 设置 Zero (Z) 标志
    // 只看结果的低 8 位是否为 0
    set_flag(cpu, Z, (temp & 0x00FF) == 0);
    
    // 步骤 5: 设置 Negative (N) 标志
    // 看结果的第 7 位 (最高位)
    set_flag(cpu, N, temp & 0x80);
    
    // 步骤 6: 设置 Overflow (V) 标志 (这是最难的部分)
    // "Set if sign bit is incorrect"
    // 逻辑：只有当"正数+正数=负数" 或 "负数+负数=正数" 时，才算溢出。
    // 如果是"正+负"，永远不可能溢出。
    // 公式解释见下方。
    set_flag(cpu, V, (~((uint16_t)cpu->a ^ (uint16_t)cpu->fetched_data) & ((uint16_t)cpu->a ^ temp)) & 0x0080);
    
    // 步骤 7: 将结果存回 A
    cpu->a = temp & 0x00FF;
    
    // 步骤 8: 返回周期
    // 图片显示 "+1 if page crossed"，所以返回 1
    return 1;
}


// BRK: Force Interrupt
static uint8_t op_brk(CPU* cpu) {
    // 步骤 1: PC 自增 (跳过填充字节)
    cpu->pc++;

    // 步骤 2: 将 PC 压栈 (先高后低)
    cpu_write(cpu, 0x0100 + cpu->stkp, (cpu->pc >> 8) & 0xFF);
    cpu->stkp--;
    cpu_write(cpu, 0x0100 + cpu->stkp, cpu->pc & 0xFF);
    cpu->stkp--;

    // 步骤 3: 将状态寄存器压栈
    // 关键点：软件中断 BRK 发生时，压入堆栈的标志位必须包含 B(Bit 4) 和 U(Bit 5)
    // cpu.h 中定义: B = (1<<4), U = (1<<5)
    cpu_write(cpu, 0x0100 + cpu->stkp, cpu->status | B | U);
    cpu->stkp--;

    // 步骤 4: 设置中断屏蔽标志 (Disable Interrupts)
    set_flag(cpu, I, 1);

    // 步骤 5: 读取中断向量 (IRQ/BRK Vector: $FFFE - $FFFF)
    // 并跳转
    uint16_t lo = cpu_read(cpu, 0xFFFE);
    uint16_t hi = cpu_read(cpu, 0xFFFF);
    cpu->pc = (hi << 8) | lo;

    // 步骤 6: 返回 0 (BRK 固定 7 周期，已在表中定义，无需额外返回 1)
    return 0;
}

// RTI: Return from Interrupt
static uint8_t op_rti(CPU* cpu) {
    // 步骤 1: 弹出状态寄存器
    cpu->stkp++;
    uint8_t fetched_status = cpu_read(cpu, 0x0100 + cpu->stkp);
    
    // 恢复状态时的标准操作：忽略 B 位，强制 U 位为 1
    cpu->status = fetched_status;
    set_flag(cpu, B, 0); // B 标志实际上不在寄存器中存在
    set_flag(cpu, U, 1); // U 标志总是 1

    // 步骤 2: 弹出 PC (先低后高，与压栈相反)
    cpu->stkp++;
    uint16_t lo = cpu_read(cpu, 0x0100 + cpu->stkp);
    cpu->stkp++;
    uint16_t hi = cpu_read(cpu, 0x0100 + cpu->stkp);

    cpu->pc = (hi << 8) | lo;

    return 0;
}