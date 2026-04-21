#include <stdint.h>
#include "game.h"
#include "video.h"
#include "input.h"

// --- Player ---
#define PLAYER_SIZE  50
#define PLAYER_SPEED 4

static int player_x;
static int player_y;

// --- Bullets ---
#define MAX_BULLETS   32
#define BULLET_SPEED  8
#define BULLET_WIDTH  4
#define BULLET_HEIGHT 12

typedef struct {
    int x;
    int y;
    int active;
} Bullet;

static Bullet bullets[MAX_BULLETS];

// --- Input edge detection ---
static int space_was_down;

static void fire_bullet(int x, int y) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].active = 1;
            return;
        }
    }
}

static void update_player(void) {
    if (input_is_key_down(KEY_LEFT))  player_x -= PLAYER_SPEED;
    if (input_is_key_down(KEY_RIGHT)) player_x += PLAYER_SPEED;
    if (input_is_key_down(KEY_UP))    player_y -= PLAYER_SPEED;
    if (input_is_key_down(KEY_DOWN))  player_y += PLAYER_SPEED;

    if (player_x < 0) player_x = 0;
    if (player_x > SCREEN_WIDTH - PLAYER_SIZE) player_x = SCREEN_WIDTH - PLAYER_SIZE;
    if (player_y < 0) player_y = 0;
    if (player_y > SCREEN_HEIGHT - PLAYER_SIZE) player_y = SCREEN_HEIGHT - PLAYER_SIZE;

    int space_is_down = input_is_key_down(KEY_SPACE);
    if (space_is_down && !space_was_down) {
        fire_bullet(player_x + PLAYER_SIZE/2 - BULLET_WIDTH/2, player_y);
    }
    space_was_down = space_is_down;
}

static void update_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y -= BULLET_SPEED;
        if (bullets[i].y + BULLET_HEIGHT < 0) {
            bullets[i].active = 0;
        }
    }
}

static void draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            video_set_pixel(x + dx, y + dy, color);
        }
    }
}

static void draw_player(void) {
    draw_rect(player_x, player_y, PLAYER_SIZE, PLAYER_SIZE, 0xFFFFFF);
}

static void draw_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        draw_rect(bullets[i].x, bullets[i].y, BULLET_WIDTH, BULLET_HEIGHT, 0xFFFF00);
    }
}

void game_init(void) {
    player_x = SCREEN_WIDTH / 2 - PLAYER_SIZE / 2;
    player_y = SCREEN_HEIGHT - PLAYER_SIZE - 40;

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
    }

    space_was_down = 0;
}

void game_update_and_render(void) {
    update_player();
    update_bullets();

    video_clear(0x000020);
    draw_player();
    draw_bullets();
}
