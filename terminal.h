#ifndef INCLUDE__TERMINAL_H
#define INCLUDE__TERMINAL_H

#include "common.h"


enum t_poll_modifier 
{
	T_SHIFT      = 0x01,
	T_ALT        = 0x02,
	T_CONTROL    = 0x04,
	T_META       = 0x08,
	T_SPECIAL    = 0x10, /* see below */
	T_ERROR      = 0x80, /* see below in `t_poll_special` */
};

enum t_poll_special
{
	T_F0         = 0,
	T_F1,
	T_F2,
	T_F3,
	T_F4,
	T_F5,
	T_F6,
	T_F7,
	T_F8,
	T_F9,
	T_F10,
	T_F11,
	T_F12,

	T_UP         = 65,
	T_DOWN,
	T_RIGHT,
	T_LEFT,

	/* particularly used in combination with `T_ERROR` */
	T_UNKNOWN    = 240,
	T_DISCARD,
};

#define T_POLL_CODE(modifier, value) \
	(((modifier & 0xff) << 8) | (value & 0xff))

bool
t_manager_setup();

void
t_manager_cleanup();

u32
t_poll();

u32
t_flush();

u32
t_write(u8 const *message, u32 size);

u32
t_writef(char const *format, ...);

u32
t_writez(char const *message);


#define T_SEQ(Seq)           (Seq)

#define T_CURSOR_SHOW        T_SEQ("\x1b[?25h")
#define T_CURSOR_HIDE        T_SEQ("\x1b[?25l")
#define T_CURSOR_POS         T_SEQ("\x1b[%u;%uH")
#define T_CURSOR_UP          T_SEQ("\x1b[%uA")
#define T_CURSOR_DOWN        T_SEQ("\x1b[%uB")
#define T_CURSOR_FORWARD     T_SEQ("\x1b[%uC")
#define T_CURSOR_BACK        T_SEQ("\x1b[%uD")

static inline u32
t_cursor_show() 
{
	return t_writef(T_CURSOR_SHOW);
}

static inline u32
t_cursor_hide() 
{
	return t_writef(T_CURSOR_HIDE);
}

static inline u32
t_cursor_pos(
	u32 col, /* x */
	u32 row  /* y */
) {
	return t_writef(T_CURSOR_POS, row, col);
}

static inline u32
t_cursor_up(
	u32 amount
) {
	return t_writef(T_CURSOR_UP, amount);
}

static inline u32
t_cursor_down(
	u32 amount
) {
	return t_writef(T_CURSOR_DOWN, amount);
}

static inline u32
t_cursor_forward(
	u32 amount
) {
	return t_writef(T_CURSOR_FORWARD, amount);
}

static inline u32
t_cursor_back(
	u32 amount
) {
	return t_writef(T_CURSOR_BACK, amount);
}

#define T_CLEAR              T_SEQ("\x1b[2J")
#define T_RESET              T_SEQ("\x1b[0m")

#define T_FOREGROUND_256     T_SEQ("\x1b[38;5;%um")
#define T_BACKGROUND_256     T_SEQ("\x1b[48;5;%um")

static inline u32
t_clear() 
{
	return t_writez(T_CLEAR);
}

static inline u32
t_reset() 
{
	return t_writez(T_RESET);
}

static inline u32
t_foreground_256(
	u8 color_code
) {
	return t_writef(T_FOREGROUND_256, color_code);
}

static inline u32
t_background_256(
	u8 color_code
) {
	return t_writef(T_BACKGROUND_256, color_code);
}

#endif /* INCLUDE__TERMINAL_H */
