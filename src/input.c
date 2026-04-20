#include <stdint.h>
#include "input.h"

// From kernel_main.c
uint8_t inb(uint16_t port);

#define KBD_STATUS_PORT  0x64
#define KBD_DATA_PORT    0x60
#define KBD_STATUS_READY 0x01

// Key state table. Indexed by scancode (0-255). 1 = down, 0 = up.
// For extended keys (arrow keys, etc.) we use scancode | 0x80 as the index.
static uint8_t key_state[256];

// Tracks whether the last byte we read was the 0xE0 extended prefix.
static uint8_t extended_pending;

void input_init(void) {
    for (int i = 0; i < 256; i++) {
        key_state[i] = 0;
    }
    extended_pending = 0;

    // Drain any pending scancodes left in the keyboard buffer from boot.
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_READY) {
        (void)inb(KBD_DATA_PORT);
    }
}

void input_poll(void) {
    // Read all pending scancodes. There may be zero, one, or several.
    while (inb(KBD_STATUS_PORT) & KBD_STATUS_READY) {
        uint8_t code = inb(KBD_DATA_PORT);

        if (code == 0xE0) {
            // This is the extended prefix. The actual key is in the next byte.
            extended_pending = 1;
            continue;
        }

        // Determine if this is a key-up (break) or key-down (make) event.
        // Bit 7 set = break.
        uint8_t is_break = code & 0x80;
        uint8_t scancode = code & 0x7F;

        // Build the key index. For extended keys, set bit 7 so they don't
        // collide with regular keys that happen to share the low 7 bits.
        uint8_t key_index = scancode;
        if (extended_pending) {
            key_index |= 0x80;
            extended_pending = 0;
        }

        key_state[key_index] = is_break ? 0 : 1;
    }
}

int input_is_key_down(uint8_t key) {
    return key_state[key];
}
