#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <time.h>

#include "common.h"
#include "journal.h"
#include "terminal.h"
#include "draw.h"
#include "app.h"


/* @SECTION(roll) */
#define ROLL_NOTE_COUNT 12

enum note 
{
	NOTE_INVALID = - 1,
	NOTE_C       =   0,
	NOTE_Cs      =   1,
	NOTE_Db      =   1,
	NOTE_D       =   2,
	NOTE_Ds      =   3,
	NOTE_Eb      =   3,
	NOTE_E       =   4,
	NOTE_Es      =   5,
	NOTE_Fb      =   4,
	NOTE_F       =   5,
	NOTE_Fs      =   6,
	NOTE_Gb      =   6,
	NOTE_G       =   7,
	NOTE_Gs      =   8,
	NOTE_Ab      =   8,
	NOTE_A       =   9,
	NOTE_As      =  10,
	NOTE_Bb      =  10,
	NOTE_B       =  11,
};

/* @GLOBAL */
static char const * const ROLL_KBD_PATTERN = 
	            "+-------------+\n"
	            "||b|b|||b|b|b||\n"
	            "||b|b|||b|b|b||\n"
	            "|+-+-+|+-+-+-+|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "+-+-+-+-+-+-+-+\n";

/* lane counts are dependent on the keyboard layout. */
#define ROLL_KBD_WIDTH  15
#define ROLL_KBD_LANES  (ROLL_KBD_WIDTH-1)
#define ROLL_KBD_KERN   (ROLL_KBD_WIDTH-1)
#define ROLL_KBD_HEIGHT 7

static char const *const ROLL_KBD_OVERLAY [ROLL_NOTE_COUNT] = {
	[NOTE_C ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            " W             \n"
	            " W             \n"
	            "               \n",

	[NOTE_Cs] = "               \n"
	            "  B            \n"
	            "  B            \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_D ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "   W           \n"
	            "   W           \n"
	            "               \n",

	[NOTE_Ds] = "               \n"
	            "    B          \n"
	            "    B          \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_E ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "     W         \n"
	            "     W         \n"
	            "               \n",

	[NOTE_F ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "       W       \n"
	            "       W       \n"
	            "               \n",

	[NOTE_Fs] = "               \n"
	            "        B      \n"
	            "        B      \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_G ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "         W     \n"
	            "         W     \n"
	            "               \n",

	[NOTE_Gs] = "               \n"
	            "          B    \n"
	            "          B    \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_A ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "           W   \n"
	            "           W   \n"
	            "               \n",

	[NOTE_As] = "               \n"
	            "            B  \n"
	            "            B  \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_B ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "             W \n"
	            "             W \n"
	            "               \n",
};

static char g_bug_string [4096];

static s32 g_roll_base_lane;
static s32 g_roll_base_atom;

static s32 g_roll_cursor_index = 5;
static s32 g_roll_cursor_atom;

static inline void
app__roll_index_to_lane(s32 *out_lane, s32 index)
{
	static s32 const l_offset_lane_table[ROLL_NOTE_COUNT] = {
		1, 2, 3, 4, 5, /* C-E */
		7, 8, 9, 10, 11, 12, 13, /* F-B */
	};

	s32 quot;
	s32 rem = qdiv(&quot, index, ROLL_NOTE_COUNT);

	if(out_lane) *out_lane = ROLL_KBD_LANES * (quot + 1) + l_offset_lane_table[rem];
}

static inline bool
app__roll_lane_to_index(s32 *out_index, s32 lane)
{
	static s32 const l_offset_index_table[ROLL_KBD_LANES] = {
		-1, 0, 1, 2, 3, 4,
		-1, 5, 6, 7, 8, 9, 10, 11,
	};

	s32 quot;
	s32 rem = qdiv(&quot, lane, ROLL_KBD_LANES);

	if (rem < 0) return false;

	if (out_index) *out_index = ROLL_NOTE_COUNT * (quot - 1) + l_offset_index_table[rem];

	return true;
}

static inline void
app__roll_lane_decompose(s32 *out_block, s32 *out_offset, enum note *out_note, s32 lane)
{
	static enum note const l_offset_note_table[ROLL_KBD_LANES] = {
		NOTE_INVALID,
		NOTE_C,
		NOTE_Cs,
		NOTE_D,
		NOTE_Ds,
		NOTE_E,
		NOTE_INVALID,
		NOTE_F,
		NOTE_Fs,
		NOTE_G,
		NOTE_Gs,
		NOTE_A,
		NOTE_As,
		NOTE_B,
	};

	s32 quot;
	s32 rem = qdiv(&quot, lane, ROLL_KBD_LANES);

	if (out_block)  *out_block  = quot;
	if (out_offset) *out_offset = rem;
	if (out_note)   *out_note   = l_offset_note_table [rem];
}

static inline struct box *
app__roll_compute_roll_box(struct box *dst, struct box const *view)
{
	dst->x0 = view->x0;
	dst->y0 = view->y0;
	dst->x1 = view->x1;
	dst->y1 = view->y1 - ROLL_KBD_HEIGHT;
	return box_origin_clamp(dst, dst);
}

static inline struct box *
app__roll_compute_keyboard_box(struct box *dst, struct box const *view)
{
	dst->x0 = view->x0;
	dst->y0 = BOX_HEIGHT(view) - MIN(BOX_HEIGHT(view), ROLL_KBD_HEIGHT);
	dst->x1 = view->x1;
	dst->y1 = view->y1;
	return dst;
}

static void
app__roll_draw(struct frame *frame)
{
	struct box view_box;
	frame_compute_clip_box(&view_box, frame);

	struct box roll_box;
	app__roll_compute_roll_box(&roll_box, &view_box);

	struct box kbd_box;
	app__roll_compute_keyboard_box(&kbd_box, &view_box);

	s32 block, offset;
	enum note note;
	app__roll_lane_decompose(&block, &offset, &note, g_roll_base_lane);

	/* Draw keyboard */
	{
		s32 
			x = kbd_box.x0 - offset,
			y = kbd_box.y0;

		while (x < kbd_box.x1) {
			frame_typeset_raw(frame, x, y, 0, ROLL_KBD_PATTERN);
			x += ROLL_KBD_KERN;
		}
	}

	/* Draw lanes */
	{
		for (s32 i = 0; i < BOX_WIDTH(&roll_box); ++i) {
			s32 const x = roll_box.x0 + i;
			s32 const lane = g_roll_base_lane + i;

			enum note note;
			app__roll_lane_decompose(NULL, NULL, &note, lane);
			if (note == NOTE_INVALID) {
				continue;
			}

			for (s32 y = roll_box.y0; y < roll_box.y1; ++y) {
				frame_cell_at(frame, x, y)->content = '|';
			}
		}
	}
}

static void
app__roll_position_cursor(struct frame *frame)
{
	struct box view_box, roll_box;
	app__roll_compute_roll_box(&roll_box, frame_compute_clip_box(&view_box, frame));

	/* ADJUST for cursor index */
	s32 lane;
	app__roll_index_to_lane(&lane, g_roll_cursor_index);

	if (lane < g_roll_base_lane) { /* if the user's cursor is out to the left of the roll */
		g_roll_base_lane = lane;
	}
	else if (lane >= g_roll_base_lane + BOX_WIDTH(&roll_box)) {
		g_roll_base_lane = lane - BOX_WIDTH(&roll_box) + 1;
	}

	/* ADJUST for cursor atom */
	s32 atom = g_roll_cursor_atom;

	if (atom < g_roll_base_atom) {
		g_roll_base_atom = atom;
	}
	else if (atom >= g_roll_base_atom + BOX_HEIGHT(&roll_box)) {
		g_roll_base_atom = atom - BOX_HEIGHT(&roll_box) + 1;
	}

	/* position */
	s32 const
		x = roll_box.x0 + (lane - g_roll_base_lane),
		y = roll_box.y1 - (atom - g_roll_base_atom) - 1;

	t_cursor_pos(x+1, y+1);
}

/* @SECTION(services) */
#define APP__NANO 1000000000

/* @GLOBAL */
static struct timespec g_genesis;

static struct journal  g_journal_sys;

static void
app__init_services()
{
	if (!t_manager_setup()) {
		app_panic_and_die(1, "Not a TTY!");
	}

	if (clock_gettime(CLOCK_MONOTONIC, &g_genesis) < 0) {
		app_panic_and_die(1, "Check your clock captain!");
	}

	// g_journal_sys.memory_overhead_threshold = KILO(64);
}

static void
app__destroy_services()
{
	t_manager_cleanup();
}

void
app_log(enum journal_level level, char const *restrict format_message, ...)
{
	/* get required content buffer size */
	va_list ap;
	va_start(ap, format_message);

	s32 n = vsnprintf(NULL, 0, format_message, ap);
	if (n < 0) {
		return; /* @TODO logging: invalid format_message */
	}
	++n; /* make room for 0 terminator. */

	/* All good, allocate the record and print into it again. */
	struct journal_record *record = journal_create_record(&g_journal_sys, n);
	if (!record) {
		return; /* @TODO logging: cannot create record */
	}
	va_start(ap, format_message);
	vsnprintf(record->content, n, format_message, ap);

	record->time = app_uptime();
	record->level = level;
}

void
_app_dump_system_journal(s32 fd)
{
	struct journal_record *now = g_journal_sys.head;
	while (now) {
		dprintf(fd, "[serial:%6d][cs:%4d][time:%9.4f][level:%s] %s\n", 
			now->serial, now->content_size, now->time, journal_level_string(now->level), now->content
		);
		now = now->next;
	}
}

double
app_sleep(double seconds)
{
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	u64 nanoseconds = (u64) (seconds * 1e9);
	struct timespec sleep = {
		.tv_sec  = nanoseconds / APP__NANO,
		.tv_nsec = nanoseconds % APP__NANO,
	};
	clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);

	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);

	return ( 
		(end.tv_sec - start.tv_sec) +
		(end.tv_nsec - start.tv_nsec) * 1e-9
	);
}

double
app_uptime()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	return   (now.tv_sec - g_genesis.tv_sec)
		   + (now.tv_nsec - g_genesis.tv_nsec) * 1e-9;
}

void
app_panic_and_die(u32 code, char const *message) 
{
	fputs(message, stderr);
	exit(code);
}


/* @SECTION(app) */
#define APP__DRAW_DELTA 8e-3

/* @GLOBAL  */
static bool            g_should_run       = true;
static double          g_time_draw_last;


#ifndef APP_DEMO
int
main(void)
{
	/* 
	 * Setup
	 */
	app__init_services();
	
	g_time_draw_last = app_uptime();

	struct frame frame;
	frame_alloc(&frame, 0, 0);

	/*
	 * Do cool stuff
	 */
	while (g_should_run) {

		double const time_now = app_uptime();
		double const time_delta_draw = time_now - g_time_draw_last;

		if (time_delta_draw >= APP__DRAW_DELTA) {
			g_time_draw_last = time_now;

			s32 term_w, term_h;
			t_query_size(&term_w, &term_h);

			struct clip old_clip;
			frame_realloc(&frame, term_w, term_h);
			frame_zero_grid(&frame);

			/* Draw roll: Allow 2 rows below the roll to render the 
			 * minibuffer and status information (we'll see how that 
			 * goes.)
			 */
			struct clip roll_clip;
			old_clip = frame_clip_inset(&frame, 0, 0, 0, 2);
			{
				roll_clip = frame.clip;
				app__roll_draw(&frame);
			}
			frame.clip = old_clip;
			
			/* Draw minibuffer/status line */
			old_clip = frame_clip_inset(&frame, 0, frame.height - 2, 0, 0);
			{
#if 0
				char status [64] = {0};
				s32 lane;
				app__roll_index_to_lane(&lane, g_roll_cursor_index);
				snprintf(status, 64, "%d:%d (%d)", 
					g_roll_cursor_index, g_roll_cursor_atom, lane
				);
#endif
				frame_typeset_raw(&frame, 0, frame.height - 2, 0, g_bug_string);
				frame_typeset_raw(&frame, 0, frame.height - 1, 0, ":Here is a minibuffer!");
			}
			frame.clip = old_clip;

			/* Make things gray to ease the eyes  */
			frame_stencil_cmp(&frame, CELL_CONTENT_BIT, 0);
			frame_stencil_setne(&frame, CELL_FOREGROUND_BIT, &CELL_FOREGROUND_GRAY(128));

			/* Push through the framebuffer */
			t_reset();
			t_clear();
			frame_rasterize(&frame, 0, 0);

			old_clip = frame.clip;
			frame.clip = roll_clip;
			app__roll_position_cursor(&frame);
			frame.clip = old_clip;

			t_flush();
		}

		switch (t_poll()) {
		case T_POLL_CODE(0, 'h'):
			--g_roll_cursor_index;
			break;
		case T_POLL_CODE(0, 'j'):
			--g_roll_cursor_atom;
			break;
		case T_POLL_CODE(0, 'k'):
			++g_roll_cursor_atom;
			break;
		case T_POLL_CODE(0, 'l'):
			++g_roll_cursor_index;
			break;
		}
	}
	
	/* 
	 * Application-specific cleanup.
	 */
	frame_free(&frame);

	app__destroy_services();

	return 0;
}
#endif /* ifndef APP_DEMO */


#ifdef APP_DEMO
extern int 
demo();

int 
main()
{
#define SUPPRESS(x) ((void) (x))
	SUPPRESS(g_should_run);
	SUPPRESS(g_time_draw_last);

	app__init_services();

	int const demo_status = demo();

	app__destroy_services();

	return demo_status;
}
#endif /* ifdef APP_DEMO  */
