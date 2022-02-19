#include <stdio.h>

#include <terminal.h>
#include <draw.h>
#include <app.h>

static char const * const SAMPLE =
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam   "
	"volutpat eros nulla, nec pulvinar nibh finibus sit amet. Ut quis "
	"purus varius, bibendum leo in, tincidunt arcu. Curabitur et      "
	"suscipit est. Nulla varius fringilla justo, sed accumsan magna   "
	"condimentum eu. Sed enim odio, lobortis at dapibus semper,       "
	"maximus ut diam. Donec quis condimentum lorem. Mauris ultrices   "
	"pretium placerat. Duis a dignissim justo, in pulvinar ex.        "

	"Phasellus ut lobortis ex. Donec eleifend turpis leo, at          "
	"consectetur mi vestibulum a. Morbi tristique massa eu lectus     "
	"consectetur, ut cursus velit gravida. Nam congue faucibus erat,  "
	"sodales semper enim luctus eu. Suspendisse non purus eu magna    "
	"ultricies congue in eu ipsum. Aliquam erat volutpat. Ut quis     "
	"purus id ligula ultrices mattis. Nam dolor nibh, volutpat eu     "
	"odio eu, maximus gravida dolor. Etiam aliquam sit amet tortor in "
	"molestie. Aenean eu turpis gravida, luctus nulla in, dignissim   "
	"mauris. Mauris ultrices tellus nec pharetra euismod.             ";

int
demo()
{
	s32 term_w, term_h;
	t_query_size(&term_w, &term_h);

	struct frame frame;
	frame_alloc(&frame, term_w, term_h);
	frame_zero_grid(&frame);

	struct box box;
	frame_typeset_flrr(&frame, &box, 1, 1, 40, 1, SAMPLE);

	struct cell style = {
		.foreground = gray256(0),
		.background = gray256(255),
	};
	frame_stencil_cmp(&frame, CELL_STENCIL_BIT, 1);
	frame_stencil_seteq(&frame, CELL_FOREGROUND_BIT | CELL_BACKGROUND_BIT,
		&style);

	char buffer [128] = {0};
	snprintf(buffer, 128, "box: %d,%d:%d,%d", box.x0, box.y0, box.x1, box.y1);

	frame_typeset_raw(&frame, box.x0, box.y1, 0, buffer);

	t_reset();
	t_clear();
	frame_rasterize(&frame, 0, 0);
	t_flush();

	frame_free(&frame);

	return 0;
}
