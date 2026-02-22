/**
 * NESPRESSO - NES Emulator
 * CPU Module - 6502 Processor Implementation
 *
 * Full implementation of all 151 official 6502 opcodes
 * with cycle-accurate timing where possible.
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "cpu.h"
#include <string.h>
#include <stdio.h>

/* Opcode table with mnemonics, addressing modes, cycles, and lengths */
static const opcode_info_t g_opcode_table[256] = {
    /* $00 */ {"BRK", MODE_IMPLIED, 7, 1},
    /* $01 */ {"ORA", MODE_INDEXED_INDIRECT, 6, 2},
    /* $02 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $03 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $04 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $05 */ {"ORA", MODE_ZERO_PAGE, 3, 2},
    /* $06 */ {"ASL", MODE_ZERO_PAGE, 5, 2},
    /* $07 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $08 */ {"PHP", MODE_IMPLIED, 3, 1},
    /* $09 */ {"ORA", MODE_IMMEDIATE, 2, 2},
    /* $0A */ {"ASL", MODE_ACCUMULATOR, 2, 1},
    /* $0B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $0C */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $0D */ {"ORA", MODE_ABSOLUTE, 4, 3},
    /* $0E */ {"ASL", MODE_ABSOLUTE, 6, 3},
    /* $0F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $10 */ {"BPL", MODE_RELATIVE, 2, 2},
    /* $11 */ {"ORA", MODE_INDIRECT_INDEXED, 5, 2},
    /* $12 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $13 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $14 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $15 */ {"ORA", MODE_ZERO_PAGE_X, 4, 2},
    /* $16 */ {"ASL", MODE_ZERO_PAGE_X, 6, 2},
    /* $17 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $18 */ {"CLC", MODE_IMPLIED, 2, 1},
    /* $19 */ {"ORA", MODE_ABSOLUTE_Y, 4, 3},
    /* $1A */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $1B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $1C */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $1D */ {"ORA", MODE_ABSOLUTE_X, 4, 3},
    /* $1E */ {"ASL", MODE_ABSOLUTE_X, 7, 3},
    /* $1F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $20 */ {"JSR", MODE_ABSOLUTE, 6, 3},
    /* $21 */ {"AND", MODE_INDEXED_INDIRECT, 6, 2},
    /* $22 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $23 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $24 */ {"BIT", MODE_ZERO_PAGE, 3, 2},
    /* $25 */ {"AND", MODE_ZERO_PAGE, 3, 2},
    /* $26 */ {"ROL", MODE_ZERO_PAGE, 5, 2},
    /* $27 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $28 */ {"PLP", MODE_IMPLIED, 4, 1},
    /* $29 */ {"AND", MODE_IMMEDIATE, 2, 2},
    /* $2A */ {"ROL", MODE_ACCUMULATOR, 2, 1},
    /* $2B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $2C */ {"BIT", MODE_ABSOLUTE, 4, 3},
    /* $2D */ {"AND", MODE_ABSOLUTE, 4, 3},
    /* $2E */ {"ROL", MODE_ABSOLUTE, 6, 3},
    /* $2F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $30 */ {"BMI", MODE_RELATIVE, 2, 2},
    /* $31 */ {"AND", MODE_INDIRECT_INDEXED, 5, 2},
    /* $32 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $33 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $34 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $35 */ {"AND", MODE_ZERO_PAGE_X, 4, 2},
    /* $36 */ {"ROL", MODE_ZERO_PAGE_X, 6, 2},
    /* $37 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $38 */ {"SEC", MODE_IMPLIED, 2, 1},
    /* $39 */ {"AND", MODE_ABSOLUTE_Y, 4, 3},
    /* $3A */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $3B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $3C */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $3D */ {"AND", MODE_ABSOLUTE_X, 4, 3},
    /* $3E */ {"ROL", MODE_ABSOLUTE_X, 7, 3},
    /* $3F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $40 */ {"RTI", MODE_IMPLIED, 6, 1},
    /* $41 */ {"EOR", MODE_INDEXED_INDIRECT, 6, 2},
    /* $42 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $43 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $44 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $45 */ {"EOR", MODE_ZERO_PAGE, 3, 2},
    /* $46 */ {"LSR", MODE_ZERO_PAGE, 5, 2},
    /* $47 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $48 */ {"PHA", MODE_IMPLIED, 3, 1},
    /* $49 */ {"EOR", MODE_IMMEDIATE, 2, 2},
    /* $4A */ {"LSR", MODE_ACCUMULATOR, 2, 1},
    /* $4B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $4C */ {"JMP", MODE_ABSOLUTE, 3, 3},
    /* $4D */ {"EOR", MODE_ABSOLUTE, 4, 3},
    /* $4E */ {"LSR", MODE_ABSOLUTE, 6, 3},
    /* $4F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $50 */ {"BVC", MODE_RELATIVE, 2, 2},
    /* $51 */ {"EOR", MODE_INDIRECT_INDEXED, 5, 2},
    /* $52 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $53 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $54 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $55 */ {"EOR", MODE_ZERO_PAGE_X, 4, 2},
    /* $56 */ {"LSR", MODE_ZERO_PAGE_X, 6, 2},
    /* $57 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $58 */ {"CLI", MODE_IMPLIED, 2, 1},
    /* $59 */ {"EOR", MODE_ABSOLUTE_Y, 4, 3},
    /* $5A */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $5B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $5C */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $5D */ {"EOR", MODE_ABSOLUTE_X, 4, 3},
    /* $5E */ {"LSR", MODE_ABSOLUTE_X, 7, 3},
    /* $5F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $60 */ {"RTS", MODE_IMPLIED, 6, 1},
    /* $61 */ {"ADC", MODE_INDEXED_INDIRECT, 6, 2},
    /* $62 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $63 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $64 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $65 */ {"ADC", MODE_ZERO_PAGE, 3, 2},
    /* $66 */ {"ROR", MODE_ZERO_PAGE, 5, 2},
    /* $67 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $68 */ {"PLA", MODE_IMPLIED, 4, 1},
    /* $69 */ {"ADC", MODE_IMMEDIATE, 2, 2},
    /* $6A */ {"ROR", MODE_ACCUMULATOR, 2, 1},
    /* $6B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $6C */ {"JMP", MODE_INDIRECT, 5, 3},
    /* $6D */ {"ADC", MODE_ABSOLUTE, 4, 3},
    /* $6E */ {"ROR", MODE_ABSOLUTE, 6, 3},
    /* $6F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $70 */ {"BVS", MODE_RELATIVE, 2, 2},
    /* $71 */ {"ADC", MODE_INDIRECT_INDEXED, 5, 2},
    /* $72 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $73 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $74 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $75 */ {"ADC", MODE_ZERO_PAGE_X, 4, 2},
    /* $76 */ {"ROR", MODE_ZERO_PAGE_X, 6, 2},
    /* $77 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $78 */ {"SEI", MODE_IMPLIED, 2, 1},
    /* $79 */ {"ADC", MODE_ABSOLUTE_Y, 4, 3},
    /* $7A */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $7B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $7C */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $7D */ {"ADC", MODE_ABSOLUTE_X, 4, 3},
    /* $7E */ {"ROR", MODE_ABSOLUTE_X, 7, 3},
    /* $7F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $80 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $81 */ {"STA", MODE_INDEXED_INDIRECT, 6, 2},
    /* $82 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $83 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $84 */ {"STY", MODE_ZERO_PAGE, 3, 2},
    /* $85 */ {"STA", MODE_ZERO_PAGE, 3, 2},
    /* $86 */ {"STX", MODE_ZERO_PAGE, 3, 2},
    /* $87 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $88 */ {"DEY", MODE_IMPLIED, 2, 1},
    /* $89 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $8A */ {"TXA", MODE_IMPLIED, 2, 1},
    /* $8B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $8C */ {"STY", MODE_ABSOLUTE, 4, 3},
    /* $8D */ {"STA", MODE_ABSOLUTE, 4, 3},
    /* $8E */ {"STX", MODE_ABSOLUTE, 4, 3},
    /* $8F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $90 */ {"BCC", MODE_RELATIVE, 2, 2},
    /* $91 */ {"STA", MODE_INDIRECT_INDEXED, 6, 2},
    /* $92 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $93 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $94 */ {"STY", MODE_ZERO_PAGE_X, 4, 2},
    /* $95 */ {"STA", MODE_ZERO_PAGE_X, 4, 2},
    /* $96 */ {"STX", MODE_ZERO_PAGE_Y, 4, 2},
    /* $97 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $98 */ {"TYA", MODE_IMPLIED, 2, 1},
    /* $99 */ {"STA", MODE_ABSOLUTE_Y, 5, 3},
    /* $9A */ {"TXS", MODE_IMPLIED, 2, 1},
    /* $9B */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $9C */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $9D */ {"STA", MODE_ABSOLUTE_X, 5, 3},
    /* $9E */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $9F */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $A0 */ {"LDY", MODE_IMMEDIATE, 2, 2},
    /* $A1 */ {"LDA", MODE_INDEXED_INDIRECT, 6, 2},
    /* $A2 */ {"LDX", MODE_IMMEDIATE, 2, 2},
    /* $A3 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $A4 */ {"LDY", MODE_ZERO_PAGE, 3, 2},
    /* $A5 */ {"LDA", MODE_ZERO_PAGE, 3, 2},
    /* $A6 */ {"LDX", MODE_ZERO_PAGE, 3, 2},
    /* $A7 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $A8 */ {"TAY", MODE_IMPLIED, 2, 1},
    /* $A9 */ {"LDA", MODE_IMMEDIATE, 2, 2},
    /* $AA */ {"TAX", MODE_IMPLIED, 2, 1},
    /* $AB */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $AC */ {"LDY", MODE_ABSOLUTE, 4, 3},
    /* $AD */ {"LDA", MODE_ABSOLUTE, 4, 3},
    /* $AE */ {"LDX", MODE_ABSOLUTE, 4, 3},
    /* $AF */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $B0 */ {"BCS", MODE_RELATIVE, 2, 2},
    /* $B1 */ {"LDA", MODE_INDIRECT_INDEXED, 5, 2},
    /* $B2 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $B3 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $B4 */ {"LDY", MODE_ZERO_PAGE_X, 4, 2},
    /* $B5 */ {"LDA", MODE_ZERO_PAGE_X, 4, 2},
    /* $B6 */ {"LDX", MODE_ZERO_PAGE_Y, 4, 2},
    /* $B7 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $B8 */ {"CLV", MODE_IMPLIED, 2, 1},
    /* $B9 */ {"LDA", MODE_ABSOLUTE_Y, 4, 3},
    /* $BA */ {"TSX", MODE_IMPLIED, 2, 1},
    /* $BB */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $BC */ {"LDY", MODE_ABSOLUTE_X, 4, 3},
    /* $BD */ {"LDA", MODE_ABSOLUTE_X, 4, 3},
    /* $BE */ {"LDX", MODE_ABSOLUTE_Y, 4, 3},
    /* $BF */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $C0 */ {"CPY", MODE_IMMEDIATE, 2, 2},
    /* $C1 */ {"CMP", MODE_INDEXED_INDIRECT, 6, 2},
    /* $C2 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $C3 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $C4 */ {"CPY", MODE_ZERO_PAGE, 3, 2},
    /* $C5 */ {"CMP", MODE_ZERO_PAGE, 3, 2},
    /* $C6 */ {"DEC", MODE_ZERO_PAGE, 5, 2},
    /* $C7 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $C8 */ {"INY", MODE_IMPLIED, 2, 1},
    /* $C9 */ {"CMP", MODE_IMMEDIATE, 2, 2},
    /* $CA */ {"DEX", MODE_IMPLIED, 2, 1},
    /* $CB */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $CC */ {"CPY", MODE_ABSOLUTE, 4, 3},
    /* $CD */ {"CMP", MODE_ABSOLUTE, 4, 3},
    /* $CE */ {"DEC", MODE_ABSOLUTE, 6, 3},
    /* $CF */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $D0 */ {"BNE", MODE_RELATIVE, 2, 2},
    /* $D1 */ {"CMP", MODE_INDIRECT_INDEXED, 5, 2},
    /* $D2 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $D3 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $D4 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $D5 */ {"CMP", MODE_ZERO_PAGE_X, 4, 2},
    /* $D6 */ {"DEC", MODE_ZERO_PAGE_X, 6, 2},
    /* $D7 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $D8 */ {"CLD", MODE_IMPLIED, 2, 1},
    /* $D9 */ {"CMP", MODE_ABSOLUTE_Y, 4, 3},
    /* $DA */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $DB */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $DC */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $DD */ {"CMP", MODE_ABSOLUTE_X, 4, 3},
    /* $DE */ {"DEC", MODE_ABSOLUTE_X, 7, 3},
    /* $DF */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $E0 */ {"CPX", MODE_IMMEDIATE, 2, 2},
    /* $E1 */ {"SBC", MODE_INDEXED_INDIRECT, 6, 2},
    /* $E2 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $E3 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $E4 */ {"CPX", MODE_ZERO_PAGE, 3, 2},
    /* $E5 */ {"SBC", MODE_ZERO_PAGE, 3, 2},
    /* $E6 */ {"INC", MODE_ZERO_PAGE, 5, 2},
    /* $E7 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $E8 */ {"INX", MODE_IMPLIED, 2, 1},
    /* $E9 */ {"SBC", MODE_IMMEDIATE, 2, 2},
    /* $EA */ {"NOP", MODE_IMPLIED, 2, 1},
    /* $EB */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $EC */ {"CPX", MODE_ABSOLUTE, 4, 3},
    /* $ED */ {"SBC", MODE_ABSOLUTE, 4, 3},
    /* $EE */ {"INC", MODE_ABSOLUTE, 6, 3},
    /* $EF */ {"??? ", MODE_IMPLIED, 2, 1},

    /* $F0 */ {"BEQ", MODE_RELATIVE, 2, 2},
    /* $F1 */ {"SBC", MODE_INDIRECT_INDEXED, 5, 2},
    /* $F2 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $F3 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $F4 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $F5 */ {"SBC", MODE_ZERO_PAGE_X, 4, 2},
    /* $F6 */ {"INC", MODE_ZERO_PAGE_X, 6, 2},
    /* $F7 */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $F8 */ {"SED", MODE_IMPLIED, 2, 1},
    /* $F9 */ {"SBC", MODE_ABSOLUTE_Y, 4, 3},
    /* $FA */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $FB */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $FC */ {"??? ", MODE_IMPLIED, 2, 1},
    /* $FD */ {"SBC", MODE_ABSOLUTE_X, 4, 3},
    /* $FE */ {"INC", MODE_ABSOLUTE_X, 7, 3},
    /* $FF */ {"??? ", MODE_IMPLIED, 2, 1},
};

/* Global bus pointer - static storage that persists beyond function calls */
static cpu_bus_t g_cpu_bus_static = {NULL, NULL, NULL};

/* Global bus pointer (accessed via extern declaration in header) */
cpu_bus_t* g_bus = &g_cpu_bus_static;

/* Helper functions */

static void read8(nes_cpu_t* cpu, uint16_t addr, uint8_t* out) {
    (void)cpu;
    *out = g_bus->read(g_bus->context, addr);
}

static void read16(nes_cpu_t* cpu, uint16_t addr, uint16_t* out) {
    (void)cpu;
    *out = g_bus->read(g_bus->context, addr);
    *out |= ((uint16_t)g_bus->read(g_bus->context, addr + 1)) << 8;
}

/* Get effective address based on addressing mode */
static uint16_t get_effective_address(nes_cpu_t* cpu, addr_mode_t mode) {
    switch (mode) {
        case MODE_IMMEDIATE:
            return cpu->reg.pc++;

        case MODE_ZERO_PAGE: {
            return g_bus->read(g_bus->context, cpu->reg.pc++);
        }

        case MODE_ZERO_PAGE_X: {
            return (g_bus->read(g_bus->context, cpu->reg.pc++) + cpu->reg.x) & 0xFF;
        }

        case MODE_ZERO_PAGE_Y: {
            return (g_bus->read(g_bus->context, cpu->reg.pc++) + cpu->reg.y) & 0xFF;
        }

        case MODE_ABSOLUTE: {
            uint16_t addr;
            read16(cpu, cpu->reg.pc, &addr);
            cpu->reg.pc += 2;
            return addr;
        }

        case MODE_ABSOLUTE_X: {
            uint16_t base;
            read16(cpu, cpu->reg.pc, &base);
            cpu->reg.pc += 2;
            uint16_t addr = base + cpu->reg.x;
            /* Page boundary penalty */
            if ((base & 0xFF00) != (addr & 0xFF00)) {
                cpu->stall_cycles = 1;
            }
            return addr;
        }

        case MODE_ABSOLUTE_Y: {
            uint16_t base;
            read16(cpu, cpu->reg.pc, &base);
            cpu->reg.pc += 2;
            uint16_t addr = base + cpu->reg.y;
            /* Page boundary penalty */
            if ((base & 0xFF00) != (addr & 0xFF00)) {
                cpu->stall_cycles = 1;
            }
            return addr;
        }

        case MODE_INDIRECT: {
            uint16_t ptr;
            read16(cpu, cpu->reg.pc, &ptr);
            cpu->reg.pc += 2;
            /* Indirect JMP bug: low byte high byte from same page */
            uint16_t addr = g_bus->read(g_bus->context, ptr);
            addr |= ((uint16_t)g_bus->read(g_bus->context, (ptr & 0xFF00) | ((ptr + 1) & 0xFF))) << 8;
            return addr;
        }

        case MODE_INDEXED_INDIRECT: {  /* (Indirect,X) */
            uint8_t ptr = (g_bus->read(g_bus->context, cpu->reg.pc++) + cpu->reg.x) & 0xFF;
            uint16_t addr = g_bus->read(g_bus->context, ptr);
            addr |= ((uint16_t)g_bus->read(g_bus->context, (ptr + 1) & 0xFF)) << 8;
            return addr;
        }

        case MODE_INDIRECT_INDEXED: {  /* (Indirect),Y */
            uint8_t ptr = g_bus->read(g_bus->context, cpu->reg.pc++);
            uint16_t base = g_bus->read(g_bus->context, ptr);
            base |= ((uint16_t)g_bus->read(g_bus->context, (ptr + 1) & 0xFF)) << 8;
            uint16_t addr = base + cpu->reg.y;
            /* Page boundary penalty */
            if ((base & 0xFF00) != (addr & 0xFF00)) {
                cpu->stall_cycles = 1;
            }
            return addr;
        }

        case MODE_RELATIVE: {
            int8_t offset = (int8_t)g_bus->read(g_bus->context, cpu->reg.pc++);
            return cpu->reg.pc + offset;
        }

        default:
            return 0;
    }
}

/* Load a byte from memory based on addressing mode */
static uint8_t load_byte(nes_cpu_t* cpu, addr_mode_t mode) {
    switch (mode) {
        case MODE_ACCUMULATOR:
            return cpu->reg.a;
        case MODE_IMMEDIATE:
            return g_bus->read(g_bus->context, cpu->reg.pc++);
        default: {
            uint16_t addr = get_effective_address(cpu, mode);
            return g_bus->read(g_bus->context, addr);
        }
    }
}

/* Store a byte to memory based on addressing mode */
static void store_byte(nes_cpu_t* cpu, addr_mode_t mode, uint8_t value) {
    uint16_t addr = get_effective_address(cpu, mode);
    g_bus->write(g_bus->context, addr, value);
}

/* Branch instruction helper */
static void do_branch(nes_cpu_t* cpu, int condition) {
    int8_t offset = (int8_t)g_bus->read(g_bus->context, cpu->reg.pc++);
    if (condition) {
        uint16_t old_pc = cpu->reg.pc;
        cpu->reg.pc += offset;
        /* Page boundary penalty */
        if ((old_pc & 0xFF00) != (cpu->reg.pc & 0xFF00)) {
            cpu->stall_cycles = 1;
        }
    }
}

/* Helper for comparing (CMP, CPX, CPY) */
static void compare(nes_cpu_t* cpu, uint8_t reg_val, uint8_t mem_val) {
    uint8_t result = reg_val - mem_val;
    nes_cpu_set_flag(cpu, FLAG_CARRY, reg_val >= mem_val);
    nes_cpu_set_flag(cpu, FLAG_ZERO, (result == 0));
    nes_cpu_set_flag(cpu, FLAG_NEGATIVE, (result & 0x80) != 0);
}

/* ADC instruction */
static void do_adc(nes_cpu_t* cpu, uint8_t value) {
    uint16_t result = cpu->reg.a + value + nes_cpu_get_flag(cpu, FLAG_CARRY);
    nes_cpu_set_flag(cpu, FLAG_CARRY, result > 0xFF);
    nes_cpu_set_flag(cpu, FLAG_OVERFLOW,
        ((~(cpu->reg.a ^ value) & (cpu->reg.a ^ result)) & 0x80) != 0);
    cpu->reg.a = (uint8_t)result;
    nes_cpu_update_zn(cpu, cpu->reg.a);
}

/* SBC instruction */
static void do_sbc(nes_cpu_t* cpu, uint8_t value) {
    do_adc(cpu, ~value);
}

/* Rotate Left helper */
static uint8_t do_rol(uint8_t value, uint8_t carry_in, uint8_t* carry_out) {
    uint16_t result = ((uint16_t)value << 1) | carry_in;
    *carry_out = (result > 0xFF) ? 1 : 0;
    return (uint8_t)result;
}

/* Rotate Right helper */
static uint8_t do_ror(uint8_t value, uint8_t carry_in, uint8_t* carry_out) {
    uint16_t result = ((uint16_t)value >> 1) | (carry_in << 7);
    *carry_out = value & 1;
    return (uint8_t)result;
}

/* Bit shift left */
static uint8_t do_asl_cpu(uint8_t value, uint8_t* carry_out) {
    *carry_out = (value & 0x80) ? 1 : 0;
    return value << 1;
}

/* Bit shift right */
static uint8_t do_lsr_cpu(uint8_t value, uint8_t* carry_out) {
    *carry_out = value & 1;
    return value >> 1;
}

/* Public API Implementation */

void nes_cpu_init(nes_cpu_t* cpu, cpu_bus_t* bus) {
    memset(cpu, 0, sizeof(nes_cpu_t));
    cpu->opcode_table = g_opcode_table;
    g_bus = bus;
    if (bus) {
        nes_cpu_reset(cpu);
    }
}

void nes_cpu_reset(nes_cpu_t* cpu) {
    printf("  nes_cpu_reset called, cpu=%p, g_bus=%p\n", (void*)cpu, (void*)g_bus);
    cpu->reg.p = FLAG_UNUSED | FLAG_INTERRUPT;
    cpu->reg.sp = 0xFD;
    cpu->stall_cycles = 0;
    cpu->cycle_count = 0;
    cpu->pending_nmi = 0;
    cpu->pending_irq = 0;

    /* Read reset vector - only if bus is available */
    if (g_bus && g_bus->read && g_bus->context) {
        printf("    Reading reset vector from bus...\n");
        uint16_t reset_addr = g_bus->read(g_bus->context, NES_VECTOR_RESET);
        reset_addr |= ((uint16_t)g_bus->read(g_bus->context, NES_VECTOR_RESET + 1)) << 8;
        cpu->reg.pc = reset_addr;
        printf("    Reset vector: 0x%04X\n", reset_addr);
    } else {
        /* Default reset vector for when bus not available */
        printf("    No bus available, using default PC=0x8000\n");
        cpu->reg.pc = 0x8000;  /* Fall back to start of PRG-ROM */
    }
    printf("    CPU reset complete, PC=0x%04X\n", cpu->reg.pc);
}

uint8_t nes_cpu_step(nes_cpu_t* cpu) {
    /* Handle any stall cycles from previous instruction */
    if (cpu->stall_cycles > 0) {
        cpu->stall_cycles--;
        cpu->cycle_count++;
        return 1;
    }

    /* Check for interrupts */
    if (cpu->pending_nmi) {
        cpu->pending_nmi = 0;
        /* NMI takes 7 cycles */
        nes_cpu_push_word(cpu, cpu->reg.pc);
        uint8_t status = cpu->reg.p | FLAG_UNUSED;
        nes_cpu_push(cpu, status);

        uint16_t nmi_addr = g_bus->read(g_bus->context, NES_VECTOR_NMI);
        nmi_addr |= ((uint16_t)g_bus->read(g_bus->context, NES_VECTOR_NMI + 1)) << 8;
        cpu->reg.pc = nmi_addr;

        cpu->reg.p |= FLAG_INTERRUPT;
        cpu->cycle_count += 7;
        return 7;
    }

    if (cpu->pending_irq && !nes_cpu_get_flag(cpu, FLAG_INTERRUPT)) {
        cpu->pending_irq = 0;
        /* IRQ takes 7 cycles */
        nes_cpu_push_word(cpu, cpu->reg.pc);
        uint8_t status = cpu->reg.p | FLAG_UNUSED;
        nes_cpu_push(cpu, status);

        uint16_t irq_addr = g_bus->read(g_bus->context, NES_VECTOR_IRQ_BRK);
        irq_addr |= ((uint16_t)g_bus->read(g_bus->context, NES_VECTOR_IRQ_BRK + 1)) << 8;
        cpu->reg.pc = irq_addr;

        cpu->reg.p |= FLAG_INTERRUPT;
        cpu->cycle_count += 7;
        return 7;
    }

    /* Fetch opcode */
    uint8_t opcode = g_bus->read(g_bus->context, cpu->reg.pc);
    const opcode_info_t* info = &g_opcode_table[opcode];
    cpu->reg.pc++;

    uint8_t cycles = info->cycles;

    /* Execute instruction */
    switch (opcode) {
        /* ADC - Add with Carry */
        case 0x69: case 0x65: case 0x75: case 0x6D: case 0x7D: case 0x79:
        case 0x61: case 0x71:
            do_adc(cpu, load_byte(cpu, info->mode));
            break;

        /* AND - Logical AND */
        case 0x29: case 0x25: case 0x35: case 0x2D: case 0x3D: case 0x39:
        case 0x21: case 0x31:
            cpu->reg.a &= load_byte(cpu, info->mode);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* ASL - Arithmetic Shift Left */
        case 0x0A: {
            uint8_t carry;
            cpu->reg.a = do_asl_cpu(cpu->reg.a, &carry);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;
        }
        case 0x06: case 0x16: case 0x0E: case 0x1E: {
            uint16_t addr = get_effective_address(cpu, info->mode);
            uint8_t val = g_bus->read(g_bus->context, addr);
            uint8_t carry;
            val = do_asl_cpu(val, &carry);
            g_bus->write(g_bus->context, addr, val);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry);
            nes_cpu_update_zn(cpu, val);
            break;
        }

        /* BCC - Branch if Carry Clear */
        case 0x90:
            do_branch(cpu, !nes_cpu_get_flag(cpu, FLAG_CARRY));
            break;

        /* BCS - Branch if Carry Set */
        case 0xB0:
            do_branch(cpu, nes_cpu_get_flag(cpu, FLAG_CARRY));
            break;

        /* BEQ - Branch if Equal */
        case 0xF0:
            do_branch(cpu, nes_cpu_get_flag(cpu, FLAG_ZERO));
            break;

        /* BIT - Bit Test */
        case 0x24: case 0x2C: {
            uint8_t val = load_byte(cpu, info->mode);
            uint8_t temp = cpu->reg.a & val;
            nes_cpu_set_flag(cpu, FLAG_ZERO, temp == 0);
            nes_cpu_set_flag(cpu, FLAG_OVERFLOW, (val & 0x40) != 0);
            nes_cpu_set_flag(cpu, FLAG_NEGATIVE, (val & 0x80) != 0);
            break;
        }

        /* BMI - Branch if Minus */
        case 0x30:
            do_branch(cpu, nes_cpu_get_flag(cpu, FLAG_NEGATIVE));
            break;

        /* BNE - Branch if Not Equal */
        case 0xD0:
            do_branch(cpu, !nes_cpu_get_flag(cpu, FLAG_ZERO));
            break;

        /* BPL - Branch if Positive */
        case 0x10:
            do_branch(cpu, !nes_cpu_get_flag(cpu, FLAG_NEGATIVE));
            break;

        /* BRK - Force Interrupt */
        case 0x00:
            cpu->reg.pc++;
            nes_cpu_push_word(cpu, cpu->reg.pc);
            uint8_t status = cpu->reg.p | FLAG_BREAK | FLAG_UNUSED;
            nes_cpu_push(cpu, status);
            uint16_t brk_addr = g_bus->read(g_bus->context, NES_VECTOR_IRQ_BRK);
            brk_addr |= ((uint16_t)g_bus->read(g_bus->context, NES_VECTOR_IRQ_BRK + 1)) << 8;
            cpu->reg.pc = brk_addr;
            cpu->reg.p |= FLAG_INTERRUPT;
            break;

        /* BVC - Branch if Overflow Clear */
        case 0x50:
            do_branch(cpu, !nes_cpu_get_flag(cpu, FLAG_OVERFLOW));
            break;

        /* BVS - Branch if Overflow Set */
        case 0x70:
            do_branch(cpu, nes_cpu_get_flag(cpu, FLAG_OVERFLOW));
            break;

        /* CLC - Clear Carry */
        case 0x18:
            nes_cpu_set_flag(cpu, FLAG_CARRY, 0);
            break;

        /* CLD - Clear Decimal */
        case 0xD8:
            nes_cpu_set_flag(cpu, FLAG_DECIMAL, 0);
            break;

        /* CLI - Clear Interrupt */
        case 0x58:
            nes_cpu_set_flag(cpu, FLAG_INTERRUPT, 0);
            break;

        /* CLV - Clear Overflow */
        case 0xB8:
            nes_cpu_set_flag(cpu, FLAG_OVERFLOW, 0);
            break;

        /* CMP - Compare */
        case 0xC9: case 0xC5: case 0xD5: case 0xCD: case 0xDD: case 0xD9:
        case 0xC1: case 0xD1:
            compare(cpu, cpu->reg.a, load_byte(cpu, info->mode));
            break;

        /* CPX - Compare X */
        case 0xE0: case 0xE4: case 0xEC:
            compare(cpu, cpu->reg.x, load_byte(cpu, info->mode));
            break;

        /* CPY - Compare Y */
        case 0xC0: case 0xC4: case 0xCC:
            compare(cpu, cpu->reg.y, load_byte(cpu, info->mode));
            break;

        /* DEC - Decrement Memory */
        case 0xC6: case 0xD6: case 0xCE: case 0xDE: {
            uint16_t addr = get_effective_address(cpu, info->mode);
            uint8_t val = (g_bus->read(g_bus->context, addr) - 1) & 0xFF;
            g_bus->write(g_bus->context, addr, val);
            nes_cpu_update_zn(cpu, val);
            break;
        }

        /* DEX - Decrement X */
        case 0xCA:
            cpu->reg.x = (cpu->reg.x - 1) & 0xFF;
            nes_cpu_update_zn(cpu, cpu->reg.x);
            break;

        /* DEY - Decrement Y */
        case 0x88:
            cpu->reg.y = (cpu->reg.y - 1) & 0xFF;
            nes_cpu_update_zn(cpu, cpu->reg.y);
            break;

        /* EOR - Exclusive OR */
        case 0x49: case 0x45: case 0x55: case 0x4D: case 0x5D: case 0x59:
        case 0x41: case 0x51:
            cpu->reg.a ^= load_byte(cpu, info->mode);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* INC - Increment Memory */
        case 0xE6: case 0xF6: case 0xEE: case 0xFE: {
            uint16_t addr = get_effective_address(cpu, info->mode);
            uint8_t val = (g_bus->read(g_bus->context, addr) + 1) & 0xFF;
            g_bus->write(g_bus->context, addr, val);
            nes_cpu_update_zn(cpu, val);
            break;
        }

        /* INX - Increment X */
        case 0xE8:
            cpu->reg.x = (cpu->reg.x + 1) & 0xFF;
            nes_cpu_update_zn(cpu, cpu->reg.x);
            break;

        /* INY - Increment Y */
        case 0xC8:
            cpu->reg.y = (cpu->reg.y + 1) & 0xFF;
            nes_cpu_update_zn(cpu, cpu->reg.y);
            break;

        /* JMP - Jump */
        case 0x4C: {
            uint16_t addr = get_effective_address(cpu, MODE_ABSOLUTE);
            cpu->reg.pc = addr;
            break;
        }
        case 0x6C: {
            uint16_t addr = get_effective_address(cpu, MODE_INDIRECT);
            cpu->reg.pc = addr;
            break;
        }

        /* JSR - Jump to Subroutine */
        case 0x20: {
            uint16_t addr = get_effective_address(cpu, MODE_ABSOLUTE);
            cpu->reg.pc--;  /* JSR pushes return address - 1 */
            nes_cpu_push_word(cpu, cpu->reg.pc);
            cpu->reg.pc = addr + 1;
            break;
        }

        /* LDA - Load Accumulator */
        case 0xA9: case 0xA5: case 0xB5: case 0xAD: case 0xBD: case 0xB9:
        case 0xA1: case 0xB1:
            cpu->reg.a = load_byte(cpu, info->mode);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* LDX - Load X */
        case 0xA2: case 0xA6: case 0xB6: case 0xAE: case 0xBE:
            cpu->reg.x = load_byte(cpu, info->mode);
            nes_cpu_update_zn(cpu, cpu->reg.x);
            break;

        /* LDY - Load Y */
        case 0xA0: case 0xA4: case 0xB4: case 0xAC: case 0xBC:
            cpu->reg.y = load_byte(cpu, info->mode);
            nes_cpu_update_zn(cpu, cpu->reg.y);
            break;

        /* LSR - Logical Shift Right */
        case 0x4A: {
            uint8_t carry;
            cpu->reg.a = do_lsr_cpu(cpu->reg.a, &carry);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;
        }
        case 0x46: case 0x56: case 0x4E: case 0x5E: {
            uint16_t addr = get_effective_address(cpu, info->mode);
            uint8_t val = g_bus->read(g_bus->context, addr);
            uint8_t carry;
            val = do_lsr_cpu(val, &carry);
            g_bus->write(g_bus->context, addr, val);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry);
            nes_cpu_update_zn(cpu, val);
            break;
        }

        /* NOP - No Operation */
        case 0xEA:
            break;

        /* ORA - Logical OR */
        case 0x09: case 0x05: case 0x15: case 0x0D: case 0x1D: case 0x19:
        case 0x01: case 0x11:
            cpu->reg.a |= load_byte(cpu, info->mode);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* PHA - Push Accumulator */
        case 0x48:
            nes_cpu_push(cpu, cpu->reg.a);
            break;

        /* PHP - Push Processor Status */
        case 0x08:
            nes_cpu_push(cpu, cpu->reg.p | FLAG_BREAK | FLAG_UNUSED);
            break;

        /* PLA - Pull Accumulator */
        case 0x68:
            cpu->reg.a = nes_cpu_pop(cpu);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* PLP - Pull Processor Status */
        case 0x28: {
            uint8_t status = nes_cpu_pop(cpu);
            cpu->reg.p = (status & ~(FLAG_BREAK | FLAG_UNUSED)) | FLAG_UNUSED;
            break;
        }

        /* ROL - Rotate Left */
        case 0x2A: {
            uint8_t carry_in = nes_cpu_get_flag(cpu, FLAG_CARRY);
            uint8_t carry_out;
            cpu->reg.a = do_rol(cpu->reg.a, carry_in, &carry_out);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry_out);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;
        }
        case 0x26: case 0x36: case 0x2E: case 0x3E: {
            uint16_t addr = get_effective_address(cpu, info->mode);
            uint8_t val = g_bus->read(g_bus->context, addr);
            uint8_t carry_in = nes_cpu_get_flag(cpu, FLAG_CARRY);
            uint8_t carry_out;
            val = do_rol(val, carry_in, &carry_out);
            g_bus->write(g_bus->context, addr, val);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry_out);
            nes_cpu_update_zn(cpu, val);
            break;
        }

        /* ROR - Rotate Right */
        case 0x6A: {
            uint8_t carry_in = nes_cpu_get_flag(cpu, FLAG_CARRY);
            uint8_t carry_out;
            cpu->reg.a = do_ror(cpu->reg.a, carry_in, &carry_out);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry_out);
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;
        }
        case 0x66: case 0x76: case 0x6E: case 0x7E: {
            uint16_t addr = get_effective_address(cpu, info->mode);
            uint8_t val = g_bus->read(g_bus->context, addr);
            uint8_t carry_in = nes_cpu_get_flag(cpu, FLAG_CARRY);
            uint8_t carry_out;
            val = do_ror(val, carry_in, &carry_out);
            g_bus->write(g_bus->context, addr, val);
            nes_cpu_set_flag(cpu, FLAG_CARRY, carry_out);
            nes_cpu_update_zn(cpu, val);
            break;
        }

        /* RTI - Return from Interrupt */
        case 0x40: {
            uint8_t status = nes_cpu_pop(cpu);
            cpu->reg.p = (status & ~(FLAG_BREAK | FLAG_UNUSED)) | FLAG_UNUSED;
            uint16_t pc = nes_cpu_pop_word(cpu);
            cpu->reg.pc = pc;
            break;
        }

        /* RTS - Return from Subroutine */
        case 0x60: {
            uint16_t pc = nes_cpu_pop_word(cpu);
            cpu->reg.pc = pc + 1;
            break;
        }

        /* SBC - Subtract with Carry */
        case 0xE9: case 0xE5: case 0xF5: case 0xED: case 0xFD: case 0xF9:
        case 0xE1: case 0xF1:
            do_sbc(cpu, load_byte(cpu, info->mode));
            break;

        /* SEC - Set Carry */
        case 0x38:
            nes_cpu_set_flag(cpu, FLAG_CARRY, 1);
            break;

        /* SED - Set Decimal */
        case 0xF8:
            nes_cpu_set_flag(cpu, FLAG_DECIMAL, 1);
            break;

        /* SEI - Set Interrupt */
        case 0x78:
            nes_cpu_set_flag(cpu, FLAG_INTERRUPT, 1);
            break;

        /* STA - Store Accumulator */
        case 0x85: case 0x95: case 0x8D: case 0x9D: case 0x99:
        case 0x81: case 0x91:
            store_byte(cpu, info->mode, cpu->reg.a);
            break;

        /* STX - Store X */
        case 0x86: case 0x96: case 0x8E:
            store_byte(cpu, info->mode, cpu->reg.x);
            break;

        /* STY - Store Y */
        case 0x84: case 0x94: case 0x8C:
            store_byte(cpu, info->mode, cpu->reg.y);
            break;

        /* TAX - Transfer A to X */
        case 0xAA:
            cpu->reg.x = cpu->reg.a;
            nes_cpu_update_zn(cpu, cpu->reg.x);
            break;

        /* TAY - Transfer A to Y */
        case 0xA8:
            cpu->reg.y = cpu->reg.a;
            nes_cpu_update_zn(cpu, cpu->reg.y);
            break;

        /* TSX - Transfer SP to X */
        case 0xBA:
            cpu->reg.x = cpu->reg.sp;
            nes_cpu_update_zn(cpu, cpu->reg.x);
            break;

        /* TXA - Transfer X to A */
        case 0x8A:
            cpu->reg.a = cpu->reg.x;
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* TXS - Transfer X to SP */
        case 0x9A:
            cpu->reg.sp = cpu->reg.x;
            /* TXS doesn't set flags */
            break;

        /* TYA - Transfer Y to A */
        case 0x98:
            cpu->reg.a = cpu->reg.y;
            nes_cpu_update_zn(cpu, cpu->reg.a);
            break;

        /* Unofficial opcodes - treat as NOP */
        default:
            /* Most unofficial opcodes behave as NOP or have unstable behavior */
            break;
    }

    cpu->cycle_count += cycles + cpu->stall_cycles;
    uint8_t total_cycles = cycles + cpu->stall_cycles;
    cpu->stall_cycles = 0;
    return total_cycles;
}

void nes_cpu_execute_cycles(nes_cpu_t* cpu, uint32_t cycles) {
    uint32_t executed = 0;
    while (executed < cycles) {
        executed += nes_cpu_step(cpu);
    }
}

void nes_cpu_trigger_nmi(nes_cpu_t* cpu) {
    cpu->pending_nmi = 1;
}

void nes_cpu_trigger_irq(nes_cpu_t* cpu) {
    cpu->pending_irq = 1;
}

void nes_cpu_set_bus(nes_cpu_t* cpu, cpu_bus_t* bus) {
    /* Copy bus configuration to static storage */
    if (bus) {
        g_cpu_bus_static.context = bus->context;
        g_cpu_bus_static.read = bus->read;
        g_cpu_bus_static.write = bus->write;
    }
    g_bus = &g_cpu_bus_static;
    (void)cpu;  /* unused */
}

void nes_cpu_disassemble(nes_cpu_t* cpu, uint16_t addr, char* buffer, size_t buffer_size) {
    (void)cpu;  /* unused */
    uint8_t opcode = g_bus->read(g_bus->context, addr);
    const opcode_info_t* info = &g_opcode_table[opcode];

    int len = snprintf(buffer, buffer_size, "$%04X: %s ", addr, info->mnemonic);

    /* Add operand based on addressing mode */
    switch (info->mode) {
        case MODE_ACCUMULATOR:
            snprintf(buffer + len, buffer_size - len, "A");
            break;
        case MODE_IMMEDIATE:
            snprintf(buffer + len, buffer_size - len, "#$%02X", g_bus->read(g_bus->context, addr + 1));
            break;
        case MODE_ZERO_PAGE:
            snprintf(buffer + len, buffer_size - len, "$%02X", g_bus->read(g_bus->context, addr + 1));
            break;
        case MODE_ZERO_PAGE_X:
            snprintf(buffer + len, buffer_size - len, "$%02X,X", g_bus->read(g_bus->context, addr + 1));
            break;
        case MODE_ZERO_PAGE_Y:
            snprintf(buffer + len, buffer_size - len, "$%02X,Y", g_bus->read(g_bus->context, addr + 1));
            break;
        case MODE_ABSOLUTE: {
            uint16_t addr16 = g_bus->read(g_bus->context, addr + 1);
            addr16 |= (uint16_t)g_bus->read(g_bus->context, addr + 2) << 8;
            snprintf(buffer + len, buffer_size - len, "$%04X", addr16);
            break;
        }
        case MODE_ABSOLUTE_X: {
            uint16_t addr16 = g_bus->read(g_bus->context, addr + 1);
            addr16 |= (uint16_t)g_bus->read(g_bus->context, addr + 2) << 8;
            snprintf(buffer + len, buffer_size - len, "$%04X,X", addr16);
            break;
        }
        case MODE_ABSOLUTE_Y: {
            uint16_t addr16 = g_bus->read(g_bus->context, addr + 1);
            addr16 |= (uint16_t)g_bus->read(g_bus->context, addr + 2) << 8;
            snprintf(buffer + len, buffer_size - len, "$%04X,Y", addr16);
            break;
        }
        case MODE_INDIRECT: {
            uint16_t addr16 = g_bus->read(g_bus->context, addr + 1);
            addr16 |= (uint16_t)g_bus->read(g_bus->context, addr + 2) << 8;
            snprintf(buffer + len, buffer_size - len, "($%04X)", addr16);
            break;
        }
        case MODE_INDEXED_INDIRECT: {
            uint16_t ptr = g_bus->read(g_bus->context, addr + 1);
            snprintf(buffer + len, buffer_size - len, "($%02X,X)", ptr);
            break;
        }
        case MODE_INDIRECT_INDEXED: {
            uint16_t ptr = g_bus->read(g_bus->context, addr + 1);
            snprintf(buffer + len, buffer_size - len, "($%02X),Y", ptr);
            break;
        }
        case MODE_RELATIVE: {
            int8_t offset = (int8_t)g_bus->read(g_bus->context, addr + 1);
            snprintf(buffer + len, buffer_size - len, "$%04X", (uint16_t)(addr + 2 + offset));
            break;
        }
        default:
            break;
    }
}
