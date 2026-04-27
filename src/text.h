#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>

// Draws a string at (x, y) in the given color. Scale multiplies each
// character's pixels (e.g., scale=2 draws each font pixel as a 2x2 block).
// The string must be null-terminated. Only characters A-Z, 0-9, space,
// and a small set of punctuation are supported.
void text_draw(const char *str, int x, int y, uint32_t color, int scale);

// Width and height in pixels that `text_draw` will produce for the given
// string. Useful for centering.
int text_width(const char *str, int scale);
int text_height(int scale);

// Render an integer as text at (x, y).
void text_draw_int(int num, int x, int y, uint32_t color, int scale);

#endif
