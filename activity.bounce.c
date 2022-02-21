#include <stdlib.h>
#include <math.h>

#include "app.h"


struct bounce__opaque
{
	double x; 
	double y;

	double vx;
	double vy;

	u8 background;
};

static void
bounce__peasant_stub(s32 handle) { /* j'aime faire rien */ }

static void
bounce__on_init(s32 handle)
{
	struct bounce__opaque *opaque = malloc(sizeof(*opaque));
	if (!opaque) {
		/* @TODO app activities should return bools on init/destroy. */
		// return false;
		return;
	}

	opaque->x = 0.5;
	opaque->y = 0.5;

	opaque->vx = cos(M_PI * rand() / 180.0);
	opaque->vy = sin(M_PI * rand() / 180.0);

	opaque->background = rand() & 0xff;

	app_activity_set_opaque(handle, opaque);
	
	app_log_info("Bounce activity (#%d) created.", handle);

	// return true;
}

static void
bounce__on_destroy(s32 handle)
{
	app_log_info("Bounce activity (#%d) destroyed.", handle);
}

static void
bounce__on_input(s32 handle, u16 poll_code)
{
	app_log_info_vvv("Bounce activity (#%d) received poll code '%04x'.", handle, poll_code);
}

static void
bounce__on_update(s32 handle, double delta)
{
	app_log_info_vvv("Bounce activity (#%d) received update after %.2f seconds.", handle);

	struct bounce__opaque *opaque = NULL;
	app_activity_get_opaque(handle, (void **) &opaque);
	
	double nx = opaque->x + opaque->vx * delta;
	double ny = opaque->y + opaque->vy * delta;

	/* stupid simple, I know */
	if (nx > 1) {
		opaque->x = 1;
		opaque->vx = -opaque->vx;
	}
	else if (nx < 0) {
		opaque->x = 0;
		opaque->vx = -opaque->vx;
	}
	else {
		opaque->x = nx;
	}


	if (ny > 1) {
		opaque->y = 1;
		opaque->vy = -opaque->vy;
	}
	else if (ny < 0) {
		opaque->y = 0;
		opaque->vy = -opaque->vy;
	}
	else {
		opaque->y = ny;
	}
}

static void
bounce__on_render(s32 handle, struct frame *dst, double delta)
{
	struct box box;
	frame_compute_clip_box(&box, dst);

	struct cell clear = {
		.foreground = 0,
		.background = 0,
		.content = ' ',
		.stencil = 0,
	};
	frame_stencil_test(dst, 0, 0);
	frame_stencil_setz(dst, ~(0), &clear);

	struct bounce__opaque *opaque = NULL;
	app_activity_get_opaque(handle, (void **) &opaque);

	s32 const x = box.x0 + (BOX_WIDTH(&box)-1) * opaque->x;
	s32 const y = box.y0 + (BOX_HEIGHT(&box)-1) * opaque->y;

	frame_cell_at(dst, x, y)->background = gray256(255);
}

void
bounce_create_activity()
{
	static struct activity_callbacks const l_bounce_activity_callbacks = {
		.on_init    = bounce__on_init,
		.on_destroy = bounce__on_destroy,

		.on_appear  = bounce__peasant_stub,
		.on_vanish  = bounce__peasant_stub,
		.on_focus   = bounce__peasant_stub,
		.on_unfocus = bounce__peasant_stub,

		.on_input   = bounce__on_input,
		.on_update  = bounce__on_update,
		.on_render  = bounce__on_render,
	};

	if (app_activity_create(&l_bounce_activity_callbacks, "bounce") < 0) {
		app_log_error("Failed to create bounce activity.");
	}
}
