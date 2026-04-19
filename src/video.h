#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>

void video_init(void);
void video_clear(uint32_t color);
void video_set_pixel(uint32_t x, uint32_t y, uint32_t color);
void video_flip(void);

#endif
