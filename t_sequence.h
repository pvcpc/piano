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

#define T_CLEAR              T_SEQ("\x1b[2J")
#define T_RESET              T_SEQ("\x1b[0m")

#define T_FOREGROUND_3       T_SEQ("\x1b[%dm")
#define T_BACKGROUND_3       T_SEQ("\x1b[%dm")
#define T_THEME_3            T_SEQ("\x1b[%d;%dm")
#define T_FOREGROUND_256     T_SEQ("\x1b[38;5;%dm")
#define T_BACKGROUND_256     T_SEQ("\x1b[48;5;%dm")

enum t_color_3
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
	enum t_color_3 color
) {
	return t_write_f(T_FOREGROUND_3, color + 30);
}

static inline enum t_status
t_background_3(
	enum t_color_3 color
) {
	return t_write_f(T_BACKGROUND_3, color + 40);
}

static inline enum t_status
t_theme_3(
	enum t_color_3 fg_color,
	enum t_color_3 bg_color
) {
	return t_write_f(T_THEME_3, fg_color + 30, bg_color + 40);
}

static inline enum t_status
t_foreground_256(
	uint8_t color_code
) {
	return t_write_f(T_FOREGROUND_256, color_code);
}

static inline enum t_status
t_background_256(
	uint8_t color_code
) {
	return t_write_f(T_BACKGROUND_256, color_code);
}

static inline enum t_status
t_theme_256(
	uint8_t fg_color_code,
	uint8_t bg_color_code
) {
	enum t_status fg_stat = t_foreground_256(fg_color_code);
	enum t_status bg_stat = t_background_256(bg_color_code);
	return fg_stat >= 0 && bg_stat >= 0 ? T_OK : T_EUNKNOWN;
}

#endif /* INCLUDE_T_BASE_SEQUENCE_H */
