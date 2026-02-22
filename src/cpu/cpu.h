/**
 * NESPRESSO - NES Emulator
 * CPU Module - 6502 Processor Emulation
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_CPU_H
#define NESPRESSO_CPU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CPU Constants */
#define NES_CPU_CLOCK_NTSC   1789773  // ~1.79 MHz (NTSC)
#define NES_CPU_CLOCK_PAL    1662607  // ~1.66 MHz (PAL)
#define NES_STACK_BASE       0x0100
#define NES_VECTOR_NMI       0xFFFA   // Non-Maskable Interrupt
#define NES_VECTOR_RESET     0xFFFC   // Reset Vector
#define NES_VECTOR_IRQ_BRK   0xFFFE   // IRQ/BRK Vector

/* Status Register Flags */
typedef enum {
    FLAG_CARRY     = 0x01,  // C
    FLAG_ZERO      = 0x02,  // Z
    FLAG_INTERRUPT = 0x04,  // I (IRQ Disable)
    FLAG_DECIMAL   = 0x08,  // D (Decimal Mode - unused on NES)
    FLAG_BREAK     = 0x10,  // B (BRK flag)
    FLAG_UNUSED    = 0x20,  // Always 1
    FLAG_OVERFLOW  = 0x40,  // V
    FLAG_NEGATIVE  = 0x80   // N
} cpu_flag_t;

/* CPU Registers */
typedef struct {
    uint16_t pc;           // Program Counter
    uint8_t  a;            // Accumulator
    uint8_t  x;            // X Index Register
    uint8_t  y;            // Y Index Register
    uint8_t  sp;           // Stack Pointer
    uint8_t  p;            // Status Register
} cpu_registers_t;

/* Addressing Modes */
typedef enum {
    MODE_IMPLIED,          // No operand
    MODE_ACCUMULATOR,      // Uses accumulator
    MODE_IMMEDIATE,        // #$ operand
    MODE_ZERO_PAGE,        // $ operand
    MODE_ZERO_PAGE_X,      // $ operand, X
    MODE_ZERO_PAGE_Y,      // $ operand, Y
    MODE_ABSOLUTE,         // $ operand
    MODE_ABSOLUTE_X,       // $ operand, X
    MODE_ABSOLUTE_Y,       // $ operand, Y
    MODE_INDIRECT,         // ($ operand)
    MODE_INDEXED_INDIRECT, // ($ operand, X)
    MODE_INDIRECT_INDEXED, // ($ operand), Y
    MODE_RELATIVE          // label
} addr_mode_t;

/* Opcode Information */
typedef struct {
    const char*   mnemonic;
    addr_mode_t   mode;
    uint8_t       cycles;
    uint8_t       length;
} opcode_info_t;

/* CPU State */
typedef struct nes_cpu {
    cpu_registers_t  reg;
    uint32_t         cycle_count;
    uint8_t          pending_nmi;
    uint8_t          pending_irq;
    uint8_t          stall_cycles;
    const opcode_info_t* opcode_table;
} nes_cpu_t;

/* Memory Read/Write Callback Types */
typedef uint8_t (*cpu_read_func)(void* ctx, uint16_t addr);
typedef void  (*cpu_write_func)(void* ctx, uint16_t addr, uint8_t val);

typedef struct {
    void*           context;
    cpu_read_func   read;
    cpu_write_func  write;
} cpu_bus_t;

/* CPU API */

/**
 * Initialize the CPU with initial state
 */
void nes_cpu_init(nes_cpu_t* cpu, cpu_bus_t* bus);

/**
 * Reset the CPU to initial state
 */
void nes_cpu_reset(nes_cpu_t* cpu);

/**
 * Execute a single CPU instruction
 * Returns number of cycles consumed
 */
uint8_t nes_cpu_step(nes_cpu_t* cpu);

/**
 * Execute N CPU cycles (for frame timing)
 */
void nes_cpu_execute_cycles(nes_cpu_t* cpu, uint32_t cycles);

/**
 * Trigger a Non-Maskable Interrupt (NMI)
 */
void nes_cpu_trigger_nmi(nes_cpu_t* cpu);

/**
 * Trigger an IRQ (can be masked)
 */
void nes_cpu_trigger_irq(nes_cpu_t* cpu);

/**
 * Disassemble instruction at address for debugging
 */
void nes_cpu_disassemble(nes_cpu_t* cpu, uint16_t addr, char* buffer, size_t buffer_size);

/**
 * Set CPU bus for memory access
 */
void nes_cpu_set_bus(nes_cpu_t* cpu, cpu_bus_t* bus);

/* Global CPU bus pointer (defined in cpu.c) */
extern cpu_bus_t* g_bus;

/* Address mode helpers */

static inline uint8_t nes_cpu_get_flag(nes_cpu_t* cpu, uint8_t flag) {
    return (cpu->reg.p & flag) ? 1 : 0;
}

static inline void nes_cpu_set_flag(nes_cpu_t* cpu, uint8_t flag, uint8_t value) {
    if (value)
        cpu->reg.p |= flag;
    else
        cpu->reg.p &= ~flag;
}

/* Flag setting helpers for common operations */

/**
 * Update Zero and Negative flags based on a value
 */
static inline void nes_cpu_update_zn(nes_cpu_t* cpu, uint8_t val) {
    nes_cpu_set_flag(cpu, FLAG_ZERO, (val == 0));
    nes_cpu_set_flag(cpu, FLAG_NEGATIVE, (val & 0x80) != 0);
}

/* Stack operations */

static inline void nes_cpu_push(nes_cpu_t* cpu, uint8_t val) {
    g_bus->write(g_bus->context, NES_STACK_BASE + cpu->reg.sp--, val);
}

static inline uint8_t nes_cpu_pop(nes_cpu_t* cpu) {
    return g_bus->read(g_bus->context, NES_STACK_BASE + ++cpu->reg.sp);
}

static inline void nes_cpu_push_word(nes_cpu_t* cpu, uint16_t val) {
    nes_cpu_push(cpu, (val >> 8) & 0xFF);
    nes_cpu_push(cpu, val & 0xFF);
}

static inline uint16_t nes_cpu_pop_word(nes_cpu_t* cpu) {
    uint8_t lo = nes_cpu_pop(cpu);
    uint8_t hi = nes_cpu_pop(cpu);
    return (hi << 8) | lo;
}

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_CPU_H */
