#ifndef INCLUDE_T_GRAPHICS_H
#define INCLUDE_T_GRAPHICS_H

#include "t_util.h"


enum t_status
t_graphics_setup();

enum t_status
t_graphics_cleanup();

enum t_status
t_viewport_size_get(
	uint32_t *out_w,
	uint32_t *out_h
);

/* char frames */
struct t_color
{

};

struct t_cell
{
	uint8_t form;
	uint8_t foreground;
	uint8_t background;
	uint8_t _reserved_;
};

struct t_frame
{
	
};

/* frame buffer (two built-in t_frames to coordinate rendering) */
enum t_status
t_frame_buffer_size_to_viewport();

enum t_status
t_frame_buffer_render();

#endif /* INCLUDE_T_GRAPHICS_H */
