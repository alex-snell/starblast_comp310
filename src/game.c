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
static int difficulty_frames;
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

// Really crude randomizer for enemy spawn
static uint32_t rand_next(void)
{
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

    int min_y = SCREEN_HEIGHT / 2;          // top of bottom half
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
// -- DRAWING && HUD ---
static void draw_rect(int x, int y, int w, int h, uint32_t color)
{
    for (int dy = 0; dy < h; dy++)
    {
        for (int dx = 0; dx < w; dx++)
        {
            video_set_pixel(x + dx, y + dy, color);
        }
    }
}

static void draw_hud(void) {
    int scale = 3;
    int line_h = text_height(scale) + 6;
    int label_pad = scale * 6 * 8;   // reserve space for a 8-char label + gap

    int x_label = 10;
    int x_value = 10 + text_width("HEALTH  ", scale);

    int y = 10;

    text_draw("SCORE",   x_label, y, 0xFFFFFF, scale);
    text_draw_int(score, x_value, y, 0xFFFFFF, scale);
    y += line_h;

    text_draw("LEVEL",   x_label, y, 0xFFFF00, scale);
    text_draw_int(level, x_value, y, 0xFFFF00, scale);
    y += line_h;

    text_draw("HEALTH",  x_label, y, 0x40FF40, scale);
    text_draw_int(PLAYER_MAX_DAMAGE - player_damage, x_value, y, 0x40FF40, scale);
    y += line_h;

    text_draw("ESCAPED", x_label, y, 0xFF8040, scale);
    text_draw_int(enemies_escaped, x_value, y, 0xFF8040, scale);

    (void)label_pad; // unused, keeping for clarity
}

static const Sprite *current_player_sprite(void) {
    if (player_damage < 2) return &sprite_ship_healthy;
    if (player_damage < 4) return &sprite_ship_slight;
    if (player_damage < 6) return &sprite_ship_damaged;
    return &sprite_ship_critical;
}
static void draw_player(void) {
    if (player_damage >= PLAYER_MAX_DAMAGE) return;

    // Flash during invulnerability — skip drawing on alternating frames
    if (player_invuln_frames > 0 && (player_invuln_frames & 4)) {
        return;
    }

    sprite_draw(current_player_sprite(), player_x, player_y);
}

static void draw_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        sprite_draw(&sprite_player_bullet, bullets[i].x, bullets[i].y);
    }
}

static void damage_player(int amount) {
    if (player_invuln_frames > 0) return;  // can't be hit while invulnerable
    if (player_damage >= PLAYER_MAX_DAMAGE) return;

    player_damage += amount;
    if (player_damage >= PLAYER_MAX_DAMAGE){ 
	player_damage = PLAYER_MAX_DAMAGE;
	game_state = STATE_GAME_OVER;
        level_banner_timer = 0;
   }else{ 
   	player_invuln_frames = INVULN_DURATION;
   }
   if (enemies_escaped >= MAX_ESCAPED) {
    game_state = STATE_GAME_OVER;
    level_banner_timer = 0;     // <-- new
   }
}

static void draw_level_banner(void) {
    if (level_banner_timer <= 0) return;
    if (game_state == STATE_GAME_OVER) return; 

    char buf[16];
    int idx = 0;
    const char *prefix = "LEVEL ";
    for (const char *p = prefix; *p; p++) buf[idx++] = *p;

    if (level >= 100) buf[idx++] = '0' + (level / 100) % 10;
    if (level >= 10)  buf[idx++] = '0' + (level / 10) % 10;
    buf[idx++] = '0' + (level % 10);
    buf[idx] = '\0';

    int scale = 8;
    int x = SCREEN_WIDTH / 2 - text_width(buf, scale) / 2;
    int y = SCREEN_HEIGHT / 2 - text_height(scale) / 2;

    text_draw(buf, x, y, 0xFFFF00, scale);
}
//Difficulty

static void apply_level_difficulty(void) {
    spawn_interval_dynamic = 90 - (level - 1) * 8;
    if (spawn_interval_dynamic < 15) spawn_interval_dynamic = 15;

    enemy_fire_min_dynamic = 180 - (level - 1) * 12;
    if (enemy_fire_min_dynamic < 40) enemy_fire_min_dynamic = 40;

    enemy_fire_max_dynamic = 300 - (level - 1) * 18;
    if (enemy_fire_max_dynamic < 90) enemy_fire_max_dynamic = 90;

    if (enemy_fire_max_dynamic <= enemy_fire_min_dynamic) {
        enemy_fire_max_dynamic = enemy_fire_min_dynamic + 30;
    }

    // Number of pattern-spawns required to advance.
    // Level 1: 3 waves. Level 2: 4. Level 5: 7. Level 10: 12. Cap at 20.
    waves_required = 2 + (level/5);
    if (waves_required > 20) waves_required = 20;
}

//ENEMIES

// Spawns one enemy at a specific (x, y). Used by patterns.
static void spawn_enemy_at(int x, int y) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].x = x;
            enemies[i].y = y;
            enemies[i].active = 1;
            enemies[i].fire_cooldown = enemy_fire_min_dynamic +
                (rand_next() % (enemy_fire_max_dynamic - enemy_fire_min_dynamic));
            return;
        }
    }
}

// --- Spawn patterns ---

// Single random enemy at the top.
static void pattern_single(void) {
    int x = rand_next() % (SCREEN_WIDTH - ENEMY_SIZE);
    spawn_enemy_at(x, -ENEMY_SIZE);
}

// V formation: 5 enemies, point-down V centered on a random x.
static void pattern_v_formation(void) {
    int center_x = (SCREEN_WIDTH / 2) +
        (rand_next() % (SCREEN_WIDTH / 2)) - SCREEN_WIDTH / 4;
    int spacing = ENEMY_SIZE + 8;

    spawn_enemy_at(center_x - ENEMY_SIZE / 2, -ENEMY_SIZE);
    spawn_enemy_at(center_x - ENEMY_SIZE / 2 - spacing, -ENEMY_SIZE - spacing);
    spawn_enemy_at(center_x - ENEMY_SIZE / 2 + spacing, -ENEMY_SIZE - spacing);
    spawn_enemy_at(center_x - ENEMY_SIZE / 2 - spacing * 2, -ENEMY_SIZE - spacing * 2);
    spawn_enemy_at(center_x - ENEMY_SIZE / 2 + spacing * 2, -ENEMY_SIZE - spacing * 2);
}

// Horizontal wave: 5 enemies evenly spaced across the screen.
static void pattern_horizontal_wave(void) {
    int spacing = SCREEN_WIDTH / 6;
    for (int i = 1; i <= 5; i++) {
        spawn_enemy_at(spacing * i - ENEMY_SIZE / 2, -ENEMY_SIZE);
    }
}

// Cluster: 4 enemies stacked vertically at one column.
static void pattern_cluster(void) {
    int x = rand_next() % (SCREEN_WIDTH - ENEMY_SIZE);
    int spacing = ENEMY_SIZE + 8;
    for (int i = 0; i < 4; i++) {
        spawn_enemy_at(x, -ENEMY_SIZE - i * spacing);
    }
}

// Picks a pattern based on current difficulty and rolls a random selection.
static void choose_and_spawn_pattern(void) {
    int seconds = difficulty_frames / 60;
    int roll = rand_next() % 100;

    if (seconds < 15) {
        // Easy phase: only singles.
        pattern_single();
    } else if (seconds < 45) {
        // Medium phase: mostly singles, occasional V.
        if (roll < 70) pattern_single();
        else pattern_v_formation();
    } else if (seconds < 90) {
        // Harder: introduce horizontal waves.
        if (roll < 50) pattern_single();
        else if (roll < 80) pattern_v_formation();
        else pattern_horizontal_wave();
    } else {
        // Hard phase: all patterns possible, including clusters.
        if (roll < 30) pattern_single();
        else if (roll < 55) pattern_v_formation();
        else if (roll < 80) pattern_horizontal_wave();
        else pattern_cluster();
    }
}

static void update_enemies(void)
{
    spawn_timer--;
    if (spawn_timer <= 0){
	// Only spawn if we haven't yet hit the wave quota
        if (waves_spawned_this_level < waves_required) {
            choose_and_spawn_pattern();
            waves_spawned_this_level++;
        }
        spawn_timer = spawn_interval_dynamic;
    }

    for (int i = 0; i < MAX_ENEMIES; i++){
        if (!enemies[i].active)
            continue;
        enemies[i].y += ENEMY_SPEED;

        // Enemy escaped past the bottom
        if (enemies[i].y > SCREEN_HEIGHT)
        {
            enemies[i].active = 0;
            enemies_escaped++;
            if (enemies_escaped >= MAX_ESCAPED) {
                game_state = STATE_GAME_OVER;
            }
            continue;
        }

        enemies[i].fire_cooldown--;
        if (enemies[i].fire_cooldown <= 0 && enemies[i].y > 0) {
            fire_enemy_bullet(enemies[i].x + ENEMY_SIZE/2 - BULLET_WIDTH/2,
                              enemies[i].y + ENEMY_SIZE);
            enemies[i].fire_cooldown = enemy_fire_min_dynamic +
                (rand_next() % (enemy_fire_max_dynamic - enemy_fire_min_dynamic));
        }
    }
}



static void draw_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        sprite_draw(&sprite_enemy_ship, enemies[i].x, enemies[i].y);
    }
}


static void update_enemy_bullets(void) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemy_bullets[i].active) continue;
        enemy_bullets[i].y += ENEMY_BULLET_SPEED;  // downward
        if (enemy_bullets[i].y > SCREEN_HEIGHT) {
            enemy_bullets[i].active = 0;
        }
    }
}

static void check_level_up(void) {
    // Both conditions must be true: full quota spawned AND screen is clear.
    if (waves_spawned_this_level >= waves_required &&
        count_active_enemy() == 0) {
        level++;
        waves_spawned_this_level = 0;
        level_banner_timer = LEVEL_BANNER_FRAMES;
        apply_level_difficulty();
    }
}

static void draw_enemy_bullets(void) {
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemy_bullets[i].active) continue;
        sprite_draw(&sprite_enemy_bullet, enemy_bullets[i].x, enemy_bullets[i].y);
    }
}
//--- Collisions ---

// collision helper
static int rects_overlap(int ax, int ay, int aw, int ah,
                         int bx, int by, int bw, int bh)
{
    if (ax + aw <= bx)
        return 0;
    if (bx + bw <= ax)
        return 0;
    if (ay + ah <= by)
        return 0;
    if (by + bh <= ay)
        return 0;
    return 1;
}

// collision handler
static void handle_collisions(void)
{
    // Bullet vs enemy
    for (int b = 0; b < MAX_BULLETS; b++)
    {
        if (!bullets[b].active)
            continue;
        for (int e = 0; e < MAX_ENEMIES; e++)
        {
            if (!enemies[e].active)
                continue;

            if (rects_overlap(bullets[b].x, bullets[b].y, BULLET_WIDTH, BULLET_HEIGHT,
                              enemies[e].x, enemies[e].y, ENEMY_SIZE, ENEMY_SIZE))
            {
                bullets[b].active = 0;
                enemies[e].active = 0;
		score += 10 * level;
                break; // this bullet is consumed, stop checking enemies
            }
        }
    }
    // Enemy bullet vs player — half damage
    for (int b = 0; b < MAX_ENEMY_BULLETS; b++) {
        if (!enemy_bullets[b].active) continue;
        if (rects_overlap(enemy_bullets[b].x, enemy_bullets[b].y, BULLET_WIDTH, BULLET_HEIGHT,
                          player_x, player_y, PLAYER_SIZE, PLAYER_SIZE)) {
            damage_player(DAMAGE_BULLET);
            enemy_bullets[b].active = 0;
        }
    }

    // Ram: player vs enemy — full damage, enemy destroyed
    for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!enemies[e].active) continue;
        if (rects_overlap(player_x, player_y, PLAYER_SIZE, PLAYER_SIZE,
                          enemies[e].x, enemies[e].y, ENEMY_SIZE, ENEMY_SIZE)) {
            damage_player(DAMAGE_RAM);
            enemies[e].active = 0;  // ram destroys the enemy
        }
    }
}

// MAIN
void game_init(void)
{
    player_x = SCREEN_WIDTH / 2 - PLAYER_SIZE / 2;
    player_y = (SCREEN_HEIGHT * 3 / 4) - PLAYER_SIZE / 2;

    for (int i = 0; i < MAX_BULLETS; i++)
    {
        bullets[i].active = 0;
    }

    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for(int i = 0; i < MAX_ENEMY_BULLETS; i++) enemy_bullets[i].active = 0;
    for(int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
    space_was_down = 0;
    spawn_timer = SPAWN_INTERVAL;
    rng_state = 0xC0FFEE; //uhhh idk
    player_damage = 0;
    player_invuln_frames = 0;
    game_state = STATE_PLAYING;
    game_over_frames = 0;

    spawn_timer = spawn_interval_dynamic;
    level = 1;
    waves_spawned_this_level = 0;
    level_banner_timer = LEVEL_BANNER_FRAMES;
    score = 0;
    apply_level_difficulty();
    spawn_timer = spawn_interval_dynamic;
}

static void draw_game_over_overlay(void) {
    const char *line1 = "GAME OVER";
    const char *line2 = "PRESS R TO RESTART";

    int scale1 = 8;  // big "GAME OVER"
    int scale2 = 3;  // smaller "PRESS R..."

    int x1 = SCREEN_WIDTH / 2 - text_width(line1, scale1) / 2;
    int y1 = SCREEN_HEIGHT / 2 - text_height(scale1) / 2 - 40;

    text_draw(line1, x1, y1, 0xFF0000, scale1);

    // Flash the "PRESS R" text every ~30 frames (half-second at 60 FPS)
    if ((game_over_frames / 30) & 1) {
        int x2 = SCREEN_WIDTH / 2 - text_width(line2, scale2) / 2;
        int y2 = y1 + text_height(scale1) + 40;
        text_draw(line2, x2, y2, 0xFFFFFF, scale2);
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
    video_clear(0x000020);
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
