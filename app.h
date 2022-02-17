#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "common.h"

/* @SECTION(services) */
double
app_sleep(double seconds);

double
app_uptime();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
