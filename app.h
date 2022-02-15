#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "common.h"


double
app_sleep(double seconds);

double
app_clock();

void
app_panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
