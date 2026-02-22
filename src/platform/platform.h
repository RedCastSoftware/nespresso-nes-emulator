/**
 * NESPRESSO - NES Emulator
 * Platform Module - Cross-Platform Support
 *
 * Uses SDL2 for cross-platform video, audio, and input
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_PLATFORM_H
#define NESPRESSO_PLATFORM_H

#include <stdint.h>
#include <stddef.h>

/* Forward declarations */
typedef struct nes_system nes_system_t;
typedef struct nes_input nes_input_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Window/Display */
typedef struct nes_window {
    void*       handle;       /* SDL_Window* */
    void*       renderer;     /* SDL_Renderer* */
    void*       texture;      /* SDL_Texture* */
    int         width;
    int         height;
    int         scale;
    int         fullscreen;
} nes_window_t;

/* Audio */
typedef struct nes_audio {
    void*       device;       /* SDL_AudioDeviceID */
    void*       queue;        /* Audio queue */
    int         sample_rate;
    int         channels;
    int         buffer_size;
    int         initialized;
} nes_audio_t;

/* Platform State */
typedef struct nes_platform {
    nes_window_t    window;
    nes_audio_t     audio;

    /* Frame timing */
    uint64_t        frame_time;
    uint64_t        last_time;

    /* State flags */
    int             running;
    int             speed;     /* 1 = normal, 2 = 2x, etc */
    int             muted;
    int             fps;
} nes_platform_t;

/* Display scales */
#define NES_SCALE_SMALL    2
#define NES_SCALE_MEDIUM   3
#define NES_SCALE_LARGE    4

/* API Functions */

/**
 * Initialize platform
 */
int nes_platform_init(nes_platform_t* plat, int scale, int fullscreen);

/**
 * Shutdown platform
 */
void nes_platform_shutdown(nes_platform_t* plat);

/**
 * Create main window
 */
int nes_platform_create_window(nes_platform_t* plat, const char* title);

/**
 * Initialize audio
 */
int nes_platform_init_audio(nes_platform_t* plat);

/**
 * Process events
 * Returns 0 to exit, 1 otherwise
 */
int nes_platform_process_events(nes_platform_t* plat, nes_system_t* sys);

/**
 * Present frame buffer to screen
 */
void nes_platform_present_frame(nes_platform_t* plat, const uint32_t* frame_buffer);

/**
 * Submit audio samples
 */
void nes_platform_submit_audio(nes_platform_t* plat, const float* samples, int count);

/**
 * Get current high-precision time in microseconds
 */
uint64_t nes_platform_get_time_us(void);

/**
 * Sleep for specified microseconds
 */
void nes_platform_sleep_us(uint64_t us);

/**
 * Set fullscreen mode
 */
void nes_platform_set_fullscreen(nes_platform_t* plat, int fullscreen);

/**
 * Toggle fullscreen
 */
void nes_platform_toggle_fullscreen(nes_platform_t* plat);

/**
 * Save screenshot
 */
int nes_platform_save_screenshot(nes_platform_t* plat, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_PLATFORM_H */
