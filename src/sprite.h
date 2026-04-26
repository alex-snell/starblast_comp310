#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>

// Color used to mark transparent pixels in sprite data.
// Any pixel equal to this value will be skipped during blit.
#define SPRITE_TRANSPARENT 0xFF00FF

typedef struct {
    uint32_t width;
    uint32_t height;
    const uint32_t *pixels;  // row-major, width*height entries
} Sprite;

// Draw a sprite to the back buffer. (x, y) is the top-left corner.
// Transparent pixels are skipped.
void sprite_draw(const Sprite *sprite, int x, int y);

extern const Sprite sprite_ship_healthy;
extern const Sprite sprite_ship_damaged;
extern const Sprite sprite_ship_slight;
extern const Sprite sprite_ship_critical;
extern const Sprite sprite_player_bullet;
extern const Sprite sprite_enemy_bullet;
extern const Sprite sprite_enemy_ship;
#endif
