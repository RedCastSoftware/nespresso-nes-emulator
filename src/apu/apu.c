/**
 * NESPRESSO - NES Emulator
 * APU Module - Audio Processing Unit Implementation
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "apu.h"
#include <string.h>
#include <math.h>

/* Square Wave Duty Cycle Sequences */
const uint8_t g_square_duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},  /* 12.5% - 0x00 */
    {0, 1, 1, 0, 0, 0, 0, 0},  /* 25%   - 0x40 */
    {0, 1, 1, 1, 1, 0, 0, 0},  /* 50%   - 0x80 */
    {1, 0, 0, 1, 1, 1, 1, 1},  /* 25%   - 0xC0 (inverted phase) */
};

/* Triangle Wave Sequence (32 steps) */
const uint8_t g_triangle_seq[32] = {
    15, 14, 13, 12, 11, 10,  9,  8,
     7,  6,  5,  4,  3,  2,  1,  0,
     0,  1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, 14, 15
};

/* Length Counter Table */
const uint8_t g_length_table[32] = {
    10, 254, 20,  2, 40,  4, 80,  6,
    160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22,
    192, 24, 72, 26, 16, 28, 32, 30
};

/* Noise Timer Periods */
const uint16_t g_noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};

/* DMC Timer Periods (in CPU cycles) */
const uint16_t g_dmc_period_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 84, 72, 54
};

/* Static APU bus */
static apu_bus_t* g_apu_bus = NULL;

/* LFSR Functions */
uint16_t apu_lfsr_short(uint16_t lfsr) {
    uint16_t bit = ((lfsr & 1) ^ ((lfsr & 2) >> 1)) & 1;
    return (lfsr >> 1) | (bit << 14);
}

uint16_t apu_lfsr_long(uint16_t lfsr) {
    uint16_t bit = ((lfsr & 1) ^ ((lfsr & 0x40) >> 6)) & 1;
    return (lfsr >> 1) | (bit << 14);
}

/* Envelope Helpers */
static void envelope_clock(apu_envelope_t* env) {
    if (env->start) {
        env->start = 0;
        env->decay_level = 15;
        env->divider = env->volume;
    } else {
        if (env->divider == 0) {
            env->divider = env->volume;
            if (env->decay_level > 0) {
                env->decay_level--;
            } else if (env->loop) {
                env->decay_level = 15;
            }
        } else {
            env->divider--;
        }
    }
}

static uint8_t envelope_output(apu_envelope_t* env) {
    return env->constant ? env->volume : env->decay_level;
}

/* Square Channel Helpers */
static void square_clock(apu_square_t* sq, apu_sweep_t* sweep) {
    /* Clock length counter */
    if (!sq->length_enabled && sq->length_counter > 0) {
        sq->length_counter--;
    }

    /* Clock sweep unit */
    if (sweep->divider == 0 || sweep->reload) {
        sweep->reload = 0;
        if (sweep->enabled && sweep->divider == 0) {
            uint16_t delta = sq->timer_period >> sweep->shift;
            if (sweep->negate) {
                delta = ~delta + 1;
                if (sweep->one) delta -= 1;
            }
            uint16_t target = sq->timer_period + delta;
            if (target <= 0x7FF) {
                sq->timer_period = target;
            }
            sq->length_counter = 0;  /* Mute */
        }
        sweep->divider = sweep->period;
    } else {
        sweep->divider--;
    }
}

static void square_timer_clock(apu_square_t* sq, apu_sweep_t* sweep) {
    if (sq->timer_value == 0) {
        sq->timer_value = sq->timer_period;
        sq->duty_index = (sq->duty_index + 1) & 7;
    } else {
        sq->timer_value--;
    }
}

static uint8_t square_output(apu_square_t* sq) {
    if (sq->length_counter == 0) return 0;
    if (g_square_duty_table[sq->duty][sq->duty_index] == 0) return 0;

    /* Sweep muting */
    if (sq->timer_period < 8 || sq->timer_period > 0x7FF) return 0;

    return 1;
}

/* Triangle Channel Helpers */
static void triangle_clock_length(apu_triangle_t* tri) {
    if (!tri->length_enabled && tri->length_counter > 0) {
        tri->length_counter--;
    }
}

static void triangle_clock_linear(apu_triangle_t* tri) {
    if (tri->linear_counter_reload_flag) {
        tri->linear_counter = tri->linear_counter_reload;
    } else if (tri->linear_counter > 0) {
        tri->linear_counter--;
    }

    if (!tri->linear_counter_control) {
        tri->linear_counter_reload_flag = 0;
    }
}

static void triangle_timer_clock(apu_triangle_t* tri) {
    if (tri->timer_value == 0) {
        tri->timer_value = tri->timer_period;
        if (tri->length_counter > 0 && tri->linear_counter > 0) {
            tri->sequencing = (tri->sequencing + 1) & 31;
        }
    } else {
        tri->timer_value--;
    }
}

static uint8_t triangle_output(apu_triangle_t* tri) {
    if (tri->length_counter == 0) return 0;
    if (tri->linear_counter == 0) return 0;
    return g_triangle_seq[tri->sequencing];
}

/* Noise Channel Helpers */
static void noise_clock(apu_noise_t* noise) {
    if (!noise->length_enabled && noise->length_counter > 0) {
        noise->length_counter--;
    }

    envelope_clock(&noise->envelope);
}

static void noise_timer_clock(apu_noise_t* noise) {
    if (noise->timer_value == 0) {
        /* LFSR step */
        uint16_t bit = noise->lfsr & 1;
        uint16_t feedback;
        if (noise->mode) {
            /* Short period mode */
            feedback = bit ^ ((noise->lfsr >> 6) & 1);
        } else {
            /* Long period mode */
            feedback = bit ^ ((noise->lfsr >> 1) & 1);
        }
        noise->lfsr = (noise->lfsr >> 1) | (feedback << 14);
        noise->timer_value = noise->period;
    } else {
        noise->timer_value--;
    }
}

static uint8_t noise_output(apu_noise_t* noise) {
    if (noise->length_counter == 0) return 0;
    if ((noise->lfsr & 1) == 0) return 0;
    return envelope_output(&noise->envelope);
}

/* DMC Channel Helpers */
static void dmc_clock_reader(void (*dmc_read)(void*, uint16_t, uint8_t*), void* ctx, apu_dmc_t* dmc) {
    if (!dmc->sample_buffer_empty) return;

    if (dmc->bytes_remaining == 0) {
        if (dmc->loop) {
            dmc->current_addr = dmc->sample_addr;
            dmc->bytes_remaining = dmc->sample_length;
        } else if (dmc->irq) {
            /* DMC IRQ */
        }
        return;
    }

    if (dmc_read) {
        dmc_read(ctx, dmc->current_addr, &dmc->sample_buffer);
    } else if (g_apu_bus && g_apu_bus->read) {
        dmc->sample_buffer = g_apu_bus->read(g_apu_bus->context, dmc->current_addr);
    }
    dmc->sample_buffer_empty = 0;
    dmc->current_addr = (dmc->current_addr + 1) | 0x8000;
    dmc->bytes_remaining--;
}

static void dmc_clock_shifter(apu_dmc_t* dmc) {
    if (dmc->silence) return;

    if (dmc->shift_register & 1) {
        if (dmc->output <= 125) dmc->output += 2;
    } else {
        if (dmc->output >= 2) dmc->output -= 2;
    }
    dmc->shift_register >>= 1;
    dmc->bits_remaining--;
}

static void dmc_timer_clock(apu_dmc_t* dmc) {
    if (dmc->timer_value == 0) {
        dmc->timer_value = dmc->timer_period;

        if (dmc->bits_remaining == 0) {
            dmc->bits_remaining = 8;
            if (dmc->sample_buffer_empty) {
                dmc->silence = 1;
            } else {
                dmc->silence = 0;
                dmc->shift_register = dmc->sample_buffer;
                dmc->sample_buffer_empty = 1;
                dmc_clock_reader(NULL, NULL, dmc);
            }
        }

        dmc_clock_shifter(dmc);
    } else {
        dmc->timer_value--;
    }
}

/* Frame Counter */
static void frame_clock(nes_apu_t* apu, uint8_t step) {
    /* Step 0: Clock envelopes and triangle linear counter */
    if (step == 0 || step == 2) {
        envelope_clock(&apu->square1.envelope);
        envelope_clock(&apu->square2.envelope);
        envelope_clock(&apu->noise.envelope);
        triangle_clock_linear(&apu->triangle);
    }

    /* Step 1: Clock length counters and sweep */
    if (step == 1) {
        apu->square1.length_counter = apu->square1.length_counter > 0 ? apu->square1.length_counter - 1 : 0;
        apu->square2.length_counter = apu->square2.length_counter > 0 ? apu->square2.length_counter - 1 : 0;
        apu->noise.length_counter = apu->noise.length_counter > 0 ? apu->noise.length_counter - 1 : 0;
        apu->triangle.length_counter = apu->triangle.length_counter > 0 ? apu->triangle.length_counter - 1 : 0;
    }
}

/* Public API Implementation */

void nes_apu_init(nes_apu_t* apu, apu_bus_t* bus) {
    memset(apu, 0, sizeof(nes_apu_t));
    g_apu_bus = bus;

    /* Initialize LFSR */
    apu->noise.lfsr = 1;

    /* Initialize DMC */
    apu->dmc.sample_addr = 0xC000;
    apu->dmc.sample_buffer_empty = 1;
    apu->dmc.bits_remaining = 8;
}

void nes_apu_reset(nes_apu_t* apu) {
    memset(&apu->square1, 0, sizeof(apu_square_t));
    memset(&apu->square2, 0, sizeof(apu_square_t));
    memset(&apu->triangle, 0, sizeof(apu_triangle_t));
    memset(&apu->noise, 0, sizeof(apu_noise_t));
    memset(&apu->dmc, 0, sizeof(apu_dmc_t));

    apu->noise.lfsr = 1;
    apu->frame.mode = 0;
    apu->cycle_count = 0;
    apu->frame_cycle = 0;
}

void nes_apu_step(nes_apu_t* apu) {
    uint32_t cycle = apu->cycle_count++;
    apu->frame_cycle = cycle;

    /* Clock timers */
    square_timer_clock(&apu->square1, &apu->square1.sweep);
    square_timer_clock(&apu->square2, &apu->square2.sweep);
    triangle_timer_clock(&apu->triangle);
    noise_timer_clock(&apu->noise);
    dmc_timer_clock(&apu->dmc);

    /* Frame counter timing (every CPU cycle) */
    if (apu->frame.mode == 0) {
        /* 4-step mode */
        if (cycle == 3729) {
            frame_clock(apu, 0);
        } else if (cycle == 7457) {
            frame_clock(apu, 1);
        } else if (cycle == 11186) {
            frame_clock(apu, 0);
        } else if (cycle == 14915) {
            frame_clock(apu, 1);
            if (apu->frame.irq) {
                /* Frame IRQ */
            }
            cycle = 0;
            apu->cycle_count = 0;
        }
    } else {
        /* 5-step mode */
        if (cycle == 3729) {
            frame_clock(apu, 0);
        } else if (cycle == 7457) {
            frame_clock(apu, 1);
        } else if (cycle == 11186) {
            frame_clock(apu, 0);
        } else if (cycle == 14915) {
            /* Cycle 4 in 5-step mode - no clock */
        } else if (cycle == 18641) {
            frame_clock(apu, 0);
            cycle = 0;
            apu->cycle_count = 0;
        }
    }
}

void nes_apu_execute_cycles(nes_apu_t* apu, uint32_t cycles) {
    for (uint32_t i = 0; i < cycles; i++) {
        nes_apu_step(apu);
    }
}

void nes_apu_cpu_write(nes_apu_t* apu, uint16_t addr, uint8_t val) {
    switch (addr) {
        /* Square 1 */
        case 0x4000: {
            apu->square1.duty = (val >> 6) & 3;
            apu->square1.length_enabled = !(val & 0x20);
            apu->square1.envelope.loop = (val & 0x20) != 0;
            apu->square1.envelope.constant = (val & 0x10) != 0;
            apu->square1.envelope.volume = val & 0x0F;
            break;
        }
        case 0x4001: {
            apu->square1.sweep.enabled = (val & 0x80) != 0;
            apu->square1.sweep.period = (val >> 4) & 7;
            apu->square1.sweep.negate = (val & 8) != 0;
            apu->square1.sweep.shift = val & 7;
            apu->square1.sweep.reload = 1;
            break;
        }
        case 0x4002: {
            apu->square1.timer_period = (apu->square1.timer_period & 0x700) | val;
            break;
        }
        case 0x4003: {
            apu->square1.timer_period = (apu->square1.timer_period & 0xFF) | ((val & 7) << 8);
            apu->square1.length_counter = g_length_table[(val >> 3) & 0x1F];
            apu->square1.envelope.start = 1;
            apu->square1.duty_index = 0;
            break;
        }

        /* Square 2 */
        case 0x4004: {
            apu->square2.duty = (val >> 6) & 3;
            apu->square2.length_enabled = !(val & 0x20);
            apu->square2.envelope.loop = (val & 0x20) != 0;
            apu->square2.envelope.constant = (val & 0x10) != 0;
            apu->square2.envelope.volume = val & 0x0F;
            break;
        }
        case 0x4005: {
            apu->square2.sweep.enabled = (val & 0x80) != 0;
            apu->square2.sweep.period = (val >> 4) & 7;
            apu->square2.sweep.negate = (val & 8) != 0;
            apu->square2.sweep.shift = val & 7;
            apu->square2.sweep.reload = 1;
            break;
        }
        case 0x4006: {
            apu->square2.timer_period = (apu->square2.timer_period & 0x700) | val;
            break;
        }
        case 0x4007: {
            apu->square2.timer_period = (apu->square2.timer_period & 0xFF) | ((val & 7) << 8);
            apu->square2.length_counter = g_length_table[(val >> 3) & 0x1F];
            apu->square2.envelope.start = 1;
            apu->square2.duty_index = 0;
            break;
        }

        /* Triangle */
        case 0x4008: {
            apu->triangle.length_enabled = !(val & 0x80);
            apu->triangle.linear_counter_control = (val & 0x80) != 0;
            apu->triangle.linear_counter_reload = val & 0x7F;
            break;
        }
        case 0x400A: {
            apu->triangle.timer_period = (apu->triangle.timer_period & 0x700) | val;
            break;
        }
        case 0x400B: {
            apu->triangle.timer_period = (apu->triangle.timer_period & 0xFF) | ((val & 7) << 8);
            apu->triangle.length_counter = g_length_table[(val >> 3) & 0x1F];
            apu->triangle.linear_counter_reload_flag = 1;
            break;
        }

        /* Noise */
        case 0x400C: {
            apu->noise.length_enabled = !(val & 0x20);
            apu->noise.envelope.loop = (val & 0x20) != 0;
            apu->noise.envelope.constant = (val & 0x10) != 0;
            apu->noise.envelope.volume = val & 0x0F;
            break;
        }
        case 0x400E: {
            apu->noise.mode = (val & 0x80) != 0;
            apu->noise.period = g_noise_period_table[val & 0x0F];
            break;
        }
        case 0x400F: {
            apu->noise.length_counter = g_length_table[(val >> 3) & 0x1F];
            apu->noise.envelope.start = 1;
            break;
        }

        /* DMC */
        case 0x4010: {
            apu->dmc.irq = (val & 0x80) != 0;
            apu->dmc.loop = (val & 0x40) != 0;
            apu->dmc.timer_period = g_dmc_period_table[val & 0x0F];
            break;
        }
        case 0x4011: {
            apu->dmc.output = val & 0x7F;
            break;
        }
        case 0x4012: {
            apu->dmc.sample_addr = 0xC000 | (val << 6);
            break;
        }
        case 0x4013: {
            apu->dmc.sample_length = (val << 4) | 1;
            break;
        }

        /* Status */
        case 0x4015: {
            apu->square1.length_counter = (val & 1) ? apu->square1.length_counter : 0;
            apu->square2.length_counter = (val & 2) ? apu->square2.length_counter : 0;
            apu->triangle.length_counter = (val & 4) ? apu->triangle.length_counter : 0;
            apu->noise.length_counter = (val & 8) ? apu->noise.length_counter : 0;

            if (val & 0x10) {
                apu->dmc.enabled = 1;
                if (apu->dmc.bytes_remaining == 0) {
                    apu->dmc.current_addr = apu->dmc.sample_addr;
                    apu->dmc.bytes_remaining = apu->dmc.sample_length;
                }
            } else {
                apu->dmc.enabled = 0;
                apu->dmc.bytes_remaining = 0;
            }
            break;
        }

        /* Frame Counter */
        case 0x4017: {
            apu->frame.mode = (val & 0x80) != 0;
            apu->frame.irq = !(val & 0x40);
            if (apu->frame.mode) {
                /* 5-step mode - immediately step */
                frame_clock(apu, 0);
            }
            break;
        }
    }
}

uint8_t nes_apu_cpu_read(nes_apu_t* apu, uint16_t addr) {
    if (addr == 0x4015) {
        uint8_t status = 0;
        status |= apu->square1.length_counter > 0 ? 1 : 0;
        status |= apu->square2.length_counter > 0 ? 2 : 0;
        status |= apu->triangle.length_counter > 0 ? 4 : 0;
        status |= apu->noise.length_counter > 0 ? 8 : 0;
        status |= apu->dmc.bytes_remaining > 0 ? 0x10 : 0;
        /* Frame IRQ flag */
        return status;
    }
    return 0;
}

float nes_apu_get_output(nes_apu_t* apu) {
    /* Square channel outputs */
    uint8_t sq1 = square_output(&apu->square1);
    uint8_t sq2 = square_output(&apu->square2);
    float sq_out = sq1 ? apu->square1.envelope.volume : 0;
    sq_out += sq2 ? apu->square2.envelope.volume : 0;

    /* TND (Triangle, Noise, DMC) output */
    uint8_t tri = triangle_output(&apu->triangle);
    uint8_t ns = noise_output(&apu->noise);
    uint8_t dmc = apu->dmc.output;

    /* Mix using NES approximation equations */
    float pulse = 95.88 / ((8128.0 / (sq1 + sq2 + 1)) + 100.0);
    float tnd = 159.79 / (1.0 / ((tri / 8227.0) + (ns / 12241.0) + (dmc / 22638.0)) + 100.0);

    return pulse + tnd;
}

int nes_apu_generate_samples(nes_apu_t* apu, float* buffer, int max_samples) {
    /* Generate samples at 44.1kHz */
    /* Each frame has ~29780.5 CPU cycles */
    /* 44.1kHz samples per frame = 735 samples */

    int samples = (APU_CPU_CLOCK_NTSC / APU_SAMPLE_RATE);
    if (samples > max_samples) samples = max_samples;

    float cpu_cycles_per_sample = (float)APU_CPU_CLOCK_NTSC / APU_SAMPLE_RATE;

    for (int i = 0; i < samples; i++) {
        uint32_t cycles_end = (uint32_t)((i + 1) * cpu_cycles_per_sample);
        uint32_t cycles_start = (uint32_t)(i * cpu_cycles_per_sample);

        nes_apu_execute_cycles(apu, cycles_end - cycles_start);

        buffer[i] = nes_apu_get_output(apu) * 2.0f;  /* Amplify */
    }

    return samples;
}

uint8_t nes_apu_square_output(nes_apu_t* apu, int channel) {
    return square_output(channel == 0 ? &apu->square1 : &apu->square2);
}

uint8_t nes_apu_triangle_output(nes_apu_t* apu) {
    return triangle_output(&apu->triangle);
}

uint8_t nes_apu_noise_output(nes_apu_t* apu) {
    return noise_output(&apu->noise);
}

uint8_t nes_apu_dmc_output(nes_apu_t* apu) {
    return apu->dmc.output;
}

void nes_apu_set_bus(nes_apu_t* apu, apu_bus_t* bus) {
    g_apu_bus = bus;
}
