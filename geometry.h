#ifndef INCLUDE__GEOMETRY_H
#define INCLUDE__GEOMETRY_H

#include "common.h"


struct box
{
	s32 x0, y0;
	s32 x1, y1;
};

#define BOX(x0, y0, x1, y1) \
	((struct box){ x0, y0, x1, y1 })
#define BOX_GEOM(x, y, width, height) \
	((struct box){ x, y, x + width, y + height })
#define BOX_SCREEN(width, height) \
	((struct box){ 0, 0, width, height })

#define BOX_WIDTH(box_ptr) \
	ABS((box_ptr)->x1 - (box_ptr)->x0)
#define BOX_HEIGHT(box_ptr) \
	ABS((box_ptr)->y1 - (box_ptr)->y0)

static inline struct box *
box_translate(
	struct box *dst,
	struct box const *src,
	s32 x,
	s32 y
) {
	dst->x0 = src->x0 + x;
	dst->y0 = src->y0 + y;
	dst->x1 = src->x1 + x;
	dst->y1 = src->y1 + y;
	return dst;
}

static inline struct box *
box_copy(
	struct box *dst,
	struct box const *src
) {
	dst->x0 = src->x0;
	dst->y0 = src->y0;
	dst->x1 = src->x1;
	dst->y1 = src->y1;
	return dst;
}

static inline struct box *
box_standardize(
	struct box *dst, 
	struct box const *src
) {
	s32	
		x0 = MIN(src->x0, src->x1),
		y0 = MIN(src->y0, src->y1),
		x1 = MAX(src->x0, src->x1),
		y1 = MAX(src->y0, src->y1);
	dst->x0 = x0;
	dst->y0 = y0;
	dst->x1 = x1;
	dst->y1 = y1;
	return dst;
}

static inline struct box * 
box_intersect_no_standardize(
	struct box *dst, 
	struct box const *srca, 
	struct box const *srcb
) {
	dst->x0 = MAX(srca->x0, srcb->x0),
	dst->y0 = MAX(srca->y0, srcb->y0),
	dst->x1 = MIN(srca->x1, srcb->x1),
	dst->y1 = MIN(srca->y1, srcb->y1);
	return dst;
}

static inline struct box *
box_intersect(
	struct box *dst,
	struct box const *srca,
	struct box const *srcb
) {
	struct box tmpa, tmpb;
	return box_intersect_no_standardize(dst,
		box_standardize(&tmpa, srca),
		box_standardize(&tmpb, srcb)
	);
}

static inline void
box_intersect_with_offset(
	struct box *dsta,
	struct box *dstb,
	struct box const *srca,
	struct box const *srcb,
	s32 bx,
	s32 by
) {
	box_translate(dstb, srcb, bx, by);
	box_intersect(dsta, srca, dstb);
	box_translate(dstb, dsta, -bx, -by);
}

#endif /* INCLUDE__GEOMETRY_H */
