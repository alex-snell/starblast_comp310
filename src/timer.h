#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Initialize the timer subsystem. Call once at startup.
void timer_init(void);

// Block until approximately `microseconds` have elapsed.
void timer_delay_us(uint32_t microseconds);

// Block until approximately `milliseconds` have elapsed.
void timer_delay_ms(uint32_t milliseconds);

// Call this at the end of every frame to cap the frame rate.
// Waits until 1/60th of a second has elapsed since the last call.
void timer_wait_frame(void);

#endif
