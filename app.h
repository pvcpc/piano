#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "journal.h"
#include "common.h"

/* @SECTION(services) */
void
app_log(enum journal_level level, char const *restrict format_message, ...);
#define app_log_info_vvv(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF0, format_message, __VA_ARGS__)
#define app_log_info_vv(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF1, format_message, __VA_ARGS__)
#define app_log_info_v(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF2, format_message, __VA_ARGS__)
#define app_log_info(format_message, ...) \
	app_log(JOURNAL_LEVEL_INF3, format_message, __VA_ARGS__)
#define app_log_warn(format_message, ...) \
	app_log(JOURNAL_LEVEL_WARN, format_message, __VA_ARGS__)
#define app_log_error(format_message, ...) \
	app_log(JOURNAL_LEVEL_ERROR, format_message, __VA_ARGS__)
#define app_log_critical(format_message, ...) \
	app_log(JOURNAL_LEVEL_CRITICAL, format_message, __VA_ARGS__)

void
_app_dump_system_journal(s32 fd);

double
app_sleep(double seconds);

double
app_uptime();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
