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
