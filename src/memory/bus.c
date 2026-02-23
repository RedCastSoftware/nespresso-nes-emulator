/**
 * NESPRESSO - NES Emulator
 * Bus Module - Memory Bus and System Integration
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "bus.h"
#include "../cpu/cpu.h"
#include "../ppu/ppu.h"
#include "../apu/apu.h"
#include "../input/input.h"
#include "../cartridge/rom.h"
#include "../mapper/mapper.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Global running flag */
static nes_system_t* g_system = NULL;

int nes_sys_init(nes_system_t* sys) {
    memset(sys, 0, sizeof(nes_system_t));

    /* Allocate components */
    sys->cpu = (nes_cpu_t*)malloc(sizeof(nes_cpu_t));
    sys->ppu = (nes_ppu_t*)malloc(sizeof(nes_ppu_t));
    sys->apu = (nes_apu_t*)malloc(sizeof(nes_apu_t));
    sys->input = (nes_input_t*)malloc(sizeof(nes_input_t));
    sys->cartridge = (nes_cartridge_t*)malloc(sizeof(nes_cartridge_t));
    sys->mapper = (nes_mapper_t*)malloc(sizeof(nes_mapper_t));

    if (!sys->cpu || !sys->ppu || !sys->apu || !sys->input ||
        !sys->cartridge || !sys->mapper) {
        nes_sys_free(sys);
        return -1;
    }

    /* Initialize components */
    nes_cpu_init(sys->cpu, NULL);
    nes_ppu_init(sys->ppu, NULL);
    nes_apu_init(sys->apu, NULL);
    nes_input_init(sys->input);
    nes_cartridge_init(sys->cartridge);

    /* Set global reference */
    g_system = sys;

    /* Set up bus */
    cpu_bus_t cpu_bus = {
        .context = sys,
        .read = (cpu_read_func)nes_sys_cpu_read,
        .write = (cpu_write_func)nes_sys_cpu_write
    };
    nes_cpu_set_bus(sys->cpu, &cpu_bus);

    ppu_bus_t ppu_bus = {
        .context = sys,
        .read_chr = (uint8_t (*)(void*, uint16_t))nes_sys_ppu_read,
        .write_chr = (void (*)(void*, uint16_t, uint8_t))nes_sys_ppu_write,
        .ppu_write_cpu = NULL,  /* Not currently used in callbacks */
    };
    nes_ppu_set_bus(sys->ppu, &ppu_bus);
    /* Note: APU uses system bus for DMA - set separately if needed */

    sys->cpu_cycles_per_frame = NES_CPU_CYCLES_PER_FRAME;
    sys->ppu_cycles_per_frame = NES_PPU_CYCLES_PER_FRAME;
    sys->running = 1;
    sys->paused = 0;

    return 0;
}

void nes_sys_free(nes_system_t* sys) {
    if (sys->cartridge) {
        nes_cartridge_free(sys->cartridge);
        free(sys->cartridge);
    }
    if (sys->mapper) {
        nes_mapper_destroy(sys->mapper);
        free(sys->mapper);
    }
    if (sys->cpu) free(sys->cpu);
    if (sys->ppu) free(sys->ppu);
    if (sys->apu) free(sys->apu);
    if (sys->input) free(sys->input);

    memset(sys, 0, sizeof(nes_system_t));
}

void nes_sys_reset(nes_system_t* sys) {
    printf("nes_sys_reset called\n");
    printf("  cpu=%p, ppu=%p, apu=%p, input=%p, mapper=%p\n",
           (void*)sys->cpu, (void*)sys->ppu, (void*)sys->apu, (void*)sys->input, (void*)sys->mapper);

    if (sys->cpu) { printf("  Calling nes_cpu_reset...\n"); nes_cpu_reset(sys->cpu); }
    if (sys->ppu) { printf("  Calling nes_ppu_reset...\n"); nes_ppu_reset(sys->ppu); }
    if (sys->apu) { printf("  Calling nes_apu_reset...\n"); nes_apu_reset(sys->apu); }
    if (sys->input) { printf("  Calling nes_input_reset...\n"); nes_input_reset(sys->input); }
    if (sys->mapper) { printf("  Calling nes_mapper_reset...\n"); nes_mapper_reset(sys->mapper); }
    printf("nes_sys_reset complete\n");
}

int nes_sys_load_rom(nes_system_t* sys, const char* filename) {
    printf("Loading ROM from: %s\n", filename);

    nes_rom_result_t result = nes_cartridge_load(sys->cartridge, filename);
    if (result != NES_ROM_OK) {
        fprintf(stderr, "Failed to load ROM: %d\n", result);
        return -1;
    }
    printf("ROM loaded, mapper=%d\n", sys->cartridge->info.mapper);

    /* Create mapper */
    printf("Creating mapper...\n");
    if (nes_mapper_create(sys->cartridge, sys->mapper, sys->ppu) != 0) {
        fprintf(stderr, "Failed to create mapper (mapper %d may not be supported yet)\n", sys->cartridge->info.mapper);
        return -1;
    }
    printf("Mapper created\n");

    /* Set mirroring */
    int mirror = sys->cartridge->info.mirroring;
    extern void nes_ppu_set_mirror_mode(uint8_t);
    nes_ppu_set_mirror_mode((uint8_t)mirror);

    /* Reset system with new cartridge */
    printf("Resetting system...\n");
    nes_sys_reset(sys);
    printf("System ready\n");

    /* Print ROM info */
    char info[256];
    nes_cartridge_get_info_string(sys->cartridge, info, sizeof(info));
    printf("%s\n", info);

    return 0;
}

/* CPU Read - called from 6502 emulation */
uint8_t nes_sys_cpu_read(nes_system_t* sys, uint16_t addr) {
    addr &= 0xFFFF;

    /* Internal RAM */
    if (addr < NES_RAM_MIRRORS) {
        return sys->ram[addr & NES_RAM_END];
    }

    /* PPU registers */
    if (addr >= NES_ADDR_PPU_REG && addr < (NES_ADDR_PPU_REG + 8)) {
        return nes_ppu_cpu_read(sys->ppu, addr & 7);
    }

    /* APU/Input registers */
    if (addr >= NES_ADDR_APU_REG && addr < NES_ADDR_CARTRIDGE) {
        switch (addr) {
            case 0x4015:  /* APU Status */
                return nes_apu_cpu_read(sys->apu, addr);

            case CONTROLLER_1:
            case CONTROLLER_2:
                return nes_input_read(sys->input, addr == CONTROLLER_1 ? 0 : 1);

            default:
                return nes_apu_cpu_read(sys->apu, addr);
        }
    }

    /* Cartridge space */
    if (addr >= NES_ADDR_CARTRIDGE && sys->mapper) {
        if (addr >= 0x6000 && addr < 0x8000) {
            /* PRG-RAM */
            return nes_cartridge_read_prg_ram(sys->cartridge, addr - 0x6000);
        }
        if (sys->mapper->cpu_read) {
            return sys->mapper->cpu_read(sys->mapper->context, addr);
        }
    }

    /* Open bus */
    return sys->ram[addr & NES_RAM_END];
}

/* CPU Write - called from 6502 emulation */
void nes_sys_cpu_write(nes_system_t* sys, uint16_t addr, uint8_t val) {
    addr &= 0xFFFF;

    /* Internal RAM */
    if (addr < NES_RAM_MIRRORS) {
        sys->ram[addr & NES_RAM_END] = val;
        return;
    }

    /* PPU registers */
    if (addr >= NES_ADDR_PPU_REG && addr < (NES_ADDR_PPU_REG + 8)) {
        nes_ppu_cpu_write(sys->ppu, addr & 7, val);
        return;
    }

    /* APU/Input registers */
    if (addr >= NES_ADDR_APU_REG && addr < NES_ADDR_CARTRIDGE) {
        switch (addr) {
            case OAM_DMA_ADDR:
                nes_sys_oam_dma(sys, val);
                return;

            case JOY_STROBE:
                nes_input_write_strobe(sys->input, val);
                return;

            default:
                nes_apu_cpu_write(sys->apu, addr, val);
                return;
        }
    }

    /* Cartridge space */
    if (addr >= 0x6000 && sys->mapper) {
        if (addr >= 0x6000 && addr < 0x8000) {
            nes_cartridge_write_prg_ram(sys->cartridge, addr - 0x6000, val);
        }
        if (sys->mapper->cpu_write) {
            sys->mapper->cpu_write(sys->mapper->context, addr, val);
        }
        return;
    }
}

/* PPU Read - CHR-ROM access */
uint8_t nes_sys_ppu_read(nes_system_t* sys, uint16_t addr) {
    if (sys->mapper && sys->mapper->ppu_read) {
        return sys->mapper->ppu_read(sys->mapper->context, addr);
    }
    return nes_cartridge_read_chr(sys->cartridge, addr);
}

/* PPU Write - CHR-RAM access */
void nes_sys_ppu_write(nes_system_t* sys, uint16_t addr, uint8_t val) {
    if (sys->mapper && sys->mapper->ppu_write) {
        sys->mapper->ppu_write(sys->mapper->context, addr, val);
    } else {
        nes_cartridge_write_chr_ram(sys->cartridge, addr, val);
    }
}

/* OAM DMA */
void nes_sys_oam_dma(nes_system_t* sys, uint8_t page) {
    uint8_t* page_data = &sys->ram[page << 8];

    /* Update CPU timing - DMA takes 513 or 514 cycles */
    extern void nes_cpu_execute_cycles(nes_cpu_t*, uint32_t);
    nes_cpu_execute_cycles(sys->cpu, 514);

    /* Transfer data to OAM */
    extern void nes_ppu_oam_dma(nes_ppu_t*, const uint8_t*);
    nes_ppu_oam_dma(sys->ppu, page_data);
}

/* Run one frame */
int nes_sys_step_frame(nes_system_t* sys) {
    if (!sys->running || sys->paused) {
        return 0;
    }

    sys->frame_complete = 0;

    /* Execute CPU for one frame's worth of cycles */
    /* Run in sync with PPU - 3 CPU cycles per PPU cycle */
    uint32_t ppu_cycles = NES_PPU_CYCLES_PER_FRAME;
    uint32_t cpu_cycles = NES_CPU_CYCLES_PER_FRAME;

    /* Interleave PPU scanning */
    for (uint32_t i = 0; i < ppu_cycles && !sys->frame_complete; i++) {
        int vblank = nes_ppu_step(sys->ppu);

        /* Trigger NMI in CPU */
        if (vblank && (sys->ppu->reg.ctrl & PPUCTRL_NMI)) {
            nes_cpu_trigger_nmi(sys->cpu);
        }

        /* Run 3 CPU cycles per 1 PPU cycle */
        nes_cpu_execute_cycles(sys->cpu, NES_CPU_PPU_RATIO);
    }

    /* Run APU for the frame */
    nes_apu_execute_cycles(sys->apu, cpu_cycles);

    sys->frame_complete = 1;
    return 1;
}

/* Get frame buffer */
const uint8_t* nes_sys_get_frame_buffer(nes_system_t* sys) {
    return nes_ppu_get_frame_buffer(sys->ppu);
}

/* Render to RGBA */
void nes_sys_render_frame(nes_system_t* sys, uint32_t* buffer) {
    nes_ppu_render_frame(sys->ppu, buffer);
}

/* Get audio samples */
int nes_sys_get_audio(nes_system_t* sys, float* buffer, int max_samples) {
    return nes_apu_generate_samples(sys->apu, buffer, max_samples);
}

/* Running state */
int nes_sys_is_running(const nes_system_t* sys) {
    return sys->running;
}

void nes_sys_set_running(nes_system_t* sys, int running) {
    sys->running = running;
}

/* Save state */
int nes_sys_save_state(nes_system_t* sys, const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) return -1;

    /* Save CPU state */
    fwrite(sys->cpu, sizeof(nes_cpu_t), 1, f);

    /* Save PPU state */
    fwrite(sys->ppu, sizeof(nes_ppu_t), 1, f);

    /* Save APU state */
    fwrite(sys->apu, sizeof(nes_apu_t), 1, f);

    /* Save input state */
    fwrite(sys->input, sizeof(nes_input_t), 1, f);

    /* Save RAM */
    fwrite(sys->ram, NES_RAM_SIZE, 1, f);

    /* Save mapper state */
    if (sys->mapper) {
        fwrite(&sys->mapper->state, sizeof(mapper_buffer_t), 1, f);
    }

    fclose(f);
    return 0;
}

/* Load state */
int nes_sys_load_state(nes_system_t* sys, const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) return -1;

    /* Load CPU state */
    fread(sys->cpu, sizeof(nes_cpu_t), 1, f);

    /* Load PPU state */
    fread(sys->ppu, sizeof(nes_ppu_t), 1, f);

    /* Load APU state */
    fread(sys->apu, sizeof(nes_apu_t), 1, f);

    /* Load input state */
    fread(sys->input, sizeof(nes_input_t), 1, f);

    /* Load RAM */
    fread(sys->ram, NES_RAM_SIZE, 1, f);

    /* Load mapper state */
    if (sys->mapper) {
        fread(&sys->mapper->state, sizeof(mapper_buffer_t), 1, f);
    }

    fclose(f);
    return 0;
}
