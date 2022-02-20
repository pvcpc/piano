#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "draw.h"
#include "journal.h"
#include "common.h"

/* @SECTION(logging) */
void
app_log(enum journal_level level, char const *source, char const *restrict format_message, ...);
#define app_log_info_vvv(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF0, __func__, format_message, ##__VA_ARGS__)
#define app_log_info_vv(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF1, __func__, format_message, ##__VA_ARGS__)
#define app_log_info_v(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF2, __func__, format_message, ##__VA_ARGS__)
#define app_log_info(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF3, __func__, format_message, ##__VA_ARGS__)
#define app_log_warn(format_message, ...) \
	app_log(JOURNAL_LEVEL_WARN, __func__, format_message, ##__VA_ARGS__)
#define app_log_error(format_message, ...) \
	app_log(JOURNAL_LEVEL_ERROR, __func__, format_message, ##__VA_ARGS__)
#define app_log_critical(format_message, ...) \
	app_log(JOURNAL_LEVEL_CRITICAL, __func__, format_message, ##__VA_ARGS__)

void
_app_dump_system_journal(s32 fd);

/* @SECTION(activities) */
struct activity_callbacks
{
	bool(*on_init   )(s32 handle);
	bool(*on_destroy)(s32 handle);

	void(*on_appear )(s32 handle);
	void(*on_vanish )(s32 handle);
	void(*on_focus  )(s32 handle);
	void(*on_unfocus)(s32 handle);

	void(*on_input  )(s32 handle, u16 poll_code);
	void(*on_update )(s32 handle, double delta);
	void(*on_render )(s32 handle, struct frame *dst, struct box const *area, double delta);
};

s32
app_activity_init(struct activity_callbacks const *cbs, void *opaque, char const *name);

/* @SECTION(misc_services) */
double
app_sleep(double seconds);

double
app_uptime();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
