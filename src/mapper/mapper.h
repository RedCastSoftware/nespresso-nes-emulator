/**
 * NESPRESSO - NES Emulator
 * Mapper Module - Cartridge Mapper Interface
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_MAPPER_H
#define NESPRESSO_MAPPER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct nes_cartridge nes_cartridge_t;
typedef struct nes_ppu nes_ppu_t;

/* Mapper Callback Types */
typedef uint8_t (*mapper_read_cpu_t)(void* ctx, uint16_t addr);
typedef void    (*mapper_write_cpu_t)(void* ctx, uint16_t addr, uint8_t val);
typedef uint8_t (*mapper_read_ppu_t)(void* ctx, uint16_t addr);
typedef void    (*mapper_write_ppu_t)(void* ctx, uint16_t addr, uint8_t val);

/* Mapper State Buffer (for save states) */
typedef struct {
    uint8_t data[256];  /* Internal mapper state */
    uint8_t size;       /* Actual used size */
} mapper_buffer_t;

/* Mapper Interface */
typedef struct nes_mapper {
    int             number;            /* Mapper number */

    /* CPU bus access */
    mapper_read_cpu_t   cpu_read;
    mapper_write_cpu_t  cpu_write;

    /* PPU bus access */
    mapper_read_ppu_t   ppu_read;
    mapper_write_ppu_t  ppu_write;

    /* Callbacks for system events */
    void (*reset)(void* ctx);
    void (*scanline)(void* ctx);       /* Called at end of scanline */
    void (*clock_irq)(void* ctx);      /* For MMC3 IRQ, etc. */

    /* Context pointer */
    void*   context;

    /* State for save/load */
    mapper_buffer_t state;
} nes_mapper_t;

/* Mapper Creation Functions - returns 0 on success */

/**
 * Create mapper instance for a given cartridge
 */
int nes_mapper_create(nes_cartridge_t* cart, nes_mapper_t* mapper, nes_ppu_t* ppu);

/**
 * Destroy mapper instance
 */
void nes_mapper_destroy(nes_mapper_t* mapper);

/**
 * Reset mapper to initial state
 */
void nes_mapper_reset(nes_mapper_t* mapper);

/* Individual mapper implementations */

/* Mapper 0 (NROM) */
int mapper_0_init(nes_cartridge_t* cart, nes_mapper_t* mapper);

/* Mapper 1 (MMC1) */
int mapper_1_init(nes_cartridge_t* cart, nes_mapper_t* mapper, nes_ppu_t* ppu);

/* Mapper 2 (UxROM) */
int mapper_2_init(nes_cartridge_t* cart, nes_mapper_t* mapper);

/* Mapper 3 (CNROM) */
int mapper_3_init(nes_cartridge_t* cart, nes_mapper_t* mapper);

/* Mapper 4 (MMC3) */
int mapper_4_init(nes_cartridge_t* cart, nes_mapper_t* mapper, nes_ppu_t* ppu);

/* Mapper 7 (AxROM) */
int mapper_7_init(nes_cartridge_t* cart, nes_mapper_t* mapper);

/* Helper for mapper-specific mirroring */
void nes_mapper_set_mirroring(nes_mapper_t* mapper, int mode);

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_MAPPER_H */
