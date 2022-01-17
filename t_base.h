#ifndef INCLUDE_T_BASE_H
#define INCLUDE_T_BASE_H

#include "t_util.h"


void
t_setup();

void
t_cleanup();

double
t_elapsed();

double
t_delta();

enum t_status
t_viewport_size_get(
	uint32_t *out_w,
	uint32_t *out_h
);


#endif /* INCLUDE_T_BASE_H */
