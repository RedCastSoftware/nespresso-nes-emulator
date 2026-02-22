/**
 * NESPRESSO - NES Emulator
 * Cartridge Module - ROM Loading and Parsing
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_ROM_H
#define NESPRESSO_ROM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ROM Constants */
#define NES_ROM_HEADER_SIZE  16
#define NES_PRG_ROM_SIZE     16384   /* 16KB */
#define NES_CHR_ROM_SIZE     8192    /* 8KB */
#define NES_MAX_PRG_ROM      512     /* Max 8MB */
#define NES_MAX_CHR_ROM      512     /* Max 4MB */
#define NES_MAX_SIZE         (NES_MAX_PRG_ROM * NES_PRG_ROM_SIZE + NES_MAX_CHR_ROM * NES_CHR_ROM_SIZE)

/* NES Header Magic */
#define NES_MAGIC "NES\x1A"

/* iNES Header Format */
typedef struct {
    uint8_t  magic[4];        /* "NES\x1A" */
    uint8_t  prg_rom_size;   /* PRG ROM in 16KB units */
    uint8_t  chr_rom_size;   /* CHR ROM in 8KB units (0 = CHR RAM) */
    uint8_t  flags6;         /* Flags 6 */
    uint8_t  flags7;         /* Flags 7 */
    uint8_t  flags8;         /* PRG RAM size (NES 2.0) */
    uint8_t  flags9;         /* Flags 9 */
    uint8_t  flags10;        /* Flags 10 */
    uint8_t  unused[5];      /* Unused padding */
} nes_rom_header_t;

/* Flags 6 bits */
#define FLAGS6_MIRROR_MASK     0x01    /* 0=horizontal, 1=vertical */
#define FLAGS6_BATTERY         0x02    /* Battery-backed RAM */
#define FLAGS6_TRAINER         0x04    /* 512-byte trainer */
#define FLAGS6_FOUR_SCREEN     0x08    /* Four-screen VRAM */
#define FLAGS6_MAPPER_LO       0xF0    /* Low nibble of mapper */

/* Flags 7 bits */
#define FLAGS7_VS_UNISYSTEM    0x01    /* VS Unisystem */
#define FLAGS7_PLAYCHOICE_10   0x02    /* PlayChoice-10 */
#define FLAGS7_NES_2_0         0x0C    /* NES 2.0 header */
#define FLAGS7_MAPPER_HI       0xF0    /* High nibble of mapper */

/* Flags 10 bits */
#define FLAGS10_TV_SYSTEM      0x03    /* 0=NTSC, 1=PAL */
#define FLAGS10_PRG_RAM        0x10    /* PRG RAM in console */

/* Mirroring modes */
typedef enum {
    MIRROR_HORIZONTAL,
    MIRROR_VERTICAL,
    MIRROR_SINGLE_0,
    MIRROR_SINGLE_1,
    MIRROR_FOUR_SCREEN
} mirroring_t;

/* ROM Information */
typedef struct {
    uint8_t         mapper;         /* Mapper number */
    int             submapper;      /* Submapper (NES 2.0) */
    int             prg_rom_banks;  /* Number of PRG-ROM banks */
    int             chr_rom_banks;  /* Number of CHR-ROM banks */
    int             has_chrram;     /* Has CHR-RAM instead of CHR-ROM */
    int             has_battery;    /* Has battery-backed SRAM */
    int             has_trainer;    /* Has 512-byte trainer */
    mirroring_t     mirroring;      /* Mirroring mode */
    int             fourscreen;     /* Four-screen mode */
    int             vs_unisystem;   /* VS Unisystem game */
    int             playchoice;     /* PlayChoice-10 game */
    int             is_nes_2_0;     /* NES 2.0 header */
    int             is_pal;         /* PAL system */
    uint8_t*        prg_rom;        /* PRG-ROM data */
    uint8_t*        chr_rom;        /* CHR-ROM/RAM data */
    uint32_t        crc32;          /* ROM CRC32 */
    char            title[256];     /* ROM title (from database) */
} nes_rom_info_t;

/* Cartridge Structure */
typedef struct nes_cartridge {
    nes_rom_info_t  info;
    uint8_t*        prg_rom;
    uint8_t*        chr_rom;
    uint8_t*        prg_ram;        /* Save RAM (8KB standard) */
    size_t          prg_rom_size;
    size_t          chr_rom_size;
    size_t          prg_ram_size;
} nes_cartridge_t;

/* Loading Result */
typedef enum {
    NES_ROM_OK,
    NES_ROM_ERROR_FILE_NOT_FOUND,
    NES_ROM_ERROR_INVALID_HEADER,
    NES_ROM_ERROR_UNSUPPORTED_MAPPER,
    NES_ROM_ERROR_MEMORY,
    NES_ROM_ERROR_TRUNCATED,
    NES_ROM_ERROR_CHECKSUM
} nes_rom_result_t;

/* API Functions */

/**
 * Initialize cartridge structure
 */
void nes_cartridge_init(nes_cartridge_t* cart);

/**
 * Free cartridge resources
 */
void nes_cartridge_free(nes_cartridge_t* cart);

/**
 * Load NES ROM from file
 */
nes_rom_result_t nes_cartridge_load(nes_cartridge_t* cart, const char* filename);

/**
 * Load NES ROM from memory buffer
 */
nes_rom_result_t nes_cartridge_load_memory(nes_cartridge_t* cart, const uint8_t* data, size_t size);

/**
 * Get mapper number from ROM
 */
int nes_cartridge_get_mapper(const nes_cartridge_t* cart);

/**
 * Check if ROM has battery-backed SRAM
 */
int nes_cartridge_has_battery(const nes_cartridge_t* cart);

/**
 * Get mirroring mode
 */
mirroring_t nes_cartridge_get_mirroring(const nes_cartridge_t* cart);

/**
 * Read PRG-ROM at address
 */
uint8_t nes_cartridge_read_prg(const nes_cartridge_t* cart, uint32_t addr);

/**
 * Write PRG-RAM at address
 */
void nes_cartridge_write_prg_ram(nes_cartridge_t* cart, uint32_t addr, uint8_t val);

/**
 * Read PRG-RAM at address
 */
uint8_t nes_cartridge_read_prg_ram(const nes_cartridge_t* cart, uint32_t addr);

/**
 * Read CHR-ROM/RAM at address
 */
uint8_t nes_cartridge_read_chr(const nes_cartridge_t* cart, uint32_t addr);

/**
 * Write CHR-RAM (if present) at address
 */
void nes_cartridge_write_chr_ram(nes_cartridge_t* cart, uint32_t addr, uint8_t val);

/**
 * Load SRAM from file
 */
int nes_cartridge_load_sram(nes_cartridge_t* cart, const char* filename);

/**
 * Save SRAM to file
 */
int nes_cartridge_save_sram(const nes_cartridge_t* cart, const char* filename);

/**
 * Calculate CRC32 of PRG+CHR data
 */
uint32_t nes_cartridge_calc_crc32(const nes_cartridge_t* cart);

/**
 * Get ROM info string
 */
void nes_cartridge_get_info_string(const nes_cartridge_t* cart, char* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_ROM_H */
