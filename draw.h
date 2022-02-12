#ifndef INCLUDE__DRAW_H
#define INCLUDE__DRAW_H

struct rgba
{
	u8 r, g, b, a;
};

struct rgba_mask
{
	u8 r, g, b, a;
};

struct cell
{
	struct rgba fg;
	struct rgba bg;
	s8 content;
	s8 stencil;
};

struct cell_mask
{
	struct rgba_mask fg;
	struct rbga_mask bg;
	u8 content;
	u8 stencil;
};

struct frame
{
	struct cell *grid;
	s32          width;
	s32          height;
};


u32
frame_rasterize(struct frame *frame, s32 x, s32 y);

#endif /* INCLUDE__DRAW_H */
