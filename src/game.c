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

//--- ENEMIES ---
#define MAX_ENEMIES     64
#define ENEMY_SIZE      40
#define ENEMY_SPEED     2
#define SPAWN_INTERVAL  60   // frames between spawns (60 = 1 second at 60 FPS)

typedef struct{
    int x;
    int y;
    int active;
} Enemy;

static Enemy enemies[MAX_ENEMIES];
static int spawn_timer;
static uint32_t rng_state;

// --- BACKGROUND / LEVEL ---
#define NUM_STARS 100

typedef struct {
    int x;
    int y;
} Star;

static Star stars[NUM_STARS];

static int bg_offset;
static int scroll_speed;
static int level;
static int score;

//Really crude randomizer for enemy spawn
static uint32_t rand_next(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

// --- PLAYER AND BULLETS ---
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

//ENEMIES

static void spawn_enemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = rand_next() % (SCREEN_WIDTH - ENEMY_SIZE);
            enemies[i].y = -ENEMY_SIZE;  // start just above top of screen
            enemies[i].active = 1;
            return;
        }
    }
}

static void update_enemies(void) {
    // Spawn on a timer
    spawn_timer--;
    if (spawn_timer <= 0) {
        spawn_enemy();
        spawn_timer = SPAWN_INTERVAL;
    }

    // Move each active enemy downward
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        enemies[i].y += ENEMY_SPEED;
        if (enemies[i].y > SCREEN_HEIGHT) {
            enemies[i].active = 0;
        }
    }
}

static void draw_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        draw_rect(enemies[i].x, enemies[i].y, ENEMY_SIZE, ENEMY_SIZE, 0xFF4040);
    }
}


//--- Collisions ---

//collision helper
static int rects_overlap(int ax, int ay, int aw, int ah,
                         int bx, int by, int bw, int bh) {
    if (ax + aw <= bx) return 0;
    if (bx + bw <= ax) return 0;
    if (ay + ah <= by) return 0;
    if (by + bh <= ay) return 0;
    return 1;
}

//collision handler
static void handle_collisions(void) {
    // Bullet vs enemy
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!enemies[e].active) continue;
            if (rects_overlap(bullets[b].x, bullets[b].y, BULLET_WIDTH, BULLET_HEIGHT,
                              enemies[e].x, enemies[e].y, ENEMY_SIZE, ENEMY_SIZE)) {
                bullets[b].active = 0;
                enemies[e].active = 0;
                score += 10;
                break;  // this bullet is consumed, stop checking enemies
            }
        }
    }
}

static void draw_background(void) {
    uint32_t star_color;
    uint32_t bg_color;

    if (level == 1) {
        star_color = 0xFFFFFF;
        bg_color = 0x000020;
        scroll_speed = 1;
    } else if (level == 2) {
        star_color = 0x00FF00;
        bg_color = 0x001000;
        scroll_speed = 2;
    } else {
        star_color = 0xFF0000;
        bg_color = 0x200000;
        scroll_speed = 3;
    }

    video_clear(bg_color);

    bg_offset += scroll_speed;
    if (bg_offset >= SCREEN_HEIGHT) {
        bg_offset = 0;
    }

    for (int i = 0; i < NUM_STARS; i++) {
        int y = (stars[i].y + bg_offset) % SCREEN_HEIGHT;
        video_set_pixel(stars[i].x, y, star_color);
    }
}


//MAIN
void game_init(void) {
    player_x = SCREEN_WIDTH / 2 - PLAYER_SIZE / 2;
    player_y = SCREEN_HEIGHT - PLAYER_SIZE - 40;

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
    }

    space_was_down = 0;
    spawn_timer = SPAWN_INTERVAL;
    rng_state = 0xC0FFEE; //uhhh idk

    bg_offset = 0;
    scroll_speed = 1;
    level = 1;
    score = 0;

    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = (i * 37) % SCREEN_WIDTH;
        stars[i].y = (i * 53) % SCREEN_HEIGHT;
    }
}

void game_update_and_render(void) {
    if (score > 100) level = 2;
    if (score > 300) level = 3;

    update_player();
    update_bullets();
    update_enemies();
    handle_collisions();

    draw_background();

    draw_player();
    draw_bullets();
    draw_enemies();
}
