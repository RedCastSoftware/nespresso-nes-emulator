/**
 * NESPRESSO - NES Emulator
 * Input Module - Controller Handling
 *
 * Copyright (c) 2025 NESPRESSO Team
 * Brewing Nostalgia One Frame at a Time!
 */

#ifndef NESPRESSO_INPUT_H
#define NESPRESSO_INPUT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Controller button indices */
#define NES_BUTTON_A      0
#define NES_BUTTON_B      1
#define NES_BUTTON_SELECT 2
#define NES_BUTTON_START  3
#define NES_BUTTON_UP     4
#define NES_BUTTON_DOWN   5
#define NES_BUTTON_LEFT   6
#define NES_BUTTON_RIGHT  7

#define NUM_BUTTONS       8

/* Controller Port Registers */
#define JOY1_READ       0x4016
#define JOY2_READ       0x4017
#define JOY_STROBE      0x4016

/* Input State */
typedef struct nes_input_state {
    uint8_t buttons[2][NUM_BUTTONS];  /* 2 controllers */
    uint8_t strobe;
    uint8_t read_index[2];            /* Button read index */
    uint8_t last_buttons[2][NUM_BUTTONS];  /* Last frame state */
} nes_input_t;

/* Key mapping structure */
typedef struct {
    int a;
    int b;
    int select;
    int start;
    int up;
    int down;
    int left;
    int right;
} keymap_t;

/* Default key mappings */
extern const keymap_t g_default_keymap;

/* API Functions */

/**
 * Initialize input system
 */
void nes_input_init(nes_input_t* input);

/**
 * Reset input state
 */
void nes_input_reset(nes_input_t* input);

/**
 * Set button state for controller (0 = controller 1, 1 = controller 2)
 */
void nes_input_set_button(nes_input_t* input, int controller, int button, int pressed);

/**
 * Get button state
 */
int nes_input_get_button(const nes_input_t* input, int controller, int button);

/**
 * Handle controller strobe write
 */
void nes_input_write_strobe(nes_input_t* input, uint8_t value);

/**
 * Read controller data
 */
uint8_t nes_input_read(nes_input_t* input, int controller);

/**
 * Update input from keyboard state
 */
void nes_input_update_keyboard(nes_input_t* input, const uint8_t* key_state);

/**
 * Check for button combo (like Reset / Power)
 */
int nes_input_check_combo(const nes_input_t* input, const uint8_t* combo);

#ifdef __cplusplus
}
#endif

#endif /* NESPRESSO_INPUT_H */
