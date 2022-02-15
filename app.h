#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "common.h"

#define APP__NANO 1000000000

void
app_setup_and_never_call_again();

void
app_cleanup_and_never_call_again();

double
app_sleep(double seconds);

double
app_clock();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
