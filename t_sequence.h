#ifndef INCLUDE_T_BASE_SEQUENCE_H
#define INCLUDE_T_BASE_SEQUENCE_H

#include "t_util.h"
#include "t_base.h"

#define T_SEQ(Seq)           (Seq)

#define T_CURSOR_SHOW        T_SEQ("\x1b[?25h")
#define T_CURSOR_HIDE        T_SEQ("\x1b[?25l")
#define T_CURSOR_POS         T_SEQ("\x1b[%d;%dH")
#define T_CURSOR_UP          T_SEQ("\x1b[%dA")
#define T_CURSOR_DOWN        T_SEQ("\x1b[%dB")
#define T_CURSOR_FORWARD     T_SEQ("\x1b[%dC")
#define T_CURSOR_BACK        T_SEQ("\x1b[%dD")

static inline enum t_status
t_cursor_show() 
{
	return t_write_f(T_CURSOR_SHOW);
}

static inline enum t_status
t_cursor_hide() 
{
	return t_write_f(T_CURSOR_HIDE);
}

static inline enum t_status
t_cursor_pos(
	int col, /* x */
	int row  /* y */
) {
	return t_write_f(T_CURSOR_POS, row, col);
}

static inline enum t_status
t_cursor_up(
	int amount
) {
	return t_write_f(T_CURSOR_UP, amount);
}

static inline enum t_status
t_cursor_down(
	int amount
) {
	return t_write_f(T_CURSOR_DOWN, amount);
}

static inline enum t_status
t_cursor_forward(
	int amount
) {
	return t_write_f(T_CURSOR_FORWARD, amount);
}

static inline enum t_status
t_cursor_back(
	int amount
) {
	return t_write_f(T_CURSOR_BACK, amount);
}

#define T_CLEAR        T_SEQ("\x1b[2J")
#define T_RESET        T_SEQ("\x1b[0m")

#define T_FG3          T_SEQ("\x1b[%dm")
#define T_BG3          T_SEQ("\x1b[%dm")
#define T_FBG3         T_SEQ("\x1b[%d;%dm")
#define T_FG256        T_SEQ("\x1b[38;5;%dm")
#define T_BG256        T_SEQ("\x1b[48;5;%dm")

/* colors (32-bit packed RGBA, -1 = default/reset/washed color) */
/* @TODO(max): move these to draw, simplify t_fore/backround_256 */
#define T_RGBA(r, g, b, a) (   \
	(((a) & 0xff) << 24) | \
	(((b) & 0xff) << 16) | \
	(((g) & 0xff) <<  8) | \
	(((r) & 0xff)      )   \
)
#define T_RGB(r, g, b) T_RGBA(r, g, b, 255)
#define T_WASHED T_RGBA(0, 0, 0, 0)

#define T_RED(rgba)   (((rgba)      ) & 0xff)
#define T_GREEN(rgba) (((rgba) >>  8) & 0xff)
#define T_BLUE(rgba)  (((rgba) >> 16) & 0xff)
#define T_ALPHA(rgba) (((rgba) >> 24) & 0xff)

#define T_MASK_R 0x000000ff
#define T_MASK_G 0x0000ff00
#define T_MASK_B 0x00ff0000
#define T_MASK_A 0xff000000

enum t_color3
{
	T_BLACK    = 0,
	T_RED,
	T_GREEN,
	T_YELLOW,
	T_BLUE,
	T_MAGENTA,
	T_CYAN,
	T_WHITE,
};

static inline enum t_status
t_clear() 
{
	return t_write_f(T_CLEAR);
}

static inline enum t_status
t_reset() 
{
	return t_write_f(T_RESET);
}

static inline enum t_status
t_foreground_3(
	enum t_color3 color
) {
	return t_write_f(T_FG3, color + 30);
}

static inline enum t_status
t_background_3(
	enum t_color3 color
) {
	return t_write_f(T_BG3, color + 40);
}

static inline enum t_status
t_theme_3(
	enum t_color3 fg_color,
	enum t_color3 bg_color
) {
	return t_write_f(T_FBG3, fg_color + 30, bg_color + 40);
}

static int
t_rgb_compress_256(
	int rgb
) {
	static int const l_point_table [] = {
		0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff,
	};
	int const components [3] = {
		T_RED(rgb), T_GREEN(rgb), T_BLUE(rgb),
	};
	int cube [3];

	/* @SPEED(max): check compiler output for unrolling to make sure */
	for (int i = 0; i < 3; ++i) {
		int close_diff = 255;
		for (int j = 0; j < 6; ++j) {
			int diff = T_ABS(l_point_table[j] - components[i]);
			if (diff < close_diff) {
				cube[i] = j;
				close_diff = diff;
			}
		}
	}

	return 16 +
	       cube[0] * 36 +
	       cube[1] *  6 +
	       cube[2];
}

static inline enum t_status
t_foreground_256(
	int rgb
) {
	int comp = t_rgb_compress_256(rgb);
	return t_write_f(T_FG256, comp);
}
#define t_foreground_256_ex(r, g, b) t_foreground_256(T_RGB(r, g, b))

static inline enum t_status
t_background_256(
	int rgb
) {
	return t_write_f(T_BG256, t_rgb_compress_256(rgb));
}
#define t_background_256_ex(r, g, b) t_background_256(T_RGB(r, g, b))

static inline enum t_status
t_theme_256(
	int fg_rgb,
	int bg_rgb
) {
	enum t_status fg_stat = t_foreground_256(fg_rgb);
	enum t_status bg_stat = t_background_256(bg_rgb);
	return fg_stat >= 0 && bg_stat >= 0 ? T_OK : T_EUNKNOWN;
}

#endif /* INCLUDE_T_BASE_SEQUENCE_H */
