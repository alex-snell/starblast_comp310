#include "sprite.h"
#include "video.h"






void sprite_draw(const Sprite *sprite, int x, int y) {
    for (uint32_t dy = 0; dy < sprite->height; dy++) {
        for (uint32_t dx = 0; dx < sprite->width; dx++) {
            uint32_t color = sprite->pixels[dy * sprite->width + dx];
            if (color == SPRITE_TRANSPARENT) continue;
            video_set_pixel(x + dx, y + dy, color);
        }
    }
}
