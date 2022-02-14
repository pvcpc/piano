#ifndef INCLUDE__DRAW_H
#define INCLUDE__DRAW_H

#include <string.h>


/* @SECTION(color) */
struct rgba
{
	u8 r, g, b, a;
};

struct rgba_mask
{
	u8 r, g, b, a;
};

#define RGBA(pr, pg, pb, pa) \
	((struct rgba){ pr, pg, pb, pa })
#define RGB(pr, pg, pb) \
	RGBA(pr, pg, pb, 255)

#define RGBA_TO_U32(rgba) \
	(((rgba.r & 0xff) << 24) | \
	 ((rgba.g & 0xff) << 16) | \
	 ((rgba.b & 0xff) <<  8) | \
	 ((rgba.a & 0xff)      ))

static inline s32
rgba_compare(struct rgba lhs, struct rgba rhs)
{
	return (s32) (RGBA_TO_U32(lhs) - RGBA_TO_U32(rhs));
}

static inline bool
rgba_not_equal(struct rgba lhs, struct rgba rhs)
{
	return rgba_compare(lhs, rhs);
}

static inline u8
rgb_compress_cube_256(struct rgba color)
{
	static s32 const l_point_table [6] = {
		0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff,
	};
	s32 const components [3] = {
		color.r, color.g, color.b,
	};
	s32 cube [3];

	/* @SPEED(max): check compiler output for unrolling to make sure */
	for (int i = 0; i < 3; ++i) {
		int close_diff = 255;
		for (int j = 0; j < 6; ++j) {
			int diff = ABS(l_point_table[j] - components[i]);
			if (diff < close_diff) {
				cube[i] = j;
				close_diff = diff;
			}
		}
	}
	return (16) + (36 * cube[0]) + (6 * cube[1]) + (cube[2]);
}

static inline u8
rgb_compress_gray_256(struct rgba color)
{
	int const scale = (color.r + color.g + color.b) / 3;
	int const index = (scale - 0x08) / 10;

	/* because grayscale doesn't end with 0xffffff, we have to do this
	 * stupid magic to wrap the 256 to a 231 which happens to be the
	 * another code for 0xffffff */
	return 232 + (index % 24) - (index / 24);
}

static inline u8
rgb_compress_256(struct rgba color)
{
	/* @SPEED(max): branch */
	if (color.r == color.g && color.g == color.b) {
		return rgb_compress_gray_256(color);
	}
	return rgb_compress_cube_256(color);
}

/* @SECTION(cell) */
struct cell
{
	struct rgba fg;
	struct rgba bg;
	char content;
	s8 stencil;
};
#define LOCAL_GRID(width, height) \
	((struct cell [(width)*(height)]){{{0}}})
#define GRID_SIZEOF(width, height) \
	(sizeof(struct cell) * (width) * (height))

struct cell_mask
{
	struct rgba_mask fg;
	struct rgba_mask bg;
	u8 content;
	u8 stencil;
};

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
#define frame_alloc(frame, width, height) frame_realloc(frame, width, height)

/**
 * Free automatically managed frame.
 *
 * @param frame The frame to free (can be NULL).
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
 * @param frame The frame where to emplace the pattern.
 * @param x The desired column.
 * @param y The desired row.
 * @param pattern The pattern definition string.
 *
 * @return The number of characters emplaced (excluding control character
 * such as \n, \r, \v, etc.), or 0 on failure. This will also log
 * the cause of the failure to the application logger.
 */
u32
frame_load_pattern(struct frame *frame, s32 x, s32 y, char const *pattern);

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
