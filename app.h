#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "common.h"

/* @SECTION(services) */
void
app_log_info(char const *restrict format_message, ...);

void
app_log_warn(char const *restrict format_message, ...);

void
app_log_error(char const *restrict format_message, ...);

bool
_app_dump_system_journal(s32 fd);

double
app_sleep(double seconds);

double
app_uptime();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
