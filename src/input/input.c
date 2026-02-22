/**
 * NESPRESSO - NES Emulator
 * Input Module - Controller Handling Implementation
 *
 * Copyright (c) 2025 NESPRESSO Team
 */

#include "input.h"
#include <string.h>

const keymap_t g_default_keymap = {
    'z',      /* A */
    'x',      /* B */
    '\t',     /* Select (Tab) */
    '\r',     /* Start (Enter) */
    0x52,     /* Up (ARROW_UP) */
    0x51,     /* Down (ARROW_DOWN) */
    0x50,     /* Left (ARROW_LEFT) */
    0x4F,     /* Right (ARROW_RIGHT) */
};

void nes_input_init(nes_input_t* input) {
    memset(input, 0, sizeof(nes_input_t));
}

void nes_input_reset(nes_input_t* input) {
    input->strobe = 0;
    input->read_index[0] = 0;
    input->read_index[1] = 0;
}

void nes_input_set_button(nes_input_t* input, int controller, int button, int pressed) {
    if (controller >= 0 && controller < 2 && button >= 0 && button < NUM_BUTTONS) {
        input->buttons[controller][button] = pressed ? 1 : 0;
    }
}

int nes_input_get_button(const nes_input_t* input, int controller, int button) {
    if (controller >= 0 && controller < 2 && button >= 0 && button < NUM_BUTTONS) {
        return input->buttons[controller][button] != 0;
    }
    return 0;
}

void nes_input_write_strobe(nes_input_t* input, uint8_t value) {
    uint8_t strobe = value & 1;

    if (strobe && !input->strobe) {
        /* Rising edge - reset read index */
        input->read_index[0] = 0;
        input->read_index[1] = 0;
    }

    input->strobe = strobe;
}

uint8_t nes_input_read(nes_input_t* input, int controller) {
    uint8_t result = 0;
    int cid = (controller == 0) ? 0 : 1;

    if (input->strobe) {
        /* Always return button A state when strobing */
        result = input->buttons[cid][NES_BUTTON_A] ? 1 : 0;
    } else {
        if (input->read_index[cid] < NUM_BUTTONS) {
            result = input->buttons[cid][input->read_index[cid]] ? 1 : 0;
            input->read_index[cid]++;
        }
        /* After all buttons are read, return 1 (open bus) */
    }

    return result | 0x40;  /* Bit 6 is always 1 on real NES */
}

void nes_input_update_keyboard(nes_input_t* input, const uint8_t* key_state) {
    /* Map keyboard state to controller buttons */
    const keymap_t* km = &g_default_keymap;

    input->buttons[0][NES_BUTTON_A] = key_state[km->a] ? 1 : 0;
    input->buttons[0][NES_BUTTON_B] = key_state[km->b] ? 1 : 0;
    input->buttons[0][NES_BUTTON_SELECT] = key_state[km->select] ? 1 : 0;
    input->buttons[0][NES_BUTTON_START] = key_state[km->start] ? 1 : 0;
    input->buttons[0][NES_BUTTON_UP] = key_state[km->up] ? 1 : 0;
    input->buttons[0][NES_BUTTON_DOWN] = key_state[km->down] ? 1 : 0;
    input->buttons[0][NES_BUTTON_LEFT] = key_state[km->left] ? 1 : 0;
    input->buttons[0][NES_BUTTON_RIGHT] = key_state[km->right] ? 1 : 0;
}

int nes_input_check_combo(const nes_input_t* input, const uint8_t* combo) {
    /* combo[8] array of button states to check */
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if (input->buttons[0][i] != combo[i]) {
            return 0;
        }
    }
    return 1;
}
