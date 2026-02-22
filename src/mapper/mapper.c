/**
 * NESPRESSO - NES Emulator
 * Mapper Module - Cartridge Mapper Implementations
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "mapper.h"
#include "../cartridge/rom.h"
#include <string.h>
#include <stdio.h>

/* Global PPU reference for mirroring control */
static nes_ppu_t* g_ppu = NULL;

/* Mapper 0 (NROM) - No mapping */

typedef struct {
    nes_cartridge_t* cart;
} mapper_0_ctx_t;

static uint8_t mapper_0_cpu_read(void* ctx, uint16_t addr) {
    mapper_0_ctx_t* m = (mapper_0_ctx_t*)ctx;
    uint32_t prg_addr;

    if (addr >= 0x8000) {
        /* 32KB PRG-ROM mapping */
        prg_addr = addr & 0x7FFF;
        if (prg_addr < m->cart->prg_rom_size) {
            return m->cart->prg_rom[prg_addr];
        }
        /* Mirror for 16KB ROMs */
        return m->cart->prg_rom[prg_addr % m->cart->prg_rom_size];
    }
    return 0;
}

static void mapper_0_cpu_write(void* ctx, uint16_t addr, uint8_t val) {
    /* Mapper 0 has no writeable registers */
    (void)ctx;
    (void)addr;
    (void)val;
}

static uint8_t mapper_0_ppu_read(void* ctx, uint16_t addr) {
    mapper_0_ctx_t* m = (mapper_0_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->chr_rom) {
        return m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)];
    }
    return 0;
}

static void mapper_0_ppu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_0_ctx_t* m = (mapper_0_ctx_t*)ctx;
    if (addr < 0x2000) {
        /* CHR-RAM */
        m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)] = val;
    }
}

int mapper_0_init(nes_cartridge_t* cart, nes_mapper_t* mapper) {
    static mapper_0_ctx_t ctx;
    ctx.cart = cart;

    mapper->number = 0;
    mapper->cpu_read = mapper_0_cpu_read;
    mapper->cpu_write = mapper_0_cpu_write;
    mapper->ppu_read = mapper_0_ppu_read;
    mapper->ppu_write = mapper_0_ppu_write;
    mapper->context = &ctx;
    mapper->reset = NULL;
    mapper->scanline = NULL;
    mapper->clock_irq = NULL;

    return 0;
}

/* Mapper 1 (MMC1) */

#define MMC1_PRG_MODE_0  0  /* 32KB switchable @ $8000 */
#define MMC1_PRG_MODE_1  1  /* Fix first @ $8000, switch last @ $C000 */
#define MMC1_PRG_MODE_2  2  /* Fix first @ $8000, switch last @ $C000 */
#define MMC1_PRG_MODE_3  3  /* Fix last @ $C000, switch first @ $8000 */

#define MMC1_CHR_MODE_0  0  /* 8KB switchable */
#define MMC1_CHR_MODE_1  1  /* Two 4KB switchable */

typedef struct {
    nes_cartridge_t* cart;
    uint8_t  shift_reg;          /* 5-bit shift register */
    uint8_t  shift_count;        /* Number of bits in shift register */
    uint8_t  control;            /* Control register */
    uint8_t  chr_bank_0;         /* CHR bank 0 */
    uint8_t  chr_bank_1;         /* CHR bank 1 */
    uint8_t  prg_bank;           /* PRG bank */
    uint8_t  prg_mode;           /* PRG mode */
    uint8_t  chr_mode;           /* CHR mode */
    uint8_t  mirroring;          /* Mirroring mode */
    uint8_t  prg_ram_disabled;   /* PRG-RAM disable */
} mapper_1_ctx_t;

static inline uint8_t mmc1_get_bank(nes_cartridge_t* cart, uint8_t bank) {
    int num_banks = cart->info.prg_rom_banks;
    if (num_banks <= 1) return 0;
    return bank & (num_banks - 1);
}

static uint8_t mapper_1_cpu_read(void* ctx, uint16_t addr) {
    mapper_1_ctx_t* m = (mapper_1_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr >= 0x8000) {
        uint8_t bank;
        uint32_t base;

        switch (m->prg_mode) {
            case MMC1_PRG_MODE_0:
            case MMC1_PRG_MODE_1:
                /* 32KB mode: address lines A14 select bank */
                bank = mmc1_get_bank(cart, (m->prg_bank & 0x0E) | ((addr >> 14) & 1));
                base = (bank * NES_PRG_ROM_SIZE) + (addr & 0x7FFF);
                if (base < cart->prg_rom_size) {
                    return cart->prg_rom[base];
                }
                return cart->prg_rom[base % cart->prg_rom_size];

            case MMC1_PRG_MODE_2:
                /* Fix first bank, switch last */
                if (addr < 0xC000) {
                    base = 0;  /* First bank */
                } else {
                    bank = m->prg_bank;
                    base = (bank * NES_PRG_ROM_SIZE) + (addr & 0x3FFF);
                }
                if (base < cart->prg_rom_size) {
                    return cart->prg_rom[base];
                }
                return cart->prg_rom[base % cart->prg_rom_size];

            case MMC1_PRG_MODE_3:
                /* Switch first bank, fix last */
                if (addr < 0xC000) {
                    bank = m->prg_bank;
                    base = (bank * NES_PRG_ROM_SIZE) + (addr & 0x3FFF);
                } else {
                    base = (cart->info.prg_rom_banks - 1) * NES_PRG_ROM_SIZE;  /* Last bank */
                    base += (addr & 0x3FFF);
                }
                if (base < cart->prg_rom_size) {
                    return cart->prg_rom[base];
                }
                return cart->prg_rom[base % cart->prg_rom_size];
        }
    }

    /* PRG-RAM @ $6000-$7FFF */
    if (addr >= 0x6000 && addr < 0x8000 && cart->prg_ram) {
        return cart->prg_ram[addr & 0x1FFF];
    }

    return 0;
}

static void mapper_1_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_1_ctx_t* m = (mapper_1_ctx_t*)ctx;

    /* Determine register based on address */
    int reg;
    if (addr < 0xA000) reg = 0;      /* Control */
    else if (addr < 0xC000) reg = 1; /* CHR bank 0 */
    else if (addr < 0xE000) reg = 2; /* CHR bank 1 */
    else reg = 3;                    /* PRG bank */

    /* MMC1 reset detection */
    if (val & 0x80) {
        m->shift_reg = 0x10;
        m->shift_count = 0;
        m->prg_mode = 3;
        return;
    }

    /* Shift in bit */
    m->shift_reg = (m->shift_reg >> 1) | ((val & 1) << 4);
    m->shift_count++;

    if (m->shift_count == 5) {
        uint8_t data = m->shift_reg;

        switch (reg) {
            case 0: /* Control */
                m->control = data;
                m->mirroring = data & 3;
                m->prg_mode = (data >> 2) & 3;
                m->chr_mode = (data >> 4) & 1;

                /* Apply mirror mode to PPU */
                extern void nes_ppu_set_mirror_mode(uint8_t);
                nes_ppu_set_mirror_mode(m->mirroring);
                break;

            case 1: /* CHR bank 0 */
                m->chr_bank_0 = data;
                break;

            case 2: /* CHR bank 1 */
                m->chr_bank_1 = data;
                break;

            case 3: /* PRG bank */
                m->prg_bank = data & 0x0F;
                m->prg_ram_disabled = (data & 0x10) != 0;
                break;
        }

        m->shift_reg = 0x10;
        m->shift_count = 0;
    }
}

static uint8_t mapper_1_ppu_read(void* ctx, uint16_t addr) {
    mapper_1_ctx_t* m = (mapper_1_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr < 0x2000 && cart->chr_rom) {
        if (m->chr_mode == MMC1_CHR_MODE_0) {
            /* 8KB mode */
            uint8_t bank = m->chr_bank_0 & 0x1E;
            uint32_t base = (bank * NES_CHR_ROM_SIZE) + addr;
            if (base < cart->chr_rom_size) {
                return cart->chr_rom[base];
            }
            return cart->chr_rom[base % cart->chr_rom_size];
        } else {
            /* 4KB mode */
            uint8_t bank;
            if (addr < 0x1000) {
                bank = m->chr_bank_0;
            } else {
                bank = m->chr_bank_1;
            }
            uint32_t base = (bank * NES_CHR_ROM_SIZE) + (addr & 0x0FFF);
            if (base < cart->chr_rom_size) {
                return cart->chr_rom[base];
            }
            return cart->chr_rom[base % cart->chr_rom_size];
        }
    }
    return 0;
}

static void mapper_1_ppu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_1_ctx_t* m = (mapper_1_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr < 0x2000 && cart->info.has_chrram) {
        if (m->chr_mode == MMC1_CHR_MODE_0) {
            uint8_t bank = (m->chr_bank_0 >> 1) & 0x1F;
            uint32_t base = (bank * NES_CHR_ROM_SIZE) + addr;
            cart->chr_rom[base % cart->chr_rom_size] = val;
        } else {
            uint8_t bank = (addr < 0x1000) ? m->chr_bank_0 : m->chr_bank_1;
            uint32_t base = (bank * NES_CHR_ROM_SIZE) + (addr & 0x0FFF);
            cart->chr_rom[base % cart->chr_rom_size] = val;
        }
    }
}

int mapper_1_init(nes_cartridge_t* cart, nes_mapper_t* mapper, nes_ppu_t* ppu) {
    static mapper_1_ctx_t ctx;
    g_ppu = ppu;
    ctx.cart = cart;
    ctx.shift_reg = 0x10;
    ctx.shift_count = 0;
    ctx.control = 0x0C;
    ctx.chr_bank_0 = 0;
    ctx.chr_bank_1 = 0;
    ctx.prg_bank = 0;
    ctx.prg_mode = 3;
    ctx.chr_mode = 0;
    ctx.mirroring = 2;
    ctx.prg_ram_disabled = 0;

    mapper->number = 1;
    mapper->cpu_read = mapper_1_cpu_read;
    mapper->cpu_write = mapper_1_write;
    mapper->ppu_read = mapper_1_ppu_read;
    mapper->ppu_write = mapper_1_ppu_write;
    mapper->context = &ctx;
    mapper->reset = NULL;
    mapper->scanline = NULL;
    mapper->clock_irq = NULL;

    return 0;
}

/* Mapper 2 (UxROM) */

typedef struct {
    nes_cartridge_t* cart;
    uint8_t  bank_select;
} mapper_2_ctx_t;

static uint8_t mapper_2_cpu_read(void* ctx, uint16_t addr) {
    mapper_2_ctx_t* m = (mapper_2_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr >= 0x8000) {
        uint32_t base;

        if (addr < 0xC000) {
            /* Switchable 16KB @ $8000 */
            uint8_t bank = m->bank_select & (cart->info.prg_rom_banks - 1);
            base = (bank * NES_PRG_ROM_SIZE) + (addr & 0x3FFF);
        } else {
            /* Fixed 16KB @ $C000 (last bank) */
            base = (cart->info.prg_rom_banks - 1) * NES_PRG_ROM_SIZE;
            base += (addr & 0x3FFF);
        }

        if (base < cart->prg_rom_size) {
            return cart->prg_rom[base];
        }
        return cart->prg_rom[base % cart->prg_rom_size];
    }

    /* PRG-RAM @ $6000-$7FFF */
    if (addr >= 0x6000 && addr < 0x8000 && cart->prg_ram) {
        return cart->prg_ram[addr & 0x1FFF];
    }

    return 0;
}

static void mapper_2_cpu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_2_ctx_t* m = (mapper_2_ctx_t*)ctx;

    if (addr >= 0x8000) {
        m->bank_select = val;
    } else if (addr >= 0x6000 && addr < 0x8000 && m->cart->prg_ram) {
        m->cart->prg_ram[addr & 0x1FFF] = val;
    }
}

static uint8_t mapper_2_ppu_read(void* ctx, uint16_t addr) {
    mapper_2_ctx_t* m = (mapper_2_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->chr_rom) {
        return m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)];
    }
    return 0;
}

static void mapper_2_ppu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_2_ctx_t* m = (mapper_2_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->info.has_chrram) {
        m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)] = val;
    }
}

int mapper_2_init(nes_cartridge_t* cart, nes_mapper_t* mapper) {
    static mapper_2_ctx_t ctx;
    ctx.cart = cart;
    ctx.bank_select = 0;

    mapper->number = 2;
    mapper->cpu_read = mapper_2_cpu_read;
    mapper->cpu_write = mapper_2_cpu_write;
    mapper->ppu_read = mapper_2_ppu_read;
    mapper->ppu_write = mapper_2_ppu_write;
    mapper->context = &ctx;
    mapper->reset = NULL;
    mapper->scanline = NULL;
    mapper->clock_irq = NULL;

    return 0;
}

/* Mapper 3 (CNROM) */

typedef struct {
    nes_cartridge_t* cart;
    uint8_t  chr_bank;
} mapper_3_ctx_t;

static uint8_t mapper_3_cpu_read(void* ctx, uint16_t addr) {
    mapper_3_ctx_t* m = (mapper_3_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr >= 0x8000) {
        uint32_t base = addr & 0x7FFF;
        if (base < cart->prg_rom_size) {
            return cart->prg_rom[base];
        }
        return cart->prg_rom[base % cart->prg_rom_size];
    }

    if (addr >= 0x6000 && addr < 0x8000 && cart->prg_ram) {
        return cart->prg_ram[addr & 0x1FFF];
    }

    return 0;
}

static void mapper_3_cpu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_3_ctx_t* m = (mapper_3_ctx_t*)ctx;

    if (addr >= 0x8000) {
        m->chr_bank = val & 0x03;
    } else if (addr >= 0x6000 && addr < 0x8000 && m->cart->prg_ram) {
        m->cart->prg_ram[addr & 0x1FFF] = val;
    }
}

static uint8_t mapper_3_ppu_read(void* ctx, uint16_t addr) {
    mapper_3_ctx_t* m = (mapper_3_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr < 0x2000 && cart->chr_rom) {
        uint8_t bank = m->chr_bank & (cart->info.chr_rom_banks - 1);
        uint32_t base = (bank * NES_CHR_ROM_SIZE) + addr;
        if (base < cart->chr_rom_size) {
            return cart->chr_rom[base];
        }
        return cart->chr_rom[base % cart->chr_rom_size];
    }
    return 0;
}

static void mapper_3_ppu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_3_ctx_t* m = (mapper_3_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->info.has_chrram) {
        uint8_t bank = (m->chr_bank >> 3) & 0x03;
        uint32_t base = (bank * NES_CHR_ROM_SIZE) + addr;
        m->cart->chr_rom[base % m->cart->chr_rom_size] = val;
    }
}

int mapper_3_init(nes_cartridge_t* cart, nes_mapper_t* mapper) {
    static mapper_3_ctx_t ctx;
    ctx.cart = cart;
    ctx.chr_bank = 0;

    mapper->number = 3;
    mapper->cpu_read = mapper_3_cpu_read;
    mapper->cpu_write = mapper_3_cpu_write;
    mapper->ppu_read = mapper_3_ppu_read;
    mapper->ppu_write = mapper_3_ppu_write;
    mapper->context = &ctx;
    mapper->reset = NULL;
    mapper->scanline = NULL;
    mapper->clock_irq = NULL;

    return 0;
}

/* Mapper 4 (MMC3) - Simplified */

typedef struct {
    nes_cartridge_t* cart;
    uint8_t  registers[8];
    uint8_t  bank_select;
    uint8_t  irq_counter;
    uint8_t  irq_latch;
    uint8_t  irq_enabled;
    uint8_t  irq_reload;
    uint8_t  prg_mode;
    uint8_t  chr_mode;
} mapper_4_ctx_t;

static uint8_t mapper_4_cpu_read(void* ctx, uint16_t addr) {
    mapper_4_ctx_t* m = (mapper_4_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    /* IRQ acknowledge at $E000-$FFFF */
    if (addr == 0xE000) {
        m->irq_enabled = 0;
    } else if (addr == 0xE001) {
        m->irq_enabled = 1;
    }

    if (addr >= 0x8000) {
        uint32_t base;
        uint8_t bank;

        /* Determine bank and mode */
        if (m->prg_mode == 0) {
            if (addr < 0xA000) {
                bank = m->registers[6];  /* $8000-$9FFF */
            } else if (addr < 0xC000) {
                bank = m->registers[7];  /* $A000-$BFFF */
            } else if (addr < 0xE000) {
                bank = (cart->info.prg_rom_banks - 2);  /* Fixed $C000-$DFFF */
            } else {
                bank = (cart->info.prg_rom_banks - 1);  /* Fixed $E000-$FFFF */
            }
        } else {
            if (addr < 0xA000) {
                bank = (cart->info.prg_rom_banks - 2);  /* Fixed $8000-$9FFF */
            } else if (addr < 0xC000) {
                bank = m->registers[7];  /* $A000-$BFFF */
            } else if (addr < 0xE000) {
                bank = m->registers[6];  /* $C000-$DFFF */
            } else {
                bank = (cart->info.prg_rom_banks - 1);  /* Fixed $E000-$FFFF */
            }
        }

        base = (bank * NES_PRG_ROM_SIZE) * 8 + (addr & 0x1FFF);
        if (base < cart->prg_rom_size) {
            return cart->prg_rom[base];
        }
        return cart->prg_rom[base % cart->prg_rom_size];
    }

    if (addr >= 0x6000 && addr < 0x8000 && cart->prg_ram) {
        return cart->prg_ram[addr & 0x1FFF];
    }

    return 0;
}

static void mapper_4_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_4_ctx_t* m = (mapper_4_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr >= 0x8000 && addr < 0xA001) {
        if (addr & 1) {
            m->bank_select = val;
            m->prg_mode = (val >> 6) & 1;
            m->chr_mode = (val >> 7) & 1;
        } else {
            /* Mirroring control */
            if (val & 1) {
                /* Horizontal */
            } else {
                /* Vertical */
            }
        }
    }
    else if (addr >= 0xA000 && addr < 0xC001) {
        int reg = m->bank_select & 7;
        m->registers[reg] = val;
    }
    else if (addr >= 0xC000 && addr < 0xE001) {
        if (addr & 1) {
            m->irq_reload = 1;
        } else {
            m->irq_latch = val;
        }
    }
    else if (addr >= 0xE000 && addr < 0x10001) {
        if (addr & 1) {
            m->irq_enabled = 1;
        } else {
            m->irq_enabled = 0;
        }
    }
    else if (addr >= 0x6000 && addr < 0x8000 && cart->prg_ram) {
        cart->prg_ram[addr & 0x1FFF] = val;
    }
}

static uint8_t mapper_4_ppu_read(void* ctx, uint16_t addr) {
    mapper_4_ctx_t* m = (mapper_4_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr < 0x2000 && cart->chr_rom) {
        uint8_t bank;
        if (m->chr_mode == 0) {
            /* 2KB mode */
            if (addr < 0x0800) bank = m->registers[0] & 0xFE;
            else if (addr < 0x1000) bank = m->registers[1] & 0xFE;
            else if (addr < 0x1400) bank = m->registers[2] & 0xFE;
            else if (addr < 0x1800) bank = m->registers[3] & 0xFE;
            else if (addr < 0x1C00) bank = m->registers[4] & 0xFE;
            else bank = m->registers[5] & 0xFE;
        } else {
            /* 1KB mode */
            if (addr < 0x0400) bank = m->registers[0];
            else if (addr < 0x0800) bank = m->registers[1];
            else if (addr < 0x0C00) bank = m->registers[2];
            else if (addr < 0x1000) bank = m->registers[3];
            else if (addr < 0x1400) bank = m->registers[4];
            else bank = m->registers[5];
        }

        uint32_t base = (bank * NES_CHR_ROM_SIZE) + addr;
        if (base < cart->chr_rom_size) {
            return cart->chr_rom[base];
        }
        return cart->chr_rom[base % cart->chr_rom_size];
    }
    return 0;
}

static void mapper_4_ppu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_4_ctx_t* m = (mapper_4_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->info.has_chrram) {
        m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)] = val;
    }
}

int mapper_4_init(nes_cartridge_t* cart, nes_mapper_t* mapper, nes_ppu_t* ppu) {
    static mapper_4_ctx_t ctx;
    g_ppu = ppu;
    ctx.cart = cart;
    memset(ctx.registers, 0, sizeof(ctx.registers));
    ctx.bank_select = 0;
    ctx.irq_counter = 0;
    ctx.irq_latch = 0;
    ctx.irq_enabled = 0;
    ctx.irq_reload = 0;
    ctx.prg_mode = 0;
    ctx.chr_mode = 0;

    mapper->number = 4;
    mapper->cpu_read = mapper_4_cpu_read;
    mapper->cpu_write = mapper_4_write;
    mapper->ppu_read = mapper_4_ppu_read;
    mapper->ppu_write = mapper_4_ppu_write;
    mapper->context = &ctx;
    mapper->reset = NULL;
    mapper->scanline = NULL;
    mapper->clock_irq = NULL;

    return 0;
}

/* Mapper 7 (AxROM) */

typedef struct {
    nes_cartridge_t* cart;
    uint8_t  prg_bank;
} mapper_7_ctx_t;

static uint8_t mapper_7_cpu_read(void* ctx, uint16_t addr) {
    mapper_7_ctx_t* m = (mapper_7_ctx_t*)ctx;
    nes_cartridge_t* cart = m->cart;

    if (addr >= 0x8000) {
        uint8_t bank = m->prg_bank & (cart->info.prg_rom_banks - 1);
        uint32_t base = (bank * NES_PRG_ROM_SIZE) + (addr & 0x7FFF);
        if (base < cart->prg_rom_size) {
            return cart->prg_rom[base];
        }
        return cart->prg_rom[base % cart->prg_rom_size];
    }

    return 0;
}

static void mapper_7_cpu_write(void* ctx, uint16_t addr, uint8_t val) {
    if (addr >= 0x8000) {
        mapper_7_ctx_t* m = (mapper_7_ctx_t*)ctx;
        m->prg_bank = val & 0x07;

        /* Single screen mirroring */
        extern void nes_ppu_set_mirror_mode(uint8_t);
        nes_ppu_set_mirror_mode(val & 0x10 ? 3 : 2);  /* Upper/lower screen */
    }
}

static uint8_t mapper_7_ppu_read(void* ctx, uint16_t addr) {
    mapper_7_ctx_t* m = (mapper_7_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->chr_rom) {
        return m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)];
    }
    return 0;
}

static void mapper_7_ppu_write(void* ctx, uint16_t addr, uint8_t val) {
    mapper_7_ctx_t* m = (mapper_7_ctx_t*)ctx;
    if (addr < 0x2000 && m->cart->info.has_chrram) {
        m->cart->chr_rom[addr & (m->cart->chr_rom_size - 1)] = val;
    }
}

int mapper_7_init(nes_cartridge_t* cart, nes_mapper_t* mapper) {
    static mapper_7_ctx_t ctx;
    ctx.cart = cart;
    ctx.prg_bank = 0;

    mapper->number = 7;
    mapper->cpu_read = mapper_7_cpu_read;
    mapper->cpu_write = mapper_7_cpu_write;
    mapper->ppu_read = mapper_7_ppu_read;
    mapper->ppu_write = mapper_7_ppu_write;
    mapper->context = &ctx;
    mapper->reset = NULL;
    mapper->scanline = NULL;
    mapper->clock_irq = NULL;

    return 0;
}

/* Mapper creation factory */

int nes_mapper_create(nes_cartridge_t* cart, nes_mapper_t* mapper, nes_ppu_t* ppu) {
    printf("nes_mapper_create: cart=%p, mapper=%p, ppu=%p\n", (void*)cart, (void*)mapper, (void*)ppu);

    if (!cart || !mapper) {
        printf("ERROR: cart or mapper is NULL\n");
        return -1;
    }

    int mapper_num = cart->info.mapper;
    printf("Mapper number: %d\n", mapper_num);

    switch (mapper_num) {
        case 0:
            printf("Initializing mapper 0...\n");
            return mapper_0_init(cart, mapper);
        case 1:
            printf("Initializing mapper 1...\n");
            return mapper_1_init(cart, mapper, ppu);
        case 2:
            printf("Initializing mapper 2...\n");
            return mapper_2_init(cart, mapper);
        case 3:
            printf("Initializing mapper 3...\n");
            return mapper_3_init(cart, mapper);
        case 4:
            printf("Initializing mapper 4...\n");
            return mapper_4_init(cart, mapper, ppu);
        case 7:
            printf("Initializing mapper 7...\n");
            return mapper_7_init(cart, mapper);
        default:
            printf("Unsupported mapper: %d\n", mapper_num);
            return -1;
    }
}

void nes_mapper_destroy(nes_mapper_t* mapper) {
    mapper->cpu_read = NULL;
    mapper->cpu_write = NULL;
    mapper->ppu_read = NULL;
    mapper->ppu_write = NULL;
    mapper->context = NULL;
}

void nes_mapper_reset(nes_mapper_t* mapper) {
    if (mapper->reset) {
        mapper->reset(mapper->context);
    }
}
