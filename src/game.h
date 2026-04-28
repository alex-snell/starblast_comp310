#ifndef GAME_H
#define GAME_H

// Screen dimensions (match your back buffer)
#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768

// Initialize all game state. Call once at startup.
void game_init(void);

// Run one frame of the game: read input, update state, render.
// Call once per iteration of the main loop.
void game_update_and_render(void);


#endif
