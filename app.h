#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "common.h"
#include "journal.h"
#include "geometry.h"
#include "draw.h"

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
	void(*on_init   )(s32 handle);
	void(*on_destroy)(s32 handle);

	void(*on_appear )(s32 handle);
	void(*on_vanish )(s32 handle);
	void(*on_focus  )(s32 handle);
	void(*on_unfocus)(s32 handle);

	void(*on_input  )(s32 handle, u16 poll_code);
	void(*on_update )(s32 handle, double delta);
	void(*on_render )(s32 handle, struct frame *dst, double delta);
};

s32
app_activity_create(struct activity_callbacks const *cbs, char const *hint_name);

s32
app_activity_set_opaque(s32 handle, void *opaque);

s32
app_activity_get_opaque(s32 handle, void **opaque);

/* @SECTION(misc_services) */
double
app_sleep(double seconds);

double
app_uptime();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
