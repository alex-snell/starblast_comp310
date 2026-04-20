#include <stdint.h>
#include "timer.h"

// The PIT runs at this base frequency.
#define PIT_BASE_HZ 1193182

// I/O ports
#define PIT_CHANNEL2_DATA 0x42
#define PIT_COMMAND       0x43
#define PORT_61           0x61  // PC speaker / PIT channel 2 gate control

// Forward declare inb (defined in kernel_main.c).
// We also need outb, which isn't in kernel_main.c yet — define it here.
uint8_t inb(uint16_t port);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "dN"(port));
}

// Target frame duration in microseconds. 60 FPS = 16666 us/frame.
#define FRAME_US 8333

void timer_init(void) {
    // Nothing to set up globally right now. The delay functions program
    // channel 2 on each call. Later, if we add proper frame accounting,
    // this is where we'd initialize state.
}

// Delay for approximately `microseconds` using PIT channel 2.
//
// Strategy: program channel 2 in one-shot mode with a divisor corresponding
// to the requested delay, enable its gate via port 0x61, then poll bit 5 of
// port 0x61 (the channel 2 output) until it goes high, which indicates the
// count has elapsed.
//
// Max single delay is limited by the 16-bit divisor: 65535 / 1193182 Hz
// ≈ 54925 us ≈ 55 ms. For longer delays we loop.
void timer_delay_us(uint32_t microseconds) {
    while (microseconds > 0) {
        uint32_t chunk_us = microseconds > 50000 ? 50000 : microseconds;
        microseconds -= chunk_us;

        // Compute divisor: ticks = us * PIT_BASE_HZ / 1_000_000
        // Rearranged to avoid overflow: ticks = us * 1193182 / 1000000
        // For chunk_us <= 50000, this is at most ~59659, fits in 16 bits.
        uint32_t ticks = (chunk_us * PIT_BASE_HZ) / 1000000;
        if (ticks == 0) ticks = 1;
        if (ticks > 0xFFFF) ticks = 0xFFFF;

        // Program PIT channel 2: mode 0 (interrupt on terminal count),
        // lobyte/hibyte access, binary mode.
        // Command byte: 10 11 000 0 = 0xB0
        //   bits 7-6: 10 = channel 2
        //   bits 5-4: 11 = access lobyte then hibyte
        //   bits 3-1: 000 = mode 0 (one-shot)
        //   bit 0:    0  = binary
        outb(PIT_COMMAND, 0xB0);

        // Before writing the count, we must enable the channel 2 gate (bit 0
        // of port 0x61) and disable the speaker output (bit 1 of port 0x61,
        // we don't want to hear a click on every frame).
        uint8_t p61 = inb(PORT_61);
        p61 = (p61 & ~0x02) | 0x01;  // gate on, speaker off
        outb(PORT_61, p61);

        // Write the 16-bit count, low byte first.
        outb(PIT_CHANNEL2_DATA, (uint8_t)(ticks & 0xFF));
        outb(PIT_CHANNEL2_DATA, (uint8_t)((ticks >> 8) & 0xFF));

        // Poll bit 5 of port 0x61. In mode 0, the output starts low and goes
        // high when the count reaches zero.
        while ((inb(PORT_61) & 0x20) == 0) {
            // Hint to the CPU we're just spinning. Safe on all x86.
            __asm__ __volatile__("pause");
        }
    }
}

void timer_delay_ms(uint32_t milliseconds) {
    timer_delay_us(milliseconds * 1000);
}

// Simple fixed-rate frame limiter. Call once per frame at end of loop.
// For now this is just "sleep for a full frame duration" — it doesn't
// account for time already spent rendering, so the actual frame rate will
// be slightly lower than 60 Hz. Good enough for now; we'll make it
// measure-and-subtract later.
void timer_wait_frame(void) {
    timer_delay_us(FRAME_US);
}
