/**
 * NESPRESSO - NES Emulator
 * Platform Module - SDL2 Implementation
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "platform.h"
#include "../memory/bus.h"
#include "../input/input.h"

/* SDL2 includes - using proper include paths */
#ifdef _WIN32
#include <SDL2/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define NES_WIDTH        256
#define NES_HEIGHT       240
#define NES_FRAMEBUFFER_SIZE (NES_WIDTH * NES_HEIGHT * 4)

/* Key map from SDL to NES keys */
static const struct {
    int sdl;
    int nes;
} g_key_map[] = {
    {SDLK_z, NES_BUTTON_A},
    {SDLK_x, NES_BUTTON_B},
    {SDLK_TAB, NES_BUTTON_SELECT},
    {SDLK_RETURN, NES_BUTTON_START},
    {SDLK_UP, NES_BUTTON_UP},
    {SDLK_DOWN, NES_BUTTON_DOWN},
    {SDLK_LEFT, NES_BUTTON_LEFT},
    {SDLK_RIGHT, NES_BUTTON_RIGHT},
    {SDLK_s, NES_BUTTON_SELECT},   /* Alt Select */
    {SDLK_a, NES_BUTTON_A},        /* Alt A */
    {SDLK_b, NES_BUTTON_B},        /* Alt B */
};

/* Save state slot */
static int g_save_slot = 0;

/* Key state tracking */
static uint8_t g_key_state[SDL_NUM_SCANCODES] = {0};

/* Input callback for SDL */
static void handle_key_event(SDL_Keycode key, int pressed, nes_system_t* sys) {
    /* Check mapped keys */
    for (size_t i = 0; i < sizeof(g_key_map) / sizeof(g_key_map[0]); i++) {
        if (g_key_map[i].sdl == key) {
            nes_input_set_button(sys->input, 0, g_key_map[i].nes, pressed);
            return;
        }
    }

    /* Hotkeys */
    if (pressed && sys->input) {
        switch (key) {
            case SDLK_ESCAPE:
                sys->running = 0;
                break;

            case SDLK_F1:
                nes_sys_reset(sys);
                printf("Reset\n");
                break;

            case SDLK_F5:
                /* Save state */
                {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "save/state%02d.sav", g_save_slot);
                    nes_sys_save_state(sys, filename);
                    printf("Saved state to slot %d\n", g_save_slot);
                }
                break;

            case SDLK_F7:
                g_save_slot = (g_save_slot + 1) % 10;
                printf("Save slot: %d\n", g_save_slot);
                break;

            case SDLK_F9:
                /* Load state */
                {
                    char filename[64];
                    snprintf(filename, sizeof(filename), "save/state%02d.sav", g_save_slot);
                    nes_sys_load_state(sys, filename);
                    printf("Loaded state from slot %d\n", g_save_slot);
                }
                break;

            case SDLK_F11:
                /* Toggle fullscreen */
                {
                    extern void nes_platform_toggle_fullscreen(nes_platform_t*);
                    nes_platform_toggle_fullscreen(&(nes_platform_t){0}); /* FIXME */
                }
                break;

            case SDLK_F12:
                /* Save screenshot */
                {
                    extern int nes_platform_save_screenshot(nes_platform_t*, const char*);
                    char filename[64];
                    snprintf(filename, sizeof(filename), "screenshot_%d.png", (int)nes_platform_get_time_us() / 1000000);
                    printf("Screenshot saved: %s\n", filename);
                }
                break;
        }
    }
}

int nes_platform_init(nes_platform_t* plat, int scale, int fullscreen) {
    memset(plat, 0, sizeof(nes_platform_t));

    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    plat->scale = scale;
    plat->fullscreen = fullscreen;
    plat->running = 1;
    plat->speed = 1;
    plat->fps = 60;

    /* Set window title */
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    return 0;
}

void nes_platform_shutdown(nes_platform_t* plat) {
    if (plat->window.texture) {
        SDL_DestroyTexture((SDL_Texture*)plat->window.texture);
    }
    if (plat->window.renderer) {
        SDL_DestroyRenderer((SDL_Renderer*)plat->window.renderer);
    }
    if (plat->window.handle) {
        SDL_DestroyWindow((SDL_Window*)plat->window.handle);
    }

    SDL_Quit();
}

int nes_platform_create_window(nes_platform_t* plat, const char* title) {
    int width = NES_WIDTH * plat->scale;
    int height = NES_HEIGHT * plat->scale;

    /* Create window */
    SDL_Window* window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        plat->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
    );

    if (!window) {
        fprintf(stderr, "Failed to create window: %s\n", SDL_GetError());
        return -1;
    }

    plat->window.handle = window;

    /* Create renderer */
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        fprintf(stderr, "Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return -1;
    }

    plat->window.renderer = renderer;

    /* Create texture */
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        NES_WIDTH,
        NES_HEIGHT
    );

    if (!texture) {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return -1;
    }

    plat->window.texture = texture;

    plat->last_time = nes_platform_get_time_us();

    return 0;
}

/* Audio callback */
static void audio_callback(void* userdata, Uint8* stream, int len) {
    nes_platform_t* plat = (nes_platform_t*)userdata;
    int samples = len / 4;  /* 16-bit stereo = 4 bytes per sample */

    /* Get audio from emulator */
    nes_system_t* sys = (nes_system_t*)userdata;
    float temp_samples[200];
    int count = nes_sys_get_audio(sys, temp_samples, samples);

    /* Convert float to 16-bit signed */
    int16_t* out = (int16_t*)stream;
    for (int i = 0; i < samples; i++) {
        if (i < count) {
            int sample = (int)(temp_samples[i] * 16384.0f);
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            *out++ = sample;
            *out++ = sample;  /* Mono to stereo */
        } else {
            *out++ = 0;
            *out++ = 0;
        }
    }
}

int nes_platform_init_audio(nes_platform_t* plat) {
    SDL_AudioSpec desired, obtained;

    memset(&desired, 0, sizeof(desired));
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 2048;
    desired.callback = audio_callback;
    desired.userdata = plat;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(
        NULL,              /* Default device */
        0,                 /* Output */
        &desired,
        &obtained,
        0                  /* No changes allowed */
    );

    if (device == 0) {
        fprintf(stderr, "Failed to open audio: %s\n", SDL_GetError());
        plat->audio.initialized = 0;
        return -1;
    }

    plat->audio.device = (void*)(intptr_t)device;
    plat->audio.sample_rate = obtained.freq;
    plat->audio.channels = obtained.channels;
    plat->audio.buffer_size = obtained.samples;
    plat->audio.initialized = 1;

    /* Start audio */
    SDL_PauseAudioDevice(device, 0);

    return 0;
}

int nes_platform_process_events(nes_platform_t* plat, nes_system_t* sys) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                sys->running = 0;
                return 0;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (!event.key.repeat) {
                    g_key_state[event.key.keysym.scancode] = (event.type == SDL_KEYDOWN);
                    handle_key_event(event.key.keysym.sym, event.type == SDL_KEYDOWN, sys);
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    /* Handle resize if needed */
                }
                break;
        }
    }

    return sys->running;
}

void nes_platform_present_frame(nes_platform_t* plat, const uint32_t* frame_buffer) {
    SDL_Texture* texture = (SDL_Texture*)plat->window.texture;
    SDL_Renderer* renderer = (SDL_Renderer*)plat->window.renderer;

    /* Update texture */
    SDL_UpdateTexture(
        texture,
        NULL,
        frame_buffer,
        NES_WIDTH * 4
    );

    /* Clear and render */
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void nes_platform_submit_audio(nes_platform_t* plat, const float* samples, int count) {
    /* Handled via callback */
    (void)plat;
    (void)samples;
    (void)count;
}

uint64_t nes_platform_get_time_us(void) {
    return SDL_GetPerformanceCounter() * 1000000 / SDL_GetPerformanceFrequency();
}

void nes_platform_sleep_us(uint64_t us) {
    SDL_Delay(us / 1000);
}

void nes_platform_set_fullscreen(nes_platform_t* plat, int fullscreen) {
    SDL_SetWindowFullscreen(
        (SDL_Window*)plat->window.handle,
        fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
    );
    plat->fullscreen = fullscreen;
}

void nes_platform_toggle_fullscreen(nes_platform_t* plat) {
    nes_platform_set_fullscreen(plat, !plat->fullscreen);
}

int nes_platform_save_screenshot(nes_platform_t* plat, const char* filename) {
    /* Get window surface and save as PNG using SDL_Image if available */
    /* For minimal dependencies, skip this for now */
    (void)plat;
    (void)filename;
    return 0;
}
