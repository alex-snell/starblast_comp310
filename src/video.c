#include <stdint.h>
#include "video.h"

// Forward declarations for the skeleton's accessors
extern uint32_t *framebuffer;
uint32_t getFramebufferWidth(void);
uint32_t getFramebufferHeight(void);

// Back buffer in BSS. Start small to be safe.
// At 640x480x4 bytes = 1.2 MB. Adjust if your BSS can't hold it.
#define BACKBUF_WIDTH  1024
#define BACKBUF_HEIGHT 768

static uint32_t backbuffer[BACKBUF_WIDTH * BACKBUF_HEIGHT];

void video_init(void) {
    video_clear(0x000000);
}

void video_clear(uint32_t color) {
    for (uint32_t i = 0; i < BACKBUF_WIDTH * BACKBUF_HEIGHT; i++) {
        backbuffer[i] = color;
    }
}

void video_set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= BACKBUF_WIDTH || y >= BACKBUF_HEIGHT) return;
    backbuffer[y * BACKBUF_WIDTH + x] = color;
}

void video_flip(void) {
    uint32_t fb_w = getFramebufferWidth();
    uint32_t total = BACKBUF_WIDTH * BACKBUF_HEIGHT;
    for(uint32_t i = 0; i < total; i++){
	framebuffer[i] = backbuffer[i];
    }
    (void)fb_w;
}
