#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Scancode constants for keys we care about.
// These are the "make codes" — break codes are make | 0x80.
// Arrow keys are extended (prefixed with 0xE0), so we encode them
// with bit 7 of the index set to disambiguate from single-byte keys.
#define KEY_SPACE     0x39
#define KEY_ESC       0x01
#define KEY_R         0x13
#define KEY_LEFT      0xCB  // E0 4B → we store under 0x4B | 0x80 = 0xCB
#define KEY_RIGHT     0xCD  // E0 4D → 0xCD
#define KEY_UP        0xC8  // E0 48 → 0xC8
#define KEY_DOWN      0xD0  // E0 50 → 0xD0

void input_init(void);

// Call this every frame to drain the keyboard buffer and update state.
void input_poll(void);

// Returns 1 if the given key is currently held, 0 otherwise.
int input_is_key_down(uint8_t key);

#endif
