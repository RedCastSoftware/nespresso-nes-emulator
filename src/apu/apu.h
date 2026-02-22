/**
 * NESPRESSO - NES Emulator
 * APU Module - Audio Processing Unit
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_APU_H
#define NESPRESSO_APU_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* APU Constants */
#define APU_SAMPLE_RATE    44100
#define APU_FRAME_RATE     60
#define APU_CPU_CLOCK_NTSC 1789773
#define APU_CPU_CYCLE_RATE 1789773.0
#define APU_SAMPLES_PER_FRAME (APU_SAMPLE_RATE / APU_FRAME_RATE)

/* APU Registers */
#define APU_SQUARE1_CTRL    0x4000  /* $4000 */
#define APU_SQUARE1_SWEEP   0x4001  /* $4001 */
#define APU_SQUARE1_TIMER   0x4002  /* $4002 */
#define APU_SQUARE1_LENGTH  0x4003  /* $4003 */
#define APU_SQUARE2_CTRL    0x4004  /* $4004 */
#define APU_SQUARE2_SWEEP   0x4005  /* $4005 */
#define APU_SQUARE2_TIMER   0x4006  /* $4006 */
#define APU_SQUARE2_LENGTH  0x4007  /* $4007 */
#define APU_TRIANGLE_CTRL   0x4008  /* $4008 */
#define APU_TRIANGLE_LEN0   0x400A  /* $400A */
#define APU_TRIANGLE_LEN1   0x400B  /* $400B */
#define APU_NOISE_CTRL     0x400C  /* $400C */
#define APU_NOISE_LEN0     0x400E  /* $400E */
#define APU_NOISE_LEN1     0x400F  /* $400F */
#define APU_DMC_START      0x4010  /* $4010 */
#define APU_DMC_LEN        0x4011  /* $4011 */
#define APU_DMC_ADDR       0x4012  /* $4012 */
#define APU_DMC_LEN_       0x4013  /* $4013 */
#define APU_STATUS         0x4015  /* $4015 */
#define APU_FRAME_COUNTER  0x4017  /* $4017 */

/* Square Wave Duty Cycle Sequences */
extern const uint8_t g_square_duty_table[4][8];

/* Linear Feedback Shift Register (Noise) */
extern uint16_t apu_lfsr_short(uint16_t lfsr);
extern uint16_t apu_lfsr_long(uint16_t lfsr);

/* Triangle Wave Sequence (32 steps) */
extern const uint8_t g_triangle_seq[32];

/* Length Counter Table (lookup) */
extern const uint8_t g_length_table[32];

/* Envelope Divider */
typedef struct {
    uint8_t  start;
    uint8_t  loop;
    uint8_t  constant;
    uint8_t  volume;
    uint8_t  divider;
    uint8_t  counter;
    uint8_t  decay_level;
} apu_envelope_t;

/* Sweep Unit */
typedef struct {
    uint8_t  enabled;
    uint8_t  period;
    uint8_t  negate;
    uint8_t  shift;
    uint8_t  reload;
    uint8_t  divider;
    uint8_t  one;
} apu_sweep_t;

/* Square Wave Channel */
typedef struct {
    apu_envelope_t   envelope;
    apu_sweep_t      sweep;
    uint16_t         timer_period;
    uint16_t         timer_value;
    uint8_t          duty;
    uint8_t          duty_index;
    uint8_t          length_counter;
    uint8_t          length_enabled;
} apu_square_t;

/* Triangle Wave Channel */
typedef struct {
    uint8_t  length_counter;
    uint8_t  length_enabled;
    uint16_t timer_period;
    uint16_t timer_value;
    uint8_t  linear_counter;
    uint8_t  linear_counter_reload;
    uint8_t  linear_counter_control;
    uint8_t  linear_counter_reload_flag;
    uint8_t  linear_counter_value;
    uint8_t  sequencing;
} apu_triangle_t;

/* Noise Channel */
typedef struct {
    apu_envelope_t   envelope;
    uint16_t        period;
    uint16_t        timer_value;
    uint8_t         mode;            /* 0 = short, 1 = long */
    uint8_t         length_counter;
    uint8_t         length_enabled;
    uint16_t        lfsr;
} apu_noise_t;

/* DMC (Delta Modulation Channel) Channel */
typedef struct {
    uint8_t  enabled;
    uint8_t  irq;
    uint8_t  loop;
    uint16_t sample_addr;      /* Current sample address ($C000-$FFFF) */
    uint16_t sample_length;    /* Remaining bytes to read */
    uint16_t current_addr;     /* Current address during DMA */
    uint16_t bytes_remaining;  /* Bytes remaining for current sample */
    uint8_t  sample_buffer;    /* Buffer for current sample */
    uint8_t  sample_buffer_empty;
    uint8_t  shift_register;
    uint8_t  bits_remaining;
    uint8_t  silence;
    uint8_t  output;
    uint8_t  timer_period;
    uint8_t  timer_value;
} apu_dmc_t;

/* APU Frame Counter */
typedef struct {
    uint8_t  mode;       /* 0 = 4-step, 1 = 5-step */
    uint8_t  irq;
    uint8_t  counter;
} apu_frame_counter_t;

/* APU State */
typedef struct nes_apu {
    /* Channels */
    apu_square_t     square1;
    apu_square_t     square2;
    apu_triangle_t   triangle;
    apu_noise_t      noise;
    apu_dmc_t        dmc;

    /* Frame counter */
    apu_frame_counter_t frame;

    /* Timing */
    uint32_t         cycle_count;
    uint32_t         frame_cycle;    /* Cycles in current frame */

    /* External callbacks */
    void*            context;
    uint8_t (*read_dmc)(void* ctx, uint16_t addr);
    void (*irq_callback)(void* ctx);
} nes_apu_t;

/* APU Bus Interface */
typedef struct {
    void*   context;
    uint8_t (*read)(void* ctx, uint16_t addr);
    void    (*write)(void* ctx, uint16_t addr, uint8_t val);
} apu_bus_t;

/* API Functions */

/**
 * Initialize APU
 */
void nes_apu_init(nes_apu_t* apu, apu_bus_t* bus);

/**
 * Reset APU to initial state
 */
void nes_apu_reset(nes_apu_t* apu);

/**
 * Step APU by one CPU cycle
 */
void nes_apu_step(nes_apu_t* apu);

/**
 * Execute N APU cycles
 */
void nes_apu_execute_cycles(nes_apu_t* apu, uint32_t cycles);

/**
 * Write to APU register from CPU
 */
void nes_apu_cpu_write(nes_apu_t* apu, uint16_t addr, uint8_t val);

/**
 * Read from APU register via CPU
 */
uint8_t nes_apu_cpu_read(nes_apu_t* apu, uint16_t addr);

/**
 * Generate audio samples for one frame
 * buffer must be at least APU_SAMPLES_PER_FRAME samples
 * Returns number of samples generated
 */
int nes_apu_generate_samples(nes_apu_t* apu, float* buffer, int max_samples);

/**
 * Get current square channel output (for debugging)
 */
uint8_t nes_apu_square_output(nes_apu_t* apu, int channel);

/**
 * Get current triangle channel output (for debugging)
 */
uint8_t nes_apu_triangle_output(nes_apu_t* apu);

/**
 * Get current noise channel output (for debugging)
 */
uint8_t nes_apu_noise_output(nes_apu_t* apu);

/**
 * Get current DMC channel output (for debugging)
 */
uint8_t nes_apu_dmc_output(nes_apu_t* apu);

/**
 * Set APU bus for DMC DMA access
 */
void nes_apu_set_bus(nes_apu_t* apu, apu_bus_t* bus);

/**
 * Get mixed audio output (all channels combined)
 * Returns value between 0.0 and 1.0
 */
float nes_apu_get_output(nes_apu_t* apu);

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_APU_H */
