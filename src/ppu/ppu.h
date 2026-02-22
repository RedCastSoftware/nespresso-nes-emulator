/**
 * NESPRESSO - NES Emulator
 * PPU Module - Picture Processing Unit
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_PPU_H
#define NESPRESSO_PPU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PPU Constants */
#define PPU_WIDTH         256
#define PPU_HEIGHT        240
#define PPU_SCANLINES     262
#define PPU_VISIBLE_SCANLINES 240

#define PPU_VRAM_SIZE     0x1000   /* 4KB VRAM */
#define PPU_PALETTE_SIZE  0x20     /* 32 palette entries */
#define PPU_OAM_SIZE      0x100    /* 256 bytes OAM */
#define PPU_SPRITE_COUNT  64

/* PPU Registers */
#define PPUCTRL          0x2000
#define PPUMASK          0x2001
#define PPUSTATUS        0x2002
#define OAMADDR          0x2003
#define OAMDATA          0x2004
#define PPUSCROLL        0x2005
#define PPUADDR          0x2006
#define PPUDATA          0x2007
#define OAMDMA           0x4014

/* PPUCTRL Register Flags ($2000) */
#define PPUCTRL_NMI       0x80   /* Generate NMI at VBlank */
#define PPUCTRL_SP_SIZE   0x20   /* Sprite size (0=8x8, 1=8x16) */
#define PPUCTRL_BG_ADDR   0x10   /* BG pattern table (0=$0000, 1=$1000) */
#define PPUCTRL_SP_ADDR   0x08   /* Sprite pattern table (0=$0000, 1=$1000) */
#define PPUCTRL_INC32     0x04   /* Increment mode (0=+1, 1=+32) */

/* PPUMASK Register Flags ($2001) */
#define PPUMASK_EMPHASIS  0xE0   /* Color emphasis */
#define PPUMASK_SHOW_BGR  0x08   /* Show background */
#define PPUMASK_SHOW_SPR  0x10   /* Show sprites */
#define PPUMASK_SHOW_BGR8 0x02   /* Show left 8px of background */
#define PPUMASK_SHOW_SPR8 0x04   /* Show left 8px of sprites */
#define PPUMASK_GRAYSCALE 0x01   /* Grayscale mode */

/* PPUSTATUS Register Flags ($2002) */
#define PPUSTATUS_VBLANK  0x80   /* VBlank flag */
#define PPUSTATUS_SP0_HIT 0x40   /* Sprite 0 hit */
#define PPUSTATUS_SP_OVF  0x20   /* Sprite overflow */

/* Sprite Attribute Bits */
#define SP_ATTR_PRIORITY  0x20   /* Priority (0=front, 1=back) */
#define SP_ATTR_FLIP_H    0x40   /* Horizontal flip */
#define SP_ATTR_FLIP_V    0x80   /* Vertical flip */
#define SP_ATTR_PAL_MASK  0x03   /* Palette bits */

/* PPU Scroll Position (v, t, w, x) */
typedef struct {
    uint16_t v;      /* Current VRAM address (15 bits) */
    uint16_t t;      /* Temporary VRAM address (15 bits) */
    uint8_t  x;      /* Fine X scroll (3 bits) */
    uint8_t  w;      /* Write toggle (1 bit) */
} ppu_scroll_t;

/* Sprite Entry (4 bytes in OAM) */
typedef struct {
    uint8_t  y;       /* Y position */
    uint8_t  tile;    /* Tile number */
    uint8_t  attr;    /* Attributes */
    uint8_t  x;       /* X position */
} ppu_sprite_t;

/* PPU Registers */
typedef struct {
    uint8_t  ctrl;
    uint8_t  mask;
    uint8_t  status;
    uint8_t  oam_addr;

    /* Internal registers */
    uint8_t  data_buffer;   /* PPUDATA read buffer */
    ppu_scroll_t scroll;
} ppu_registers_t;

/* PPU State */
typedef struct nes_ppu {
    ppu_registers_t reg;

    /* Memory */
    uint8_t         vram[PPU_VRAM_SIZE];        /* 4KB VRAM */
    uint8_t         palette[PPU_PALETTE_SIZE];  /* Palette RAM ($3F00-$3F1F) */
    uint8_t         oam[PPU_OAM_SIZE];          /* OAM */
    uint8_t         secondary_oam[PPU_OAM_SIZE];/* Secondary OAM for rendering */

    /* Scanline timing */
    uint16_t        scanline;
    uint16_t        cycle;
    uint32_t        frame;
    uint8_t         odd_frame;

    /* Sprite detection */
    uint8_t         sprite_count;
    uint8_t         sprite_pattern[8];
    ppu_sprite_t    sprites[8];     /* Sprites on current scanline */

    /* Rendering buffers */
    uint8_t         background_shift_lo;
    uint8_t         background_shift_hi;
    uint8_t         attribute_shift_lo;
    uint8_t         attribute_shift_hi;
    uint8_t         attribute_latch_lo;
    uint8_t         attribute_latch_hi;
    uint8_t         background_fetch_tile;
    uint8_t         background_fetch_attr;

    /* External callbacks */
    void*           context;
    void (*render_scanline)(void* ctx, int scanline);
    void (*vblank_callback)(void* ctx);
} nes_ppu_t;

/* PPU Bus Interface */
typedef struct {
    void*   context;
    uint8_t (*read_chr)(void* ctx, uint16_t addr);
    void    (*write_chr)(void* ctx, uint16_t addr, uint8_t val);
    void    (*ppu_write_cpu)(void* ctx, uint16_t addr, uint8_t val);
} ppu_bus_t;

/* API Functions */

/**
 * Initialize PPU
 */
void nes_ppu_init(nes_ppu_t* ppu, ppu_bus_t* bus);

/**
 * Reset PPU to initial state
 */
void nes_ppu_reset(nes_ppu_t* ppu);

/**
 * Step PPU by one cycle
 * Returns 1 if frame completed (VBlank just started)
 */
int nes_ppu_step(nes_ppu_t* ppu);

/**
 * Execute N PPU cycles
 */
void nes_ppu_execute_cycles(nes_ppu_t* ppu, int cycles);

/**
 * Write to PPU register from CPU
 */
void nes_ppu_cpu_write(nes_ppu_t* ppu, uint16_t addr, uint8_t val);

/**
 * Read from PPU register via CPU
 */
uint8_t nes_ppu_cpu_read(nes_ppu_t* ppu, uint16_t addr);

/**
 * Render complete frame to RGBA buffer
 * buffer must be at least PPU_WIDTH * PPU_HEIGHT * 4 bytes
 */
void nes_ppu_render_frame(nes_ppu_t* ppu, uint32_t* buffer);

/**
 * Get current frame buffer (raw palette indices)
 * Returns pointer to internal rendering buffer
 */
const uint8_t* nes_ppu_get_frame_buffer(nes_ppu_t* ppu);

/**
 * Get palette color in RGBA format
 */
uint32_t nes_ppu_get_color(nes_ppu_t* ppu, uint8_t palette_idx);

/**
 * Set PPU bus for CHR-ROM/RAM access
 */
void nes_ppu_set_bus(nes_ppu_t* ppu, ppu_bus_t* bus);

/* Helper Functions */

/* Mirror mode handling */
uint16_t nes_ppu_mirror_address(uint8_t mirror_mode, uint16_t addr);

/* Get name table address based on mirror mode */
uint16_t nes_ppu_nametable_addr(uint8_t mirror_mode, uint16_t addr);

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_PPU_H */
