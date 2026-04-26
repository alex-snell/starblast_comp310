#include "digits.h"

#define W 0xFFFFFF
#define T SPRITE_TRANSPARENT

// --- 0 ---
static const uint32_t digit_0_pixels[] = {
        W, W, W,
        W, T, W,
        W, T, W,
        W, T, W,
        W, W, W
};

const Sprite sprite_digit_0 = {
        .width = 3,
        .height = 5,
        .pixels = digit_0_pixels
};


// --- 1 ---
static const uint32_t digit_1_pixels[] = {
        T, W, T,
        W, W, T,
        T, W, T,
        T, W, T,
        W, W, W
};

const Sprite sprite_digit_1 = {
        .width = 3,
        .height = 5,
        .pixels = digit_1_pixels
};


// --- 2 ---
static const uint32_t digit_2_pixels[] = {
	W, W, W,
	T, T, W,
	T, W, T,
	W, T, T,
	W, W, W
};

const Sprite sprite_digit_2 = {
	.width = 3,
	.height = 5,
	.pixels = digit_2_pixels
};


// --- 3 ---
static const uint32_t digit_3_pixels[] = {
	W, W, W,
	T, T, W,
	T, W, W,
	T, T, W,
	W, W, W
};

const Sprite sprite_digit_3 = {
        .width = 3,
        .height = 5,
        .pixels = digit_3_pixels
};


// --- 4 ---
static const uint32_t digit_4_pixels[] = {
	W, T, W,
	W, T, W,
	W, W, W,
	T, T, W,
	T, T, W
};

const Sprite sprite_digit_4 = {
        .width = 3,
        .height = 5,
        .pixels = digit_4_pixels
};


// --- 5 ---
static const uint32_t digit_5_pixels[] = {
	W, W, W,
	W, T, T,
	W, W, W,
	T, T, W,
	W, W, W
};

const Sprite sprite_digit_5 = {
        .width = 3,
        .height = 5,
        .pixels = digit_5_pixels
};


// --- 6 ---
static const uint32_t digit_6_pixels[] = {
        W, W, W,
        W, T, T,
        W, W, W,
        W, T, W,
        W, W, W
};

const Sprite sprite_digit_6 = {
        .width = 3,
        .height = 5,
        .pixels = digit_6_pixels
};


// --- 7 ---
static const uint32_t digit_7_pixels[] = {
        W, W, W,
        T, T, W,
        T, W, T,
        T, W, T,
        T, W, T
};

const Sprite sprite_digit_7 = {
        .width = 3,
        .height = 5,
        .pixels = digit_7_pixels
};

// --- 8 ---
static const uint32_t digit_8_pixels[] = {
        W, W, W,
        W, T, W,
        W, W, W,
        W, T, W,
        W, W, W
};

const Sprite sprite_digit_8 = {
        .width = 3,
        .height = 5,
        .pixels = digit_8_pixels
};


// --- 9 ---
static const uint32_t digit_9_pixels[] = {
        W, W, W,
        W, T, W,
        W, W, W,
        T, T, W,
        W, W, W
};

const Sprite sprite_digit_9 = {
        .width = 3,
        .height = 5,
        .pixels = digit_9_pixels
};


