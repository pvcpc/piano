#ifndef INCLUDE__APP_H
#define INCLUDE__APP_H

#include "common.h"

double
seconds_since_genesis();

void
panic_and_die(u32 code, char const *message);

#endif /* INCLUDE__APP_H */
