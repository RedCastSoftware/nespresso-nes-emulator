/**
 * NESPRESSO - NES Emulator
 * PPU Module - Picture Processing Unit Implementation
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "ppu.h"
#include <string.h>
#include <stdio.h>

/* NES Standard Palette (64 colors, but NES only uses 0x00-0x3F) */
static const uint8_t g_nes_palette[64][3] = {
    {0x66,0x66,0x66}, {0x00,0x2A,0x88}, {0x14,0x12,0xA7}, {0x3B,0x00,0xA4},
    {0x5C,0x00,0x7E}, {0x6E,0x00,0x40}, {0x6C,0x06,0x00}, {0x56,0x1D,0x00},
    {0x33,0x35,0x00}, {0x0B,0x48,0x00}, {0x00,0x52,0x00}, {0x00,0x4F,0x08},
    {0x00,0x40,0x4D}, {0x00,0x00,0x00}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
    {0xAD,0xAD,0xAD}, {0x15,0x5F,0xD9}, {0x42,0x40,0xFF}, {0x75,0x27,0xFE},
    {0xA0,0x1A,0xCC}, {0xB7,0x1E,0x7B}, {0xB5,0x31,0x20}, {0x99,0x4E,0x00},
    {0x6B,0x6D,0x00}, {0x38,0x87,0x00}, {0x0C,0x93,0x00}, {0x00,0x8F,0x32},
    {0x00,0x7C,0x8D}, {0x00,0x00,0x00}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
    {0xFF,0xFE,0xFF}, {0x64,0xB0,0xFF}, {0x92,0x90,0xFF}, {0xC6,0x76,0xFF},
    {0xF3,0x6A,0xFF}, {0xFE,0x6E,0xCC}, {0xFE,0x81,0x70}, {0xEA,0x9E,0x22},
    {0xBC,0xBE,0x00}, {0x88,0xD8,0x00}, {0x5C,0xE4,0x30}, {0x45,0xE0,0x82},
    {0x48,0xCD,0xDE}, {0x4F,0x4F,0x4F}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
    {0xFF,0xFE,0xFF}, {0xC0,0xDF,0xFF}, {0xD3,0xD2,0xFF}, {0xE8,0xC8,0xFF},
    {0xFB,0xC2,0xFF}, {0xFE,0xC4,0xEA}, {0xFE,0xCC,0xC5}, {0xF7,0xD8,0xA5},
    {0xE4,0xE5,0x94}, {0xCF,0xEF,0x96}, {0xBD,0xF4,0xAB}, {0xB3,0xF3,0xCC},
    {0xB5,0xEB,0xF2}, {0xB8,0xB8,0xB8}, {0x00,0x00,0x00}, {0x00,0x00,0x00},
};

/* Static PPU bus - static storage that persists beyond function calls */
static ppu_bus_t g_ppu_bus_static = {NULL, NULL, NULL, NULL};
static ppu_bus_t* g_ppu_bus = NULL;

/* Frame buffer for rendering */
static uint8_t g_frame_buffer[PPU_WIDTH * PPU_HEIGHT];

/* Mirroring modes */
typedef enum {
    MIRROR_HORIZONTAL,    /* Horizontal: [0][1] [2][3] */
    MIRROR_VERTICAL,      /* Vertical:   [0][2] [1][3] */
    MIRROR_SINGLE_0,      /* Single screen 0 */
    MIRROR_SINGLE_1,      /* Single screen 1 */
    MIRROR_FOUR_SCREEN    /* Four screen */
} mirror_mode_t;

static mirror_mode_t g_mirror_mode = MIRROR_HORIZONTAL;

/* Internal helper functions */

static uint16_t read_name_table_addr(uint16_t addr) {
    uint16_t base = addr & 0x03FF;  /* Offset within nametable */
    switch (addr & 0x0C00) {
        case 0x0000:  /* NT 0 */
            break;
        case 0x0400:  /* NT 1 */
            switch (g_mirror_mode) {
                case MIRROR_HORIZONTAL: base |= 0x0400; break;    /* [0][1] */
                case MIRROR_VERTICAL:   base &= ~0x0400; break;   /* [0][0] */
                case MIRROR_SINGLE_0:   base &= ~0x0400; break;
                case MIRROR_SINGLE_1:   base |= 0x0400; break;
                case MIRROR_FOUR_SCREEN: base |= 0x0400; break;
            }
            break;
        case 0x0800:  /* NT 2 */
            switch (g_mirror_mode) {
                case MIRROR_HORIZONTAL: base &= ~0x0400; break;   /* Same as NT 1 */
                case MIRROR_VERTICAL:   base |= 0x0400; break;    /* [2][3] */
                case MIRROR_SINGLE_0:   base &= ~0x0400; break;
                case MIRROR_SINGLE_1:   base |= 0x0400; break;
                case MIRROR_FOUR_SCREEN: base |= 0x0800; break;
            }
            break;
        case 0x0C00:  /* NT 3 */
            switch (g_mirror_mode) {
                case MIRROR_HORIZONTAL: base |= 0x0400; break;    /* Same as NT 1 */
                case MIRROR_VERTICAL:   base |= 0x0400; break;    /* Same as NT 2 */
                case MIRROR_SINGLE_0:   base &= ~0x0400; break;
                case MIRROR_SINGLE_1:   base |= 0x0400; break;
                case MIRROR_FOUR_SCREEN: base |= 0x0C00; break;
            }
            break;
    }
    return 0x2000 | base;
}

static uint8_t read_palette(uint16_t addr) {
    addr = (addr & 0x1F);
    if ((addr & 0x13) == 0x10) {
        addr &= ~0x10;  /* Addresses $10, $14, $18, $1C are mirrors of $00, $04, $08, $0C */
    }
    return g_nes_palette[addr % 0x40][0];
}

static uint8_t ppu_read_vram(uint16_t addr) {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        /* Pattern tables - access via CHR bus */
        if (g_ppu_bus && g_ppu_bus->read_chr) {
            return g_ppu_bus->read_chr(g_ppu_bus->context, addr);
        }
        return 0;
    } else if (addr < 0x3F00) {
        /* Nametables and attribute tables */
        uint16_t mirrored = read_name_table_addr(addr);
        return g_frame_buffer[(mirrored - 0x2000) & 0xFFF];
    } else {
        /* Palettes */
        return read_palette(addr);
    }
}

static void ppu_write_vram(nes_ppu_t* ppu, uint16_t addr, uint8_t val) {
    addr &= 0x3FFF;

    if (addr < 0x2000) {
        /* Pattern tables - usually CHR-ROM (read-only) */
        if (g_ppu_bus && g_ppu_bus->write_chr) {
            g_ppu_bus->write_chr(g_ppu_bus->context, addr, val);
        }
    } else if (addr < 0x3F00) {
        /* Nametables */
        uint16_t mirrored = read_name_table_addr(addr);
        ppu->vram[(mirrored - 0x2000) & 0xFFF] = val;
    } else {
        /* Palettes */
        addr = addr & 0x1F;
        if ((addr & 0x13) == 0x10) {
            addr &= ~0x10;
        }
        ppu->palette[addr] = val & 0x3F;
    }
}

/* Increment VRAM address */
static void increment_vram_addr(nes_ppu_t* ppu) {
    if (ppu->reg.ctrl & PPUCTRL_INC32) {
        ppu->reg.scroll.v += 32;
    } else {
        ppu->reg.scroll.v++;
    }
    ppu->reg.scroll.v &= 0x7FFF;
}

/* Scroll register operations */
static void write_ppu_scroll_x(nes_ppu_t* ppu, uint8_t val) {
    ppu->reg.scroll.t = (ppu->reg.scroll.t & ~0x001F) | ((val >> 3) & 0x001F);
    ppu->reg.scroll.x = val & 0x07;
    ppu->reg.scroll.w = 1;
}

static void write_ppu_scroll_y(nes_ppu_t* ppu, uint8_t val) {
    ppu->reg.scroll.t = (ppu->reg.scroll.t & 0x0FBE0) | ((val & 0xF8) << 2);
    ppu->reg.scroll.t = (ppu->reg.scroll.t & 0x7C1F) | ((val & 0x07) << 12);
    ppu->reg.scroll.w = 0;
}

static void write_ppu_addr(nes_ppu_t* ppu, uint8_t val) {
    if (!ppu->reg.scroll.w) {
        /* First byte - high byte */
        ppu->reg.scroll.t = (ppu->reg.scroll.t & 0x80FF) | ((uint16_t)val & 0x3F) << 8;
        ppu->reg.scroll.v = (ppu->reg.scroll.v & 0x80FF) | ((uint16_t)val & 0x3F) << 8;
        ppu->reg.scroll.w = 1;
    } else {
        /* Second byte - low byte */
        ppu->reg.scroll.t = (ppu->reg.scroll.t & 0x7F00) | val;
        ppu->reg.scroll.v = (ppu->reg.scroll.v & 0x7F00) | val;
        ppu->reg.scroll.w = 0;
    }
}

/* Copy t to v, used at render start */
static void copy_t_to_v(nes_ppu_t* ppu) {
    ppu->reg.scroll.v &= 0x841F;  /* Keep only preserved bits */
    ppu->reg.scroll.v |= (ppu->reg.scroll.t & 0x7BE0);
}

/* Increment horizontal scroll position */
static void increment_x(nes_ppu_t* ppu) {
    if ((ppu->reg.scroll.v & 0x001F) == 31) {
        ppu->reg.scroll.v &= ~0x001F;
        ppu->reg.scroll.v ^= 0x0400;
    } else {
        ppu->reg.scroll.v++;
    }
}

/* Increment vertical scroll position */
static void increment_y(nes_ppu_t* ppu) {
    if ((ppu->reg.scroll.v & 0x7000) != 0x7000) {
        ppu->reg.scroll.v += 0x1000;
    } else {
        ppu->reg.scroll.v &= ~0x7000;
        uint8_t y = (ppu->reg.scroll.v & 0x03E0) >> 5;
        if (y == 29) {
            y = 0;
            ppu->reg.scroll.v ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y++;
        }
        ppu->reg.scroll.v = (ppu->reg.scroll.v & ~0x03E0) | (y << 5);
    }
}

/* Load background shifters with next tile data */
static void load_background_shifters(nes_ppu_t* ppu) {
    ppu->background_shift_lo = (ppu->background_shift_lo & 0xFF00) | ppu->background_fetch_tile;
    ppu->background_shift_hi = (ppu->background_shift_hi & 0xFF00) | (ppu->background_fetch_tile >> 8);

    ppu->attribute_shift_lo = (ppu->attribute_shift_lo & 0xFF00) | ppu->attribute_latch_lo;
    ppu->attribute_shift_hi = (ppu->attribute_shift_hi & 0xFF00) | ppu->attribute_latch_hi;
}

/* Fetch background tile data */
static void fetch_tile_data(nes_ppu_t* ppu, uint16_t *addr_lo, uint16_t *addr_hi) {
    uint16_t nametable_addr = 0x2000 | (ppu->reg.scroll.v & 0x0FFF);
    uint8_t tile_index = ppu_read_vram(nametable_addr);

    uint16_t attr_addr = 0x23C0 | (ppu->reg.scroll.v & 0x0C00) |
                        ((ppu->reg.scroll.v >> 4) & 0x38) |
                        ((ppu->reg.scroll.v >> 2) & 0x07);
    uint8_t attr = ppu_read_vram(attr_addr);

    if (ppu->reg.scroll.v & 0x40) attr >>= 2;
    if (ppu->reg.scroll.v & 0x02) attr >>= 2;
    attr &= 0x03;

    ppu->background_fetch_attr = ((attr & 0x01) ? 0xFF : 0x00);
    ppu->attribute_latch_hi = ((attr & 0x02) ? 0xFF : 0x00);
    ppu->attribute_latch_lo = ~ppu->attribute_latch_hi;

    uint16_t pattern_table = (ppu->reg.ctrl & PPUCTRL_BG_ADDR) ? 0x1000 : 0x0000;
    *addr_lo = pattern_table + ((uint16_t)tile_index << 4) + ((ppu->reg.scroll.v >> 12) & 0x07);
    *addr_hi = *addr_lo + 8;
}

/* Evaluate sprites for current scanline */
static void evaluate_sprites(nes_ppu_t* ppu) {
    int sprite_height = (ppu->reg.ctrl & PPUCTRL_SP_SIZE) ? 16 : 8;
    ppu->sprite_count = 0;

    for (int i = 0; i < PPU_SPRITE_COUNT && ppu->sprite_count < 8; i++) {
        ppu_sprite_t sprite;
        sprite.y    = ppu->oam[i * 4 + 0];
        sprite.tile  = ppu->oam[i * 4 + 1];
        sprite.attr  = ppu->oam[i * 4 + 2];
        sprite.x     = ppu->oam[i * 4 + 3];

        int line = (int)ppu->scanline - (int)sprite.y;
        if (line >= 0 && line < sprite_height) {
            ppu->sprites[ppu->sprite_count++] = sprite;
        }
    }
}

/* Check for sprite zero hit */
static int check_sprite_zero_hit(nes_ppu_t* ppu, int x) {
    if (ppu->sprite_count == 0) return 0;
    ppu_sprite_t* sprite = &ppu->sprites[0];

    int sprite_height = (ppu->reg.ctrl & PPUCTRL_SP_SIZE) ? 16 : 8;
    int line = (int)ppu->scanline - (int)sprite->y;

    if (line >= 0 && line < sprite_height && x >= sprite->x && x < sprite->x + 8) {
        if (!(ppu->reg.mask & PPUMASK_SHOW_SPR8) && x < 8) return 0;
        if (!(ppu->reg.mask & PPUMASK_SHOW_BGR8) && x < 8) return 0;
        return 1;
    }
    return 0;
}

/* Get sprite pixel */
static uint8_t get_sprite_pixel(nes_ppu_t* ppu, int x) {
    for (int i = 0; i < ppu->sprite_count; i++) {
        ppu_sprite_t* sprite = &ppu->sprites[i];
        int sprite_height = (ppu->reg.ctrl & PPUCTRL_SP_SIZE) ? 16 : 8;

        if (x >= sprite->x && x < sprite->x + 8) {
            int line = (int)ppu->scanline - (int)sprite->y;
            if (line >= 0 && line < sprite_height) {
                if (sprite->attr & SP_ATTR_FLIP_V) {
                    line = sprite_height - 1 - line;
                }

                uint16_t pattern_table;
                uint8_t tile = sprite->tile;
                if (sprite_height == 16) {
                    if (line >= 8) {
                        line -= 8;
                        tile |= 0x01;
                    } else {
                        tile &= ~0x01;
                    }
                    pattern_table = (tile & 0x01) ? 0x1000 : 0x0000;
                    tile &= 0xFE;
                } else {
                    pattern_table = (ppu->reg.ctrl & PPUCTRL_SP_ADDR) ? 0x1000 : 0x0000;
                }

                int column = x - sprite->x;
                if (sprite->attr & SP_ATTR_FLIP_H) {
                    column = 7 - column;
                }

                uint16_t addr = pattern_table + ((uint16_t)tile << 4) + line;
                uint8_t lo = ppu_read_vram(addr);
                uint8_t hi = ppu_read_vram(addr + 8);

                int shift = 7 - column;
                uint8_t pixel = ((lo >> shift) & 0x01) | (((hi >> shift) & 0x01) << 1);

                if (i == 0 && pixel != 0 && check_sprite_zero_hit(ppu, x)) {
                    if ((ppu->reg.status & PPUSTATUS_VBLANK) == 0) {
                        ppu->reg.status |= PPUSTATUS_SP0_HIT;
                    }
                }

                return pixel | ((sprite->attr & SP_ATTR_PAL_MASK) << 2);
            }
        }
    }
    return 0;
}

/* Render a single scanline */
static void render_scanline(nes_ppu_t* ppu) {
    if (ppu->scanline < PPU_VISIBLE_SCANLINES) {
        uint8_t* line_buf = g_frame_buffer + ppu->scanline * PPU_WIDTH;

        for (int x = 0; x < PPU_WIDTH; x++) {
            uint8_t bg_pixel = 0;
            uint8_t bg_palette = 0;

            if (ppu->reg.mask & PPUMASK_SHOW_BGR) {
                if (!(ppu->reg.mask & PPUMASK_SHOW_BGR8) && x < 8) {
                    bg_pixel = 0;
                } else {
                    uint8_t shift = 15 - ppu->reg.scroll.x - (x % 8);
                    if (shift >= 0) {
                        bg_pixel = ((ppu->background_shift_lo >> shift) & 0x01) |
                                   (((ppu->background_shift_hi >> shift) & 0x01) << 1);
                    }
                    if (bg_pixel) {
                        bg_palette = ((ppu->attribute_shift_lo >> (15 - ppu->reg.scroll.x - (x % 8))) & 0x01) |
                                    (((ppu->attribute_shift_hi >> (15 - ppu->reg.scroll.x - (x % 8))) & 0x01) << 1);
                    }
                }
            }

            uint8_t sprite_pixel = 0;
            if (ppu->reg.mask & PPUMASK_SHOW_SPR &&
                (!(ppu->reg.mask & PPUMASK_SHOW_SPR8) || x >= 8)) {
                sprite_pixel = get_sprite_pixel(ppu, x);
            }

            uint8_t pixel = 0;
            if (bg_pixel == 0 && sprite_pixel == 0) {
                pixel = 0;
            } else if (bg_pixel == 0 && sprite_pixel != 0) {
                pixel = sprite_pixel | 0x10;
            } else if (bg_pixel != 0 && sprite_pixel == 0) {
                pixel = bg_palette | 0x20;
            } else {
                /* Both non-zero - check priority */
                if (sprite_pixel & SP_ATTR_PRIORITY) {
                    pixel = bg_palette | 0x20;  /* Sprite behind background */
                } else {
                    pixel = sprite_pixel | 0x10;
                }
            }

            line_buf[x] = pixel;
        }
    }
}

/* Public API Implementation */

void nes_ppu_init(nes_ppu_t* ppu, ppu_bus_t* bus) {
    memset(ppu, 0, sizeof(nes_ppu_t));
    (void)bus;  /* unused - bus is set later via nes_ppu_set_bus */
    g_ppu_bus = &g_ppu_bus_static;

    /* Initialize palette with default values */
    for (int i = 0; i < 32; i++) {
        ppu->palette[i] = i % 64;
    }

    ppu->reg.status = PPUSTATUS_VBLANK;
    ppu->scanline = 261;
    ppu->cycle = 0;
}

void nes_ppu_reset(nes_ppu_t* ppu) {
    ppu->reg.ctrl = 0;
    ppu->reg.mask = 0;
    ppu->reg.oam_addr = 0;
    ppu->reg.scroll.w = 0;
    ppu->reg.data_buffer = 0;
    ppu->reg.status = PPUSTATUS_VBLANK;
    ppu->scanline = 261;  /* Start at pre-render scanline */
    ppu->cycle = 0;
    ppu->frame = 0;
    ppu->odd_frame = 0;
    ppu->sprite_count = 0;
}

int nes_ppu_step(nes_ppu_t* ppu) {
    int frame_complete = 0;

    /* Visible scanline rendering (0-239) */
    if (ppu->scanline < 240) {
        /* Evaluate sprites for this scanline */
        if (ppu->cycle == 1) {
            evaluate_sprites(ppu);
        }

        /* Fetch tile data */
        if ((ppu->cycle >= 1 && ppu->cycle <= 256) || (ppu->cycle >= 321 && ppu->cycle <= 336)) {
            int cycle_mod = ppu->cycle % 8;

            if (cycle_mod == 1) {
                load_background_shifters(ppu);
            } else if (cycle_mod == 0) {
                increment_x(ppu);
            }

            if (cycle_mod == 0) {
                /* Fetch tile data */
                uint16_t addr_lo, addr_hi;
                fetch_tile_data(ppu, &addr_lo, &addr_hi);
                ppu->background_fetch_tile = ppu_read_vram(addr_lo) |
                                            (ppu_read_vram(addr_hi) << 8);
            }
        }

        /* Copy scroll at cycle 304 (pre-render) */
        if (ppu->scanline == 261 && ppu->cycle == 304) {
            copy_t_to_v(ppu);
        }

        /* Render background pixels during visible scanlines */
        if (ppu->cycle >= 1 && ppu->cycle <= 256 && (ppu->reg.mask & (PPUMASK_SHOW_BGR | PPUMASK_SHOW_SPR))) {
            /* Background rendering */
            if (ppu->reg.mask & PPUMASK_SHOW_BGR) {
                uint8_t* line_buf = g_frame_buffer + ppu->scanline * PPU_WIDTH;
                int x = ppu->cycle - 1;

                uint8_t pixel = 0;
                if (!(ppu->reg.mask & PPUMASK_SHOW_BGR8) && x < 8) {
                    pixel = 0;
                } else {
                    uint8_t shift = 7 - ((x + ppu->reg.scroll.x) % 8) /*ppu->reg.scroll.x*/;
                    if (shift <= 15) {
                        uint8_t bg_lo = (ppu->background_shift_lo >> shift) & 0x01;
                        uint8_t bg_hi = (ppu->background_shift_hi >> shift) & 0x01;
                        pixel = bg_lo | (bg_hi << 1);

                        if (pixel && (x & 3) == 0) {
                            /* Get attribute at tile boundaries */
                            uint16_t attr_addr = 0x23C0 | (ppu->reg.scroll.v & 0x0C00) |
                                               ((ppu->reg.scroll.v >> 4) & 0x38) |
                                               ((ppu->reg.scroll.v >> 2) & 0x07);
                            uint8_t attr = ppu_read_vram(attr_addr);

                            if ((x / 2) & 1) attr >>= 2;
                            if ((ppu->scanline / 2) & 1) attr >>= 2;

                            pixel |= (attr & 0x03) << 2;
                        }
                    }
                }

                /* Render sprites */
                uint8_t sprite_pixel = 0;
                if (ppu->reg.mask & PPUMASK_SHOW_SPR &&
                    (!(ppu->reg.mask & PPUMASK_SHOW_SPR8) || x >= 8)) {
                    for (int i = 0; i < ppu->sprite_count; i++) {
                        ppu_sprite_t* sprite = &ppu->sprites[i];
                        int sprite_height = (ppu->reg.ctrl & PPUCTRL_SP_SIZE) ? 16 : 8;
                        int line = (int)ppu->scanline - (int)sprite->y;

                        if (line >= 0 && line < sprite_height && x >= sprite->x && x < sprite->x + 8) {
                            uint8_t attr = sprite->attr;
                            if (line >= sprite_height) continue;

                            uint16_t pattern_table = (ppu->reg.ctrl & PPUCTRL_SP_ADDR) ? 0x1000 : 0x0000;
                            int column = x - sprite->x;
                            if (attr & SP_ATTR_FLIP_H) column = 7 - column;

                            uint16_t addr = pattern_table + ((uint16_t)sprite->tile << 4) + line;
                            uint8_t lo = ppu_read_vram(addr);
                            uint8_t hi = ppu_read_vram(addr + 8);
                            sprite_pixel = ((lo >> column) & 1) | (((hi >> column) & 1) << 1);

                            if (i == 0 && sprite_pixel && pixel) {
                                ppu->reg.status |= PPUSTATUS_SP0_HIT;
                            }

                            if (sprite_pixel) {
                                sprite_pixel |= (attr & SP_ATTR_PAL_MASK) << 2;
                                if (attr & SP_ATTR_PRIORITY) {
                                    pixel = 0;  /* Sprite behind */
                                } else {
                                    break;  /* Sprite in front */
                                }
                            }
                        }
                    }
                }

                if (sprite_pixel) pixel = sprite_pixel | 0x10;
                else if (pixel) pixel = pixel | 0x20;

                line_buf[x] = pixel;
            }
        }

        /* Shift background registers for each cycle */
        if (ppu->cycle >= 1 && ppu->cycle <= 256) {
            ppu->background_shift_lo <<= 1;
            ppu->background_shift_hi <<= 1;
            ppu->attribute_shift_lo <<= 1;
            ppu->attribute_shift_hi <<= 1;
        }

        /* Increment Y at end of visible scanline */
        if (ppu->cycle == 256) {
            increment_y(ppu);
        }

        /* Reset X at cycle 257 */
        if (ppu->cycle == 257) {
            ppu->reg.scroll.v = (ppu->reg.scroll.v & 0xFBE0) | (ppu->reg.scroll.t & 0x041F);
        }
    }
    /* Post-render scanline (240) */
    else if (ppu->scanline == 240) {
        if (ppu->cycle == 0) {
            /* Do nothing - idle */
        }
    }
    /* VBlank (241-260) */
    else if (ppu->scanline >= 241 && ppu->scanline <= 260) {
        if (ppu->cycle == 1 && ppu->scanline == 241) {
            ppu->reg.status |= PPUSTATUS_VBLANK;
            ppu->reg.status &= ~PPUSTATUS_SP0_HIT;
            ppu->reg.status &= ~PPUSTATUS_SP_OVF;

            if (ppu->reg.ctrl & PPUCTRL_NMI) {
                frame_complete = 1;
            }
        }
    }
    /* Pre-render scanline (261) */
    else if (ppu->scanline == 261) {
        if (ppu->cycle == 1) {
            ppu->reg.status &= ~PPUSTATUS_VBLANK;
            ppu->reg.status &= ~PPUSTATUS_SP0_HIT;
            ppu->reg.status &= ~PPUSTATUS_SP_OVF;
        }

        if ((ppu->cycle >= 1 && ppu->cycle <= 256) || (ppu->cycle >= 321 && ppu->cycle <= 336)) {
            if (ppu->cycle == 1) {
                evaluate_sprites(ppu);
            }

            int cycle_mod = ppu->cycle % 8;
            if (cycle_mod == 1) {
                load_background_shifters(ppu);
            } else if (cycle_mod == 0) {
                increment_x(ppu);
            }

            if (cycle_mod == 0) {
                uint16_t addr_lo, addr_hi;
                fetch_tile_data(ppu, &addr_lo, &addr_hi);
                ppu->background_fetch_tile = ppu_read_vram(addr_lo) |
                                            (ppu_read_vram(addr_hi) << 8);
            }
        }

        if (ppu->cycle >= 280 && ppu->cycle <= 304) {
            if (ppu->odd_frame && ppu->cycle == 304) {
                /* Skip last cycle on odd frames */
            } else {
                ppu->reg.scroll.v = (ppu->reg.scroll.v & 0x841F) | (ppu->reg.scroll.t & 0x7BE0);
            }
        }

        if (ppu->cycle >= 1 && ppu->cycle <= 256) {
            ppu->background_shift_lo <<= 1;
            ppu->background_shift_hi <<= 1;
            ppu->attribute_shift_lo <<= 1;
            ppu->attribute_shift_hi <<= 1;
        }

        if (ppu->cycle == 256) {
            increment_y(ppu);
        }

        if (ppu->cycle == 257) {
            ppu->reg.scroll.v = (ppu->reg.scroll.v & 0xFBE0) | (ppu->reg.scroll.t & 0x041F);
        }
    }

    /* Advance cycle */
    ppu->cycle++;
    if (ppu->cycle > 340) {
        ppu->cycle = 0;
        ppu->scanline++;
        if (ppu->scanline > 261) {
            ppu->scanline = 0;
            ppu->frame++;
            ppu->odd_frame = !ppu->odd_frame;
        }
    }

    return frame_complete;
}

void nes_ppu_execute_cycles(nes_ppu_t* ppu, int cycles) {
    for (int i = 0; i < cycles; i++) {
        nes_ppu_step(ppu);
    }
}

/* CPU registers read/write */

void nes_ppu_cpu_write(nes_ppu_t* ppu, uint16_t addr, uint8_t val) {
    static int debug_mode = 0;  /* Set to 1 for PPU write debugging */
    if (debug_mode && addr <= 7) {
        printf("PPU write: $%04X = $%02X (addr&7=$%X)\n", 0x2000 + (addr & 7), val, addr & 7);
        fflush(stdout);
    }
    addr &= 0x0007;

    switch (addr) {
        case 0:  /* PPUCTRL */
            ppu->reg.ctrl = val;
            if (val & PPUCTRL_NMI) {
                /* If VBlank already in progress and NMI not yet triggered */
                if (ppu->reg.status & PPUSTATUS_VBLANK) {
                    /* May trigger immediate NMI */
                }
            }
            ppu->reg.scroll.t = (ppu->reg.scroll.t & 0x73FF) | ((val & 0x03) << 10);
            break;

        case 1:  /* PPUMASK */
            ppu->reg.mask = val;
            break;

        case 3:  /* OAMADDR */
            ppu->reg.oam_addr = val;
            break;

        case 4:  /* OAMDATA */
            ppu->oam[ppu->reg.oam_addr++] = val;
            break;

        case 5:  /* PPUSCROLL */
            if (!ppu->reg.scroll.w) {
                write_ppu_scroll_x(ppu, val);
            } else {
                write_ppu_scroll_y(ppu, val);
            }
            break;

        case 6:  /* PPUADDR */
            write_ppu_addr(ppu, val);
            break;

        case 7:  /* PPUDATA */
            ppu_write_vram(ppu, ppu->reg.scroll.v, val);
            increment_vram_addr(ppu);
            break;
    }
}

uint8_t nes_ppu_cpu_read(nes_ppu_t* ppu, uint16_t addr) {
    addr &= 0x0007;
    uint8_t result = 0;

    switch (addr) {
        case 2:  /* PPUSTATUS */
            result = ppu->reg.status & (PPUSTATUS_VBLANK | PPUSTATUS_SP0_HIT | PPUSTATUS_SP_OVF);
            result |= ppu->reg.data_buffer & 0x1F;  /* Open bus */
            ppu->reg.status &= ~PPUSTATUS_VBLANK;
            ppu->reg.scroll.w = 0;
            break;

        case 4:  /* OAMDATA */
            result = ppu->oam[ppu->reg.oam_addr];
            break;

        case 7:  /* PPUDATA */
            result = ppu->reg.data_buffer;
            ppu->reg.data_buffer = ppu_read_vram(ppu->reg.scroll.v);

            /* Palette reads are not buffered */
            if ((ppu->reg.scroll.v & 0x3FFF) >= 0x3F00) {
                result = ppu->reg.data_buffer;
            }
            increment_vram_addr(ppu);
            break;
    }

    return result;
}

uint32_t nes_ppu_get_rgba_color(nes_ppu_t* ppu, uint8_t val) {
    uint8_t idx;
    if (val & 0x10) {  /* Sprite */
        idx = ((val & 0x0F) | 0x10) & 0x1F;
    } else if (val & 0x20) {  /* Background */
        idx = (val & 0x0F) + 4;
    } else {
        idx = 0;
    }

    uint8_t palette_entry = ppu->palette[idx];
    if (palette_entry & 0x10) palette_entry = 0x0F;
    if (ppu->reg.mask & PPUMASK_GRAYSCALE) {
        /* Grayscale - just use the low 3 bits */
        palette_entry = (palette_entry & 0x30) | ((palette_entry & 0x0F) ? 0x13 : 0);
    }

    uint8_t r = g_nes_palette[palette_entry % 64][0];
    uint8_t g = g_nes_palette[palette_entry % 64][1];
    uint8_t b = g_nes_palette[palette_entry % 64][2];

    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

void nes_ppu_render_frame(nes_ppu_t* ppu, uint32_t* buffer) {
    for (int y = 0; y < PPU_HEIGHT; y++) {
        for (int x = 0; x < PPU_WIDTH; x++) {
            uint8_t pixel = g_frame_buffer[y * PPU_WIDTH + x];
            buffer[y * PPU_WIDTH + x] = nes_ppu_get_rgba_color(ppu, pixel);
        }
    }
}

const uint8_t* nes_ppu_get_frame_buffer(nes_ppu_t* ppu) {
    (void)ppu;  /* unused */
    return g_frame_buffer;
}

void nes_ppu_set_bus(nes_ppu_t* ppu, ppu_bus_t* bus) {
    /* Copy bus configuration to static storage */
    if (bus) {
        g_ppu_bus_static.context = bus->context;
        g_ppu_bus_static.read_chr = bus->read_chr;
        g_ppu_bus_static.write_chr = bus->write_chr;
        g_ppu_bus_static.ppu_write_cpu = bus->ppu_write_cpu;
    }
    g_ppu_bus = &g_ppu_bus_static;
    (void)ppu;  /* unused */
}

void nes_ppu_nmi_enabled_check(nes_ppu_t* ppu, int* nmi_pending) {
    if ((ppu->reg.status & PPUSTATUS_VBLANK) && (ppu->reg.ctrl & PPUCTRL_NMI)) {
        *nmi_pending = 1;
    }
}

int nes_ppu_in_vblank(nes_ppu_t* ppu) {
    return (ppu->reg.status & PPUSTATUS_VBLANK) != 0;
}

void nes_ppu_set_mirror_mode(uint8_t mirroring) {
    switch (mirroring) {
        case 0: g_mirror_mode = MIRROR_HORIZONTAL; break;
        case 1: g_mirror_mode = MIRROR_VERTICAL; break;
        case 2: g_mirror_mode = MIRROR_SINGLE_0; break;
        case 3: g_mirror_mode = MIRROR_SINGLE_1; break;
    }
}

void nes_ppu_trigger_nmi(nes_ppu_t* ppu) {
    if (ppu->reg.ctrl & PPUCTRL_NMI) {
        ppu->reg.status |= PPUSTATUS_VBLANK;
    }
}

/* DMA transfer for OAM */
void nes_ppu_oam_dma(nes_ppu_t* ppu, const uint8_t* page_data) {
    for (int i = 0; i < 256; i++) {
        ppu->oam[(ppu->reg.oam_addr + i) & 0xFF] = page_data[i];
    }
}
