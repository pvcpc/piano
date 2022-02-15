#ifndef INCLUDE__DRAW_H
#define INCLUDE__DRAW_H

#include <string.h>

#include "geometry.h"


/* @SECTION(color) */
static inline u8
rgb256(s32 r, s32 g, s32 b)
{
	s32 const
		idx_r = MAX(0, ((r + 5) / 40) - 1),
		idx_g = MAX(0, ((g + 5) / 40) - 1),
		idx_b = MAX(0, ((b + 5) / 40) - 1);
	return (36 * idx_r) + (6 * idx_g) + (idx_b) + (16);
}

static inline u8
gray256(s32 scale)
{
	s32 const index = (scale - 0x08) / 10;

	/* because grayscale doesn't end with 0xffffff, we have to do this
	 * stupid magic to wrap the 256 to a 231 which happens to be the
	 * another code for 0xffffff */
	return 232 + (index % 24) - (index / 24);
}

/* @SECTION(cell) */
#define CELL_FOREGROUND_BIT 0x01
#define CELL_BACKGROUND_BIT 0x02
#define CELL_CONTENT_BIT    0x04
#define CELL_STENCIL_BIT    0x08

struct cell
{
	u8 foreground;
	u8 background;
	s8 content;
	s8 stencil;
};
#define CELL_FOREGROUND(foreground_) \
	((struct cell){.foreground = foreground_,})
#define CELL_BACKGROUND(background_) \
	((struct cell){.background = background_,})
#define CELL_CONTENT(content_) \
	((struct cell){.content = content_,})
#define CELL_STENCIL(stencil_) \
	((struct cell){.stencil = stencil_,})

#define LOCAL_GRID(width, height) \
	((struct cell [(width)*(height)]){{0}})
#define GRID_SIZEOF(width, height) \
	(sizeof(struct cell) * (width) * (height))

struct cell_mask
{
	u8 foreground;
	u8 background;
	u8 content;
	u8 stencil;
};

static inline struct cell_mask *
cell_mask_from_bits(struct cell_mask *dst, u8 mask)
{
	dst->foreground = mask & CELL_FOREGROUND_BIT ? 0xff : 0;
	dst->background = mask & CELL_BACKGROUND_BIT ? 0xff : 0;
	dst->content    = mask & CELL_CONTENT_BIT    ? 0xff : 0;
	dst->stencil    = mask & CELL_STENCIL_BIT    ? 0xff : 0;
	return dst;
}

static inline struct cell_mask *
cell_mask_broadcast_and(
	struct cell_mask *dst, 
	struct cell_mask const *src, 
	u8 mask
) {
	dst->foreground = mask & src->foreground;
	dst->background = mask & src->background;
	dst->content    = mask & src->content;
	dst->stencil    = mask & src->stencil;
	return dst;
}

static inline struct cell *
cell_mask_apply_binary(
	struct cell *dst, 
	struct cell const *prefer, 
	struct cell const *defer, 
	struct cell_mask const *mask
) {
	dst->foreground = (mask->foreground & prefer->foreground) | (~mask->foreground & defer->foreground);
	dst->background = (mask->background & prefer->background) | (~mask->background & defer->background);
	dst->content    = (mask->content & prefer->content)       | (~mask->content & defer->content);
	dst->stencil    = (mask->stencil & prefer->stencil)       | (~mask->stencil & defer->stencil);
	return dst;
}

/* @SECTION(frame) */
struct clip
{
	s32 tlx, tly; /* top left (x, y) offsets */
	s32 brx, bry; /* bottom right (x, y) offsets */
};

struct frame
{
	struct cell  *grid;
	s32           width;
	s32           height;

	/* Unless otherwise noted in the documentation, all routines below
	 * adhere to he setting defined by this clip (or any frame context
	 * for that matter.) */
	struct clip   clip;

	/* if used with frame_realloc, client shouldn't touch, otherwise,
	 * this is here for client code to manage their memory and give
	 * hints to library routines about grid allocation (for ex.
	 * `frame_resize`).
	 */
	struct {
		void     *grid_alloc_base;
		u32       grid_alloc_size;
		u32       grid_alloc_usable_size;
	} alloc;
};
#define LOCAL_FRAME(width_, height_) \
	((struct frame) { \
		.grid = LOCAL_GRID(width_, height_), \
		.width = width_, \
		.height = height_, \
		.alloc.grid_alloc_usable_size = GRID_SIZEOF(width_, height_), \
	 })

static inline struct box *
frame_box_with_clip_accounted(struct box *dst, struct frame const *frame)
{
	return box_intersect(dst,
		&BOX_SCREEN(frame->width, frame->height),
		&BOX(
			frame->clip.tlx, frame->clip.tly,
			frame->clip.brx + frame->width,
			frame->clip.bry + frame->height
		)
	);
}

/**
 * Identical to `memset(frame, 0, sizeof(struct frame))` for convenience.
 *
 * @param frame Frame struct to zero-out.
 *
 * @return The frame.
 */
static inline struct frame *
frame_zero_struct(struct frame *frame)
{
	if (frame) {
		memset(frame, 0, sizeof(struct frame));
	}
	return frame;
}

/**
 * Identical to `memset(frame->grid, 0, frame->alloc.grid_alloc_usable_size)`
 * for convenience.
 *
 * @param frame The frame whose grid to zero-out.
 *
 * @return The frame.
 */
static inline struct frame *
frame_zero_grid(struct frame *frame)
{
	if (frame && frame->grid) {
		memset(frame->grid, 0, frame->alloc.grid_alloc_usable_size);
	}
	return frame;
}

/**
 * Convenience function to clear the stencil byte.
 *
 * @param frame The frame whose stencil bytes to clear.
 *
 * @return The frame.
 */
static inline struct frame *
frame_zero_stencil(struct frame *frame)
{
	for (u32 i = 0; i < (frame->width * frame->height); ++i) {
		frame->grid[i].stencil = 0;
	}
	return frame;
}

/**
 * Samples a cell from the frame at the given location.
 *
 * @param frame Frame struct to sample from.
 * @param x Column to sample from.
 * @param y Row to sample from.
 * 
 * @return The desired cell.
 */
static inline struct cell *
frame_cell_at(struct frame *frame, s32 x, s32 y)
{
	return &frame->grid[(y * frame->width) + x];
}

/**
 * If automatic memory management is desired, this will allocate a
 * grid capable of holding a `width * height` cell array (see `struct
 * cell`).
 *
 * @param frame The frame whose grid to reallocate.
 * @param width The desired width (>= 0).
 * @param height The desired height (>= 0).
 *
 * @return The frame, or NULL on failure. This will also log the cause
 * of the failure to the application logger.
 */
struct frame *
frame_realloc(struct frame *frame, s32 width, s32 height);

static inline struct frame *
frame_alloc(struct frame *frame, s32 width, s32 height)
{
	return frame_realloc(frame_zero_struct(frame), width, height);
}

/**
 * Free automatically managed frame.
 *
 * @param frame The frame to free (NULL is OK).
 */
void
frame_free(struct frame *frame);

/**
 * Tests whether the given `width * height` is possible with the current
 * memory allocation of the frame, and if so, sets the frame width/height
 * to the desired dimensions.
 *
 * @param frame The frame whose dimensions to change.
 * @param width The new desired width (>= 0).
 * @param height The new desired height (>= 0).
 *
 * @return The frame, or NULL on failure. This will also log the cause
 * of the failure to the application logger.
 */
struct frame *
frame_resize(struct frame *frame, s32 width, s32 height);

/**
 * Parses the given pattern and emplace it into the given frame at
 * the desired location.
 *
 * This routine DOES NOT adhere to the `frame->clip` setting. To adhere
 * to the clip, use the `frame_typset*` family of functions instead.
 *
 * @param frame The frame where to emplace the pattern.
 * @param x The desired column.
 * @param y The desired row.
 * @param pattern The pattern definition string.
 *
 * @return The number of characters emplaced (excluding control character
 * such as \n, \r, \v, etc.).
 */
u32
frame_load_pattern(struct frame *frame, s32 x, s32 y, char const *pattern);

/* @SECTION(frame_draw) */
/**
 * Analogous to assembly CMP instruction, that is, performs a subtraction
 * between the given mask element and reference to set the stencil value.
 * Multiple mask bits are probablly not desired as the stencil is OR'd
 * with successive masked elements.
 *
 * @param frame The frame on which to perform this comparison.
 * @param mask The mask bits selecting the elements to compare.
 * @param reference The reference value to compare against.
 *
 * @return The number of elements comparing to NOT ZERO.
 */
u32
frame_stencil_cmp(struct frame *frame, u8 mask, s32 reference);

/**
 * Performs a set operation on all cells whose stencil is 0. Eligible
 * cells have their elements replaced by the masked elements in the
 * `alternate` cell.
 *
 * @param frame The frame whose grid to modify.
 * @param mask The mask selecting which elemnets of `alternate` to use.
 * @param alternate The replacement values for elible cells.
 */
u32
frame_stencil_seteq(struct frame *frame, u8 mask, struct cell const *alternate);
#define frame_stencil_setnz(frame, mask, alternate) frame_stencil_seteq(frame, mask, aslternate)

/**
 * Direct cell copy from `src` to `dst` at the specified offset (unless
 * a `src` cell has 0 for content.)
 *
 * @param dst The frame to write to.
 * @param src The frame to source cells from.
 * @param x The desired src column offset.
 * @param y The desired src row offset.
 * @param stencil The desired stencil value for copied cells.
 *
 * @param The number of `dst` cells affected.
 */
u32
frame_overlay(struct frame *dst, struct frame *src, s32 x, s32 y, s8 stencil);


u32
frame_typeset_raw(struct frame *dst, s32 x, s32 y, s8 stencil, char const *message);

/**
 * Rasterizes the given frame to the master terminal at the given 
 * location. The routine will also ensure that if the frame is clipped
 * outside terminal boundaries, it will not overflow rows onto the next
 * terminal row.
 *
 * @param frame The frame to rasterize.
 * @param x The desired column on the master terminal.
 * @param y The desired row on the master terminal.
 */
u32
frame_rasterize(struct frame *frame, s32 x, s32 y);

#endif /* INCLUDE__DRAW_H */
