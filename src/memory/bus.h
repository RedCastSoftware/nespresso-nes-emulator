/**
 * NESPRESSO - NES Emulator
 * Bus Module - Memory Bus and System Integration
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_BUS_H
#define NESPRESSO_BUS_H

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
typedef struct nes_cpu nes_cpu_t;
typedef struct nes_ppu nes_ppu_t;
typedef struct nes_apu nes_apu_t;
typedef struct nes_mapper nes_mapper_t;
typedef struct nes_cartridge nes_cartridge_t;
typedef struct nes_input_state nes_input_t;

#ifdef __cplusplus
extern "C" {
#endif

/* System Memory */
#define NES_RAM_SIZE        2048  /* 2KB internal RAM */
#define NES_RAM_END         0x07FF
#define NES_RAM_MIRRORS     0x1FFF

/* System state */
typedef struct nes_system {
    /* Components */
    nes_cpu_t*      cpu;
    nes_ppu_t*      ppu;
    nes_apu_t*      apu;
    nes_input_t*    input;
    nes_cartridge_t* cartridge;
    nes_mapper_t*   mapper;

    /* Memory */
    uint8_t         ram[NES_RAM_SIZE];

    /* Frame timing */
    uint32_t        cpu_cycles_per_frame;
    uint32_t        ppu_cycles_per_frame;
    int             frame_complete;

    /* Flags */
    int             running;
    int             paused;
} nes_system_t;

/* CPU Address Space Layout */
#define NES_ADDR_CPU_RAM        0x0000  /* Internal RAM */
#define NES_ADDR_PPU_REG        0x2000  /* PPU registers */
#define NES_ADDR_APU_REG        0x4000  /* APU registers */
#define NES_ADDR_INPUT_REG      0x4016  /* Controller registers */
#define NES_ADDR_OAM_DMA        0x4014  /* OAM DMA */
#define NES_ADDR_CARTRIDGE      0x4020  /* Cartridge space */

/* APU Registers */
#define APU_STATUS             0x4015

/* Controller registers */
#define CONTROLLER_1           0x4016
#define CONTROLLER_2           0x4017

/* System Control Registers */
#define JOY_STROBE             0x4016

/* DMA transfer */
#define OAM_DMA_ADDR           0x4014

/* API Functions */

/**
 * Initialize NES system
 */
int nes_sys_init(nes_system_t* sys);

/**
 * Free NES system resources
 */
void nes_sys_free(nes_system_t* sys);

/**
 * Reset NES system
 */
void nes_sys_reset(nes_system_t* sys);

/**
 * Load ROM into system
 */
int nes_sys_load_rom(nes_system_t* sys, const char* filename);

/**
 * System step - execute one frame
 * Returns 1 when a frame is complete
 */
int nes_sys_step_frame(nes_system_t* sys);

/**
 * Get frame buffer from PPU
 */
const uint8_t* nes_sys_get_frame_buffer(nes_system_t* sys);

/**
 * Render frame to RGBA buffer
 */
void nes_sys_render_frame(nes_system_t* sys, uint32_t* buffer);

/**
 * Generate audio samples
 */
int nes_sys_get_audio(nes_system_t* sys, float* buffer, int max_samples);

/**
 * CPU read from address
 */
uint8_t nes_sys_cpu_read(nes_system_t* sys, uint16_t addr);

/**
 * CPU write to address
 */
void nes_sys_cpu_write(nes_system_t* sys, uint16_t addr, uint8_t val);

/**
 * PPU read from address
 */
uint8_t nes_sys_ppu_read(nes_system_t* sys, uint16_t addr);

/**
 * PPU write to address
 */
void nes_sys_ppu_write(nes_system_t* sys, uint16_t addr, uint8_t val);

/**
 * Handle OAM DMA transfer
 */
void nes_sys_oam_dma(nes_system_t* sys, uint8_t page);

/**
 * Get system running state
 */
int nes_sys_is_running(const nes_system_t* sys);

/**
 * Set system running state
 */
void nes_sys_set_running(nes_system_t* sys, int running);

/**
 * Save state to file
 */
int nes_sys_save_state(nes_system_t* sys, const char* filename);

/**
 * Load state from file
 */
int nes_sys_load_state(nes_system_t* sys, const char* filename);

/* CPU-PPU Cycle Ratio */
#define NES_CPU_PPU_RATIO 3  /* 3 CPU cycles = 1 PPU cycle */

/* Frame timing */
#define NES_FRAMES_PER_SECOND 60
#define NES_CPU_CYCLES_PER_FRAME 29780  /* NTSC */
#define NES_PPU_CYCLES_PER_FRAME 89341  /* NTSC */

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_BUS_H */
