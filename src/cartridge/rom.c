/**
 * NESPRESSO - NES Emulator
 * Cartridge Module - ROM Loading and Parsing Implementation
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "rom.h"
#include "../mapper/mapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRG_RAM_SIZE 8192  /* Standard 8KB */

/* CRC32 Lookup Table */
static uint32_t g_crc32_table[256];

static void init_crc32_table(void) {
    static int initialized = 0;
    if (initialized) return;
    initialized = 1;

    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        g_crc32_table[i] = crc;
    }
}

static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t length) {
    crc = ~crc;
    for (size_t i = 0; i < length; i++) {
        crc = g_crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

void nes_cartridge_init(nes_cartridge_t* cart) {
    memset(cart, 0, sizeof(nes_cartridge_t));
    init_crc32_table();
}

void nes_cartridge_free(nes_cartridge_t* cart) {
    if (cart->prg_rom) {
        free(cart->prg_rom);
        cart->prg_rom = NULL;
    }
    if (cart->chr_rom) {
        free(cart->chr_rom);
        cart->chr_rom = NULL;
    }
    if (cart->info.prg_rom) {
        free(cart->info.prg_rom);
        cart->info.prg_rom = NULL;
    }
    if (cart->info.chr_rom) {
        free(cart->info.chr_rom);
        cart->info.chr_rom = NULL;
    }
}

/* Parse header and extract info */
static nes_rom_result_t parse_header(const nes_rom_header_t* header, nes_cartridge_t* cart) {
    /* Check magic number */
    if (memcmp(header->magic, NES_MAGIC, 4) != 0) {
        return NES_ROM_ERROR_INVALID_HEADER;
    }

    /* Get mapper number */
    uint8_t mapper_lo = (header->flags6 & FLAGS6_MAPPER_LO) >> 4;
    uint8_t mapper_hi = (header->flags7 & FLAGS7_MAPPER_HI) >> 4;
    cart->info.mapper = mapper_hi << 4 | mapper_lo;

    /* Check NES 2.0 */
    cart->info.is_nes_2_0 = ((header->flags7 & FLAGS7_NES_2_0) == FLAGS7_NES_2_0);

    /* Get ROM sizes */
    cart->info.prg_rom_banks = header->prg_rom_size;
    cart->info.prg_rom_size = cart->info.prg_rom_banks * NES_PRG_ROM_SIZE;

    if (cart->info.is_nes_2_0) {
        /* NES 2.0 format */
        uint8_t chr_lo = header->chr_rom_size & 0x0F;
        uint8_t chr_hi = (header->chr_rom_size >> 4) & 0xF0;
        cart->info.chr_rom_banks = chr_hi << 4 | chr_lo;
    } else {
        cart->info.chr_rom_banks = header->chr_rom_size;
    }
    cart->info.chr_rom_size = cart->info.chr_rom_banks * NES_CHR_ROM_SIZE;

    cart->info.has_chrram = (cart->info.chr_rom_banks == 0);

    /* Mirroring */
    if (header->flags6 & FLAGS6_FOUR_SCREEN) {
        cart->info.mirroring = MIRROR_FOUR_SCREEN;
        cart->info.fourscreen = 1;
    } else if (header->flags6 & FLAGS6_MIRROR_MASK) {
        cart->info.mirroring = MIRROR_VERTICAL;
    } else {
        cart->info.mirroring = MIRROR_HORIZONTAL;
    }

    /* Battery */
    cart->info.has_battery = (header->flags6 & FLAGS6_BATTERY) != 0;

    /* Trainer */
    cart->info.has_trainer = (header->flags6 & FLAGS6_TRAINER) != 0;

    /* VS/PlayChoice */
    cart->info.vs_unisystem = (header->flags7 & FLAGS7_VS_UNISYSTEM) != 0;
    cart->info.playchoice = (header->flags7 & FLAGS7_PLAYCHOICE_10) != 0;

    /* PAL detection */
    cart->info.is_pal = (header->flags10 & FLAGS10_TV_SYSTEM) & 0x01;

    /* Allocate PRG-ROM */
    cart->prg_rom = (uint8_t*)malloc(cart->info.prg_rom_size);
    if (!cart->prg_rom) {
        return NES_ROM_ERROR_MEMORY;
    }

    /* Allocate CHR or CHR-RAM */
    if (cart->info.chr_rom_size > 0) {
        cart->chr_rom = (uint8_t*)malloc(cart->info.chr_rom_size);
        if (!cart->chr_rom) {
            free(cart->prg_rom);
            cart->prg_rom = NULL;
            return NES_ROM_ERROR_MEMORY;
        }
    } else {
        /* CHR-RAM: allocate 8KB */
        cart->chr_rom_size = NES_CHR_ROM_SIZE;
        cart->chr_rom = (uint8_t*)calloc(1, cart->chr_rom_size);
        if (!cart->chr_rom) {
            free(cart->prg_rom);
            cart->prg_rom = NULL;
            return NES_ROM_ERROR_MEMORY;
        }
    }

    /* Allocate PRG-RAM (save RAM) */
    if (cart->info.has_battery || 1) {
        cart->prg_ram_size = PRG_RAM_SIZE;
        cart->prg_ram = (uint8_t*)calloc(1, cart->prg_ram_size);
    }

    /* Set pointers in info */
    cart->info.prg_rom = cart->prg_rom;
    cart->info.chr_rom = cart->chr_rom;

    return NES_ROM_OK;
}

nes_rom_result_t nes_cartridge_load_memory(nes_cartridge_t* cart, const uint8_t* data, size_t size) {
    if (size < NES_ROM_HEADER_SIZE) {
        return NES_ROM_ERROR_TRUNCATED;
    }

    nes_rom_result_t result = parse_header((const nes_rom_header_t*)data, cart);
    if (result != NES_ROM_OK) {
        return result;
    }

    size_t offset = NES_ROM_HEADER_SIZE;
    size_t rom_size = offset + cart->info.prg_rom_size + cart->info.chr_rom_size;

    /* Check if trainer present */
    if (cart->info.has_trainer) {
        offset += 512;
        rom_size += 512;
    }

    /* Check if we have enough data */
    if (size < rom_size) {
        nes_cartridge_free(cart);
        return NES_ROM_ERROR_TRUNCATED;
    }

    /* Copy PRG-ROM */
    memcpy(cart->prg_rom, data + offset, cart->info.prg_rom_size);
    offset += cart->info.prg_rom_size;

    /* Copy CHR-ROM */
    if (cart->info.chr_rom_banks > 0) {
        memcpy(cart->chr_rom, data + offset, cart->info.chr_rom_size);
    }

    /* Calculate CRC32 */
    cart->info.crc32 = crc32_update(0, cart->prg_rom, cart->info.prg_rom_size);
    if (cart->info.chr_rom_banks > 0) {
        cart->info.crc32 = crc32_update(cart->info.crc32, cart->chr_rom, cart->info.chr_rom_size);
    }

    return NES_ROM_OK;
}

nes_rom_result_t nes_cartridge_load(nes_cartridge_t* cart, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return NES_ROM_ERROR_FILE_NOT_FOUND;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < NES_ROM_HEADER_SIZE) {
        fclose(f);
        return NES_ROM_ERROR_TRUNCATED;
    }

    /* Read entire file */
    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer) {
        fclose(f);
        return NES_ROM_ERROR_MEMORY;
    }

    size_t read = fread(buffer, 1, size, f);
    fclose(f);

    if (read != (size_t)size) {
        free(buffer);
        return NES_ROM_ERROR_TRUNCATED;
    }

    nes_rom_result_t result = nes_cartridge_load_memory(cart, buffer, size);
    free(buffer);

    return result;
}

int nes_cartridge_get_mapper(const nes_cartridge_t* cart) {
    return cart->info.mapper;
}

int nes_cartridge_has_battery(const nes_cartridge_t* cart) {
    return cart->info.has_battery;
}

mirroring_t nes_cartridge_get_mirroring(const nes_cartridge_t* cart) {
    return cart->info.mirroring;
}

uint8_t nes_cartridge_read_prg(const nes_cartridge_t* cart, uint32_t addr) {
    addr &= 0x7FFF;  /* 32KB mask */

    if (addr < cart->info.prg_rom_size) {
        return cart->prg_rom[addr];
    }

    /* Mirror PRG-ROM */
    return cart->prg_rom[addr % cart->info.prg_rom_size];
}

void nes_cartridge_write_prg_ram(nes_cartridge_t* cart, uint32_t addr, uint8_t val) {
    if (cart->prg_ram && addr < cart->prg_ram_size) {
        cart->prg_ram[addr] = val;
    }
}

uint8_t nes_cartridge_read_prg_ram(const nes_cartridge_t* cart, uint32_t addr) {
    if (cart->prg_ram && addr < cart->prg_ram_size) {
        return cart->prg_ram[addr];
    }
    return 0;
}

uint8_t nes_cartridge_read_chr(const nes_cartridge_t* cart, uint32_t addr) {
    addr &= cart->chr_rom_size - 1;
    return cart->chr_rom[addr];
}

void nes_cartridge_write_chr_ram(nes_cartridge_t* cart, uint32_t addr, uint8_t val) {
    if (cart->info.has_chrram) {
        addr &= cart->chr_rom_size - 1;
        cart->chr_rom[addr] = val;
    }
}

int nes_cartridge_load_sram(nes_cartridge_t* cart, const char* filename) {
    if (!cart->prg_ram || cart->prg_ram_size == 0) {
        return 0;
    }

    FILE* f = fopen(filename, "rb");
    if (!f) {
        return 0;
    }

    size_t read = fread(cart->prg_ram, 1, cart->prg_ram_size, f);
    fclose(f);

    return (read == cart->prg_ram_size);
}

int nes_cartridge_save_sram(const nes_cartridge_t* cart, const char* filename) {
    if (!cart->prg_ram || cart->prg_ram_size == 0) {
        return 0;
    }

    FILE* f = fopen(filename, "wb");
    if (!f) {
        return 0;
    }

    size_t written = fwrite(cart->prg_ram, 1, cart->prg_ram_size, f);
    fclose(f);

    return (written == cart->prg_ram_size);
}

uint32_t nes_cartridge_calc_crc32(const nes_cartridge_t* cart) {
    uint32_t crc = crc32_update(0, cart->prg_rom, cart->info.prg_rom_size);
    if (cart->info.chr_rom_banks > 0) {
        crc = crc32_update(crc, cart->chr_rom, cart->info.chr_rom_size);
    }
    return crc;
}

void nes_cartridge_get_info_string(const nes_cartridge_t* cart, char* buffer, size_t size) {
    const char* mirroring_str[] = {
        "Horizontal", "Vertical", "Single Screen 0", "Single Screen 1", "Four Screen"
    };

    snprintf(buffer, size,
        "Mapper: %d | PRG: %d banks (%.1f KB) | CHR: %d banks (%.1f %s) | Mirror: %s | Battery: %s",
        cart->info.mapper,
        cart->info.prg_rom_banks,
        cart->info.prg_rom_banks * 16.0,
        cart->info.chr_rom_banks,
        cart->info.chr_rom_banks * 8.0,
        cart->info.has_chrram ? "KB RAM" : "KB ROM",
        mirroring_str[cart->info.mirroring],
        cart->info.has_battery ? "Yes" : "No"
    );
}
