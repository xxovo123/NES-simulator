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
