/**
 * NESPRESSO - NES Emulator
 * Main Program
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Platform-specific headers */
#ifdef _WIN32
    #include <windows.h>
    #include <SDL2/SDL.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <SDL2/SDL.h>
#endif

/* Local headers */
#include "ppu/ppu.h"
#include "cartridge/rom.h"
#include "memory/bus.h"
#include "platform/platform.h"
#include "input/input.h"

/* Version info */
#define NESPRESSO_VERSION "0.9.0"
#define NESPRESSO_BANNER \
    "\n" \
    "  _______  _____  _____  _____  _____  _     _  _______  \n" \
    " |__   __||  _  |/  ___||  _  |/  __ \\| \\   / |/__   __/ \n" \
    "    | |   | | | |\\ `--. | | | || /  \\/|  \\_/  |   | |   \n" \
    "    | |   | | | | `--. \\| | | || |    |       |   | |   \n" \
    "    | |   | |_| |/\\__/ /\\ \\_/ /| \\__/\\| |     |   | |   \n" \
    "    |_|   \\___/ \\____/  \\___/  \\____/\\|_|     |   | |   \n" \
    "                                                          \n" \
    "          ___    ___    ___    ___                      \n" \
    "         /  /   /  /   /  /   /  /                      \n" \
    "        /  /   /  /   /  /   /  /__                     \n" \
    "       /  /   /  /   /  /   /_____/_                   \n" \
    "      /__/   /__/   /__/   __/ /_/_                     \n" \
    "\n" \
    "NES Emulator v" NESPRESSO_VERSION "\n" \
    "Compiler: " __DATE__ "\n" \
    "Brewing Nostalgia One Frame at a Time!\n"

/* Usage instructions */
#define NESPRESSO_USAGE \
    "Usage: nespresso <rom_file> [options]\n" \
    "\n" \
    "Options:\n" \
    "  -1, -2, -3, -4   Display scale factor (default: 3)\n" \
    "  -f, --fullscreen  Start in fullscreen mode\n" \
    "  --pal             Force PAL timing\n" \
    "  --ntsc            Force NTSC timing (default)\n" \
    "  --no-audio        Disable audio\n" \
    "  -h, --help        Show this help\n" \
    "\n" \
    "Controls:\n" \
    "  Arrow Keys   - D-Pad\n" \
    "  Z            - A Button\n" \
    "  X            - B Button\n" \
    "  Enter        - Start\n" \
    "  Tab          - Select\n" \
    "\n" \
    "Hotkeys:\n" \
    "  F1           - Reset\n" \
    "  F5           - Save State\n" \
    "  F7           - Change Save Slot (0-9)\n" \
    "  F9           - Load State\n" \
    "  F11          - Toggle Fullscreen\n" \
    "  ESC          - Exit\n"

/* Global state */
static nes_system_t g_system;
static nes_platform_t g_platform;

/* Initialize save directory */
static int create_save_directory(void) {
    /* Create save directory if it doesn't exist */
#ifdef _WIN32
    CreateDirectoryA("save", NULL);
#else
    mkdir("save", 0755);
#endif
    return 0;
}

/* Main function */
int main(int argc, char* argv[]) {
    int scale = 3;
    int fullscreen = 0;
    int audio_enabled = 1;
    char* rom_filename = NULL;

    /* Parse command line */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("%s\n%s\n", NESPRESSO_BANNER, NESPRESSO_USAGE);
            return 0;
        } else if (strcmp(argv[i], "-1") == 0) {
            scale = 1;
        } else if (strcmp(argv[i], "-2") == 0) {
            scale = 2;
        } else if (strcmp(argv[i], "-3") == 0) {
            scale = 3;
        } else if (strcmp(argv[i], "-4") == 0) {
            scale = 4;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--fullscreen") == 0) {
            fullscreen = 1;
        } else if (strcmp(argv[i], "--no-audio") == 0) {
            audio_enabled = 0;
        } else if (argv[i][0] != '-') {
            rom_filename = argv[i];
        }
    }

    /* Print banner */
    printf("%s\n", NESPRESSO_BANNER);

    /* Check for ROM file */
    if (!rom_filename) {
        fprintf(stderr, "Error: No ROM file specified\n\n%s\n", NESPRESSO_USAGE);
        return 1;
    }

    /* Initialize platform */
    printf("Initializing platform...\n");
    if (nes_platform_init(&g_platform, scale, fullscreen) != 0) {
        fprintf(stderr, "Failed to initialize platform\n");
        return 1;
    }

    /* Create window */
    char window_title[128];
    snprintf(window_title, sizeof(window_title), "NESPRESSO NES Emulator v%s", NESPRESSO_VERSION);

    if (nes_platform_create_window(&g_platform, window_title) != 0) {
        fprintf(stderr, "Failed to create window\n");
        nes_platform_shutdown(&g_platform);
        return 1;
    }

    /* Initialize audio if enabled */
    if (audio_enabled) {
        printf("Initializing audio...\n");
        if (nes_platform_init_audio(&g_platform) != 0) {
            printf("Warning: Audio initialization failed, continuing without audio\n");
        }
    } else {
        printf("Audio disabled\n");
    }

    /* Initialize system */
    printf("Initializing NES system...\n");
    if (nes_sys_init(&g_system) != 0) {
        fprintf(stderr, "Failed to initialize NES system\n");
        nes_platform_shutdown(&g_platform);
        return 1;
    }

    /* Create save directory */
    create_save_directory();

    /* Load ROM */
    printf("Loading ROM: %s\n", rom_filename);
    if (nes_sys_load_rom(&g_system, rom_filename) != 0) {
        fprintf(stderr, "Failed to load ROM\n");
        nes_sys_free(&g_system);
        nes_platform_shutdown(&g_platform);
        return 1;
    }

    printf("Starting emulation...\n");
    printf("Controls: Arrows=D-Pad, Z=A, X=B, Enter=Start, Tab=Select\n");
    printf("Hotkeys: F1=Reset, F5=Save, F9=Load, F11=Fullscreen, ESC=Exit\n");

    /* Frame buffer for rendering */
    uint32_t frame_buffer[PPU_WIDTH * PPU_HEIGHT];

    /* Set system as audio user data */
    g_platform.audio.device = (void*)&g_system;

    /* Main loop */
    printf("\n--- Running (press ESC to exit) ---\n\n");
    uint64_t frame_count = 0;
    uint64_t perf_start = nes_platform_get_time_us();

    while (g_system.running) {
        /* Process events */
        if (!nes_platform_process_events(&g_platform, &g_system)) {
            break;
        }

        /* Run one frame */
        nes_sys_step_frame(&g_system);

        /* Render frame */
        nes_sys_render_frame(&g_system, frame_buffer);
        nes_platform_present_frame(&g_platform, frame_buffer);

        frame_count++;

        /* Calculate and display FPS every 60 frames */
        if (frame_count % 60 == 0) {
            uint64_t now = nes_platform_get_time_us();
            uint64_t elapsed = now - perf_start;
            if (elapsed > 0) {
                uint32_t fps = (uint32_t)(frame_count * 1000000 / elapsed);
                /* Only print FPS occasionally to avoid spam */
                if (frame_count % 360 == 0) {
                    printf("FPS: %d\n", fps);
                }
            }
        }

        /* Timing - target 60 FPS */
        uint64_t frame_time = (uint64_t)(1000000 / 60);
        uint64_t elapsed = nes_platform_get_time_us() - g_platform.last_time;
        if (elapsed < frame_time) {
            nes_platform_sleep_us(frame_time - elapsed);
        }
        g_platform.last_time = nes_platform_get_time_us();
    }

    /* Shutdown */
    printf("\nShutting down...\n");

    /* Save SRAM if needed */
    if (g_system.cartridge && g_system.cartridge->info.has_battery) {
        printf("Saving SRAM...\n");
        nes_cartridge_save_sram(g_system.cartridge, "save/sram.sav");
    }

    nes_sys_free(&g_system);
    nes_platform_shutdown(&g_platform);

    printf("Goodbye! Thanks for using NESPRESSO!\n");

    return 0;
}
