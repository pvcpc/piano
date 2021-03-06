#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "geometry.h"
#include "terminal.h"
#include "draw.h"


/* @SECTION(frame) */
struct frame *
frame_realloc(struct frame *frame, s32 width, s32 height)
{
	if (!frame || width < 0 || height < 0) {
		/* @TODO verbose log */
		return NULL;
	}
	
	u32 const requested_size = ALIGN_UP(GRID_SIZEOF(width, height), 16);

	if (requested_size > frame->alloc.grid_alloc_usable_size) {

		struct cell *new_grid;
		if (!(new_grid = realloc(frame->grid, requested_size))) {
			/* @TODO log inconvenience */
			goto e_realloc;
		}

		free(frame->grid);
		frame->grid = new_grid;
		frame->width = width;
		frame->height = height;

		frame->alloc.grid_alloc_base = new_grid;
		frame->alloc.grid_alloc_size = requested_size;
		frame->alloc.grid_alloc_usable_size = requested_size;
	}
	return frame;

e_realloc:
	return NULL;
}

void
frame_free(struct frame *frame)
{
	if (frame) {
		free(frame->grid);
		frame_zero_struct(frame);
	}
}

struct frame *
frame_resize(struct frame *frame, s32 width, s32 height)
{
	if (width < 0 || height < 0) {
		/* @TODO verbose log */
		return NULL;
	}

	/* client-code might not want this alignment, only useful for auto managed frame
	u32 const requested_size = ALIGN_UP(GRID_SIZEOF(width, height), 16);
	*/

	u32 const requested_size = GRID_SIZEOF(width, height);

	if (requested_size > frame->alloc.grid_alloc_usable_size) {
		/* @TODO verbose logging */
		return NULL;
	}

	frame->width = width;
	frame->height = height;

	return frame;
}

u32
frame_load_pattern(struct frame *frame, s32 x, s32 y, char const *pattern)
{
	u32 num_emplaced = 0;
	s32 i = x,
		j = y;

	while (*pattern) {
		switch (*pattern) { /* sure we can cascade, but that's mental overhead */

		case '\n':
			i = x;
			++j;
			break;

		case '\v':
			++j;
			break;

		case '\r':
			i = x;
			break;

		default:
			if ( (0 <= i && i < frame->width) &&
				 (0 <= j && j < frame->height) )
			{
				frame_cell_at(frame, i, j)->content = *pattern;
				++num_emplaced;
			}
			++i;
		}
		++pattern;
	}
	return num_emplaced;
}

/* @SECTION(frame_stencil) */
u32
frame_stencil_cmp(struct frame *frame, u8 mask, s32 reference)
{
	struct box box;
	frame_compute_clip_box(&box, frame);

	struct cell_mask generic_mask;
	cell_mask_from_bits(&generic_mask, mask);

	/* Number of cells that compared NOT ZERO */
	u32 num_nz = 0;

	for (s32 j = box.y0; j < box.y1; ++j) {
		for (s32 i = box.x0; i < box.x1; ++i) {
			struct cell *cell = frame_cell_at(frame, i, j);
			u8 const foreground = generic_mask.foreground & (cell->foreground - reference);
			u8 const background = generic_mask.background & (cell->background - reference);
			s8 const content    = generic_mask.content    & (cell->content - reference);
			s8 const stencil    = generic_mask.stencil    & (cell->stencil - reference);

			cell->stencil = foreground | background | content | stencil;

			num_nz += cell->stencil ? 1 : 0;
		}
	}
	return num_nz;
}

u32
frame_stencil_test(struct frame *frame, u8 mask, u8 reference)
{
	struct box box;
	frame_compute_clip_box(&box, frame);

	struct cell_mask generic_mask;
	cell_mask_from_bits(&generic_mask, mask);

	/* Number of cells that tested NOT ZERO */
	u32 num_nz = 0;

	for (s32 j = box.y0; j < box.y1; ++j) {
		for (s32 i = box.x0; i < box.x1; ++i) {
			struct cell *cell = frame_cell_at(frame, i, j);
			u8 const foreground = generic_mask.foreground & (cell->foreground & reference);
			u8 const background = generic_mask.background & (cell->background & reference);
			s8 const content    = generic_mask.content    & (cell->content & reference);
			s8 const stencil    = generic_mask.stencil    & (cell->stencil & reference);

			cell->stencil = foreground | background | content | stencil;

			num_nz += cell->stencil ? 1 : 0;
		}
	}
	return num_nz;
}

u32
frame_stencil_seteq(struct frame *frame, u8 mask, struct cell const *alternate)
{
	struct box box;
	frame_compute_clip_box(&box, frame);

	struct cell_mask generic_mask;
	cell_mask_from_bits(&generic_mask, mask);

	/* Number of cells modified */
	u32 num_modified = 0;

	for (s32 j = box.y0; j < box.y1; ++j) {
		for (s32 i = box.x0; i < box.x1; ++i) {
			struct cell *cell = frame_cell_at(frame, i, j);
			u8 const stencil_mask = !cell->stencil ? 0xff : 0;

			struct cell_mask specific_mask;
			cell_mask_broadcast_and(&specific_mask, &generic_mask, stencil_mask);
			cell_mask_apply_binary(cell, alternate, cell, &specific_mask);

			num_modified += stencil_mask & mask ? 1 : 0;
		}
	}
	return num_modified;
}

u32
frame_stencil_setne(struct frame *frame, u8 mask, struct cell const *alternate)
{
	struct box box;
	frame_compute_clip_box(&box, frame);

	struct cell_mask generic_mask;
	cell_mask_from_bits(&generic_mask, mask);

	/* Number of cells modified */
	u32 num_modified = 0;

	for (s32 j = box.y0; j < box.y1; ++j) {
		for (s32 i = box.x0; i < box.x1; ++i) {
			struct cell *cell = frame_cell_at(frame, i, j);
			u8 const stencil_mask = cell->stencil ? 0xff : 0;

			struct cell_mask specific_mask;
			cell_mask_broadcast_and(&specific_mask, &generic_mask, stencil_mask);
			cell_mask_apply_binary(cell, alternate, cell, &specific_mask);

			num_modified += stencil_mask & mask ? 1 : 0;
		}
	}
	return num_modified;
}

u32
frame_overlay(struct frame *dst, struct frame *src, s32 x, s32 y, s8 stencil)
{
	struct box dst_box, src_box;
	frame_compute_clip_box(&dst_box, dst);
	frame_compute_clip_box(&src_box, src);

	box_intersect_with_offset(
		&dst_box, &src_box,
		&dst_box, &src_box,
		x, y
	);

	u32 num_copied = 0;

	for (s32 j = 0; j < BOX_HEIGHT(&dst_box); ++j) {
		for (s32 i = 0; i < BOX_WIDTH(&dst_box); ++i) {

			s32 const
				src_x = src_box.x0 + i,
				src_y = src_box.y0 + j;

			s32 const
				dst_x = dst_box.x0 + i,
				dst_y = dst_box.y0 + j;

			struct cell *dst_cell = frame_cell_at(dst, dst_x, dst_y);
			struct cell *src_cell = frame_cell_at(src, src_x, src_y);
			if (!src_cell->content) {
				continue;
			}

			dst_cell->foreground = src_cell->foreground;
			dst_cell->background = src_cell->background;
			dst_cell->content = src_cell->content;
			dst_cell->stencil = stencil;
			++num_copied;
		}
	}

	return num_copied;
}

u32
frame_typeset_raw(struct frame *dst, s32 x, s32 y, s8 stencil, char const *message)
{
	struct box box;
	frame_compute_clip_box(&box, dst);

	if (x >= box.x1 || y >= box.y1) {
		return 0;
	}

	u32 num_written = 0;

	s32 i = x,
		j = y;

	while (*message) {
		switch (*message) { /* sure we can cascade, but that's mental overhead */

		case '\n':
			i = x;
			++j;
			break;

		case '\v':
			++j;
			break;

		case '\r':
			i = x;
			break;

		default:
			if ( (box.x0 <= i && i < box.x1) &&
				 (box.y0 <= j && j < box.y1) )
			{
				struct cell *cell = frame_cell_at(dst, i, j);
				cell->content = *message;
				cell->stencil = stencil;
				++num_written;
			}
			++i;
		}
		++message;
	}
	return num_written;
}

static inline u32
frame__typeset_flrr_line_helper(
	struct frame *dst, 
	struct box const *cbox, 
	s32 x, 
	s32 y, 
	s8 stencil, 
	char *base, 
	char **cursor
) {
	if (cbox->y0 >= cbox->y1) { /* no point */
		return 0;
	}

	u32 num_written = 0;

	char *save_base = base;

	while  (base != *cursor) {
		if ( (cbox->x0 <= x && x < cbox->x1) &&
			 (cbox->y0 <= y && y < cbox->y1) )
		{
			struct cell *cell = frame_cell_at(dst, x, y);
			cell->content = *base++;
			cell->stencil = stencil;
			++num_written;
		}
		++x;
	}
	
	*cursor = save_base;

	return num_written;
}

u32
frame_typeset_flrr(struct frame *dst, struct box *out_bb, s32 x, s32 y, s32 width, s8 stencil, char const *message)
{
	struct box box;
	frame_compute_clip_box(&box, dst);

	if (BOX_WIDTH(&box) <= 1 || 4096 <= BOX_WIDTH(&box)) {
		/* @TODO log error? we need at least two spaces to typeset */
		return 0;
	}

	u32 num_written = 0;
	u32 num_rows = 0;

	s32 buffer_width = width + 1; /* room for the null terminator */

	char scratch [buffer_width];
	char *end    = scratch + buffer_width;
	char *zterm  = scratch + buffer_width - 1;
	char *cursor = scratch;

	char const *base = NULL;

	while (*message) {

		/* plop a space */
		base = message;
		while (*message && !isgraph(*message)) {
			++message;
		}

		if (message > base) {
			if (cursor == zterm) {
				num_written += frame__typeset_flrr_line_helper(
					dst, &box, x, y + num_rows++, stencil, scratch, &cursor
				);
			}
			else {
				*cursor++ = ' ';
			}
		}

		/* plop a word */
		base = message;
		while (isgraph(*message)) {
			++message;
		}

		while ((message - base) > width) {
			if ((end - cursor) <= 1) { /* we need at least 2 spaces */
				num_written += frame__typeset_flrr_line_helper(
					dst, &box, x, y + num_rows++, stencil, scratch, &cursor
				);
			}
			u32 const space = end - cursor - 1;
			cursor = strncpy(cursor, base, space - 1) + space - 1;
			*cursor++ = '-';
			base += space - 1;
		}
		
		if ((end - cursor) < (message - base)) {
			num_written += frame__typeset_flrr_line_helper(
				dst, &box, x, y + num_rows++, stencil, scratch, &cursor
			);
		}

		/* assume for the moment that the word can fit into the buffer.  */
		cursor = strncpy(cursor, base, (message - base)) + (message - base);
	}

	/* flush */
	if (scratch < cursor) {
		num_written += frame__typeset_flrr_line_helper(
			dst, &box, x, y + num_rows++, stencil, scratch, &cursor
		);
	}

	/* if bounding box requested */
	if (out_bb) {
		out_bb->x0 = x;
		out_bb->y0 = y;
		out_bb->x0 = x + width;
		out_bb->x0 = y + num_rows;
	}

	return num_written;
}

u32
frame_rasterize(struct frame *frame, s32 x, s32 y)
{
	s32 term_w, term_h;
	t_query_size(&term_w, &term_h);

	struct box box;
	frame_compute_clip_box(&box, frame);

	struct box dstbox, srcbox;
	box_intersect_with_offset(
		&dstbox, &srcbox,
		&BOX_SCREEN(term_w, term_h),
		&box,
		x, y
	);

	/* How much was actually written out to the terminal in bytes, we're
	 * mostly interested if this remains 0. @TODO maybe make this part 
	 * of the actual terminal interface because keeping track of these 
	 * additions is annoying. */
	u32 throughput = 0;

	/* any constants less than -1 required for init position to
	 * push through initial positions onto the terminal */
	s32 prior_x = INT16_MIN;
	s32 prior_y = INT16_MIN;

	u8 prior_fg = 0;
	u8 prior_bg = 0;
	throughput += t_reset();

	for (s32 j = 0; j < BOX_HEIGHT(&dstbox); ++j) {
		for (s32 i = 0; i < BOX_WIDTH(&dstbox); ++i) {
			s32 const
				src_x = srcbox.x0 + i,
				src_y = srcbox.y0 + j;

			struct cell *cell = frame_cell_at(frame, src_x, src_y);
			if (!cell->content) {
				continue;
			}

			int32_t const
				dst_x = dstbox.x0 + i,
				dst_y = dstbox.y0 + j;

			/* CURSOR POS */
			s32 delta_x = dst_x - prior_x;
			s32 delta_y = dst_y - prior_y;

			/* @SPEED(max): take a look into this a bit more */
			/* @NOTE(max): converting delta_y into \v chars does NOT
			 * improve throughput significantly.
			 */
			/* minimaly optimize byte usage for relocation */
			if (delta_x && delta_y) {
				t_cursor_pos(dst_x + 1, dst_y + 1);
			}
			else if (delta_x > 0) {
				t_cursor_forward(delta_x);
			}
			else if (delta_x < 0) {
				t_cursor_back(-delta_x);
			}
			else if (delta_y > 0) {
				t_cursor_down(delta_y);
			}
			else if (delta_y < 0) {
				t_cursor_up(delta_y);
			}
			prior_x = dst_x + 1; /* to account for cursor advancing right when writing */
			prior_y = dst_y;
			
			/* COLOR */
			if (cell->foreground != prior_fg || 
			    cell->background != prior_bg)
			{
				if (!cell->foreground || cell->background) {
					throughput += t_reset();
					prior_fg = 0;
					prior_bg = 0;
				}
				if (cell->foreground != prior_fg) {
					prior_fg = cell->foreground;
					t_foreground_256(cell->foreground);
				}
				if (cell->background != prior_bg) {
					prior_bg = cell->background;
					t_background_256(cell->background);
				}
			}

			throughput += t_writec(cell->content);
		}
	}
	return throughput;
}
