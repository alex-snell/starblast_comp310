#include <stdint.h>
#include "game.h"
#include "video.h"
#include "input.h"
#include "sprite.h"
#include "text.h"

//end game screen flash
static int game_over_frames;  // counts up while in game-over state
// --- Levels ---
#define LEVEL_BANNER_FRAMES (60 * 2)
static int waves_spawned_this_level;
static int waves_required;          // recomputed per level
static int level_banner_timer;

// --- Player ---
#define PLAYER_SIZE 50
#define PLAYER_SPEED 4

static int player_x;
static int player_y;

// --- Bullets ---
#define MAX_BULLETS 32
#define BULLET_SPEED 8
#define BULLET_WIDTH 8
#define BULLET_HEIGHT 16
#define MAX_ENEMY_BULLETS 64
#define ENEMY_BULLET_SPEED 5
#define ENEMY_FIRE_RATE_MIN 120
#define ENEMY_FIRE_RATE_MAX 300

// -- Damage/Lives ---
#define PLAYER_MAX_LIVES 4
static int player_damage;
static int player_invuln_frames;
#define INVULN_DURATION 90
#define PLAYER_MAX_DAMAGE 8
#define DAMAGE_RAM 2
#define DAMAGE_BULLET 1

typedef struct
{
    int x;
    int y;
    int active;
} Bullet;

static Bullet bullets[MAX_BULLETS];


// --- Difficulty ramp ---
static int spawn_interval_dynamic;     // replaces SPAWN_INTERVAL usage
static int enemy_fire_min_dynamic;
static int enemy_fire_max_dynamic;

// --- Escape tracking ---
#define MAX_ESCAPED 10
static int enemies_escaped;

//--- ENEMIES ---
#define MAX_ENEMIES 64
#define ENEMY_SIZE  64
#define ENEMY_SPEED 2
#define SPAWN_INTERVAL 60 // frames between spawns (60 = 1 second at 60 FPS)
#define PATTERN_EASY 0
#define PATTERN_MEDIUM 1
#define PATTERN_HARD 2

typedef struct
{
    int x;
    int y;
    int active;
    int fire_cooldown;
} Enemy;

static Enemy enemies[MAX_ENEMIES];
static int spawn_timer;
static uint32_t rng_state;
static Bullet enemy_bullets[MAX_ENEMY_BULLETS];

//--- HUD ---
static int score;
static int level;

static int count_active_enemy(void) {
	int count = 0;
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (enemies[i].active) count++;
	}
	return count;
}

//---Game State---
typedef enum {
	STATE_PLAYING,
	STATE_GAME_OVER,
} GameState;
static GameState game_state;

// --- BACKGROUND / LEVEL ---
#define NUM_STARS 100

typedef struct {
    int x;
    int y;
} Star;

static Star stars[NUM_STARS];

static int bg_offset;
static int scroll_speed;

//Really crude randomizer for enemy spawn
static uint32_t rand_next(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

// --- PLAYER AND BULLETS ---
static int space_was_down;

static void fire_bullet(int x, int y)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
        {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].active = 1;
            return;
        }
    }
}

static void fire_enemy_bullet(int x, int y) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemy_bullets[i].active) {
            enemy_bullets[i].x = x;
            enemy_bullets[i].y = y;
            enemy_bullets[i].active = 1;
            return;
        }
    }
}

static void update_player(void)
{
    if (input_is_key_down(KEY_LEFT))
        player_x -= PLAYER_SPEED;
    if (input_is_key_down(KEY_RIGHT))
        player_x += PLAYER_SPEED;
    if (input_is_key_down(KEY_UP))
        player_y -= PLAYER_SPEED;
    if (input_is_key_down(KEY_DOWN))
        player_y += PLAYER_SPEED;

    if (player_x < 0)
        player_x = 0;
    if (player_x > SCREEN_WIDTH - PLAYER_SIZE)
        player_x = SCREEN_WIDTH - PLAYER_SIZE;

    int min_y = SCREEN_HEIGHT / 2;
    if (player_y < min_y)
        player_y = min_y;

    if (player_y > SCREEN_HEIGHT - PLAYER_SIZE)
        player_y = SCREEN_HEIGHT - PLAYER_SIZE;

    int space_is_down = input_is_key_down(KEY_SPACE);
    if (space_is_down && !space_was_down)
    {
        fire_bullet(player_x + PLAYER_SIZE / 2 - BULLET_WIDTH / 2, player_y);
    }
    space_was_down = space_is_down;

    if (player_invuln_frames > 0) {
	player_invuln_frames--;
    }
}

static void update_bullets(void)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        if (!bullets[i].active)
            continue;
        bullets[i].y -= BULLET_SPEED;
        if (bullets[i].y + BULLET_HEIGHT < 0)
        {
            bullets[i].active = 0;
        }
    }
}

// --- BACKGROUND ---
static void draw_background(void) {
    uint32_t star_color;
    uint32_t bg_color;

    int theme = ((level - 1) / 5) % 3;

    if (theme == 0) {
        star_color = 0xFFFFFF;
        bg_color = 0x000020;
    } else if (theme == 1) {
        star_color = 0x00FF00;
        bg_color = 0x001000;
    } else {
        star_color = 0xFF0000;
        bg_color = 0x200000;
    }

    scroll_speed = 1 + (level / 3);

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

void game_update_and_render(void) {
    if(game_state == STATE_PLAYING){
	update_player();
    	update_bullets();
    	update_enemies();
    	update_enemy_bullets();
    	handle_collisions();
	check_level_up();
        if (level_banner_timer > 0) level_banner_timer--; 
    } else if (game_state == STATE_GAME_OVER) {
        game_over_frames++;
        if (input_is_key_down(KEY_R)) {
            game_init();
            return;
        }
    }

    draw_background();
    draw_player();
    draw_bullets();
    draw_enemy_bullets();
    draw_enemies();
    draw_hud();
    draw_level_banner();

    if (game_state == STATE_GAME_OVER) {
        draw_game_over_overlay();
    }
}
