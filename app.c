#include <string.h>
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

/* @SECTION(logging) */

/* @GLOBAL */
static struct journal  g_journal_sys;

void
app_log(enum journal_level level, char const *source, char const *restrict format_message, ...)
{
	/* @TODO implement source field in journal */
	UNUSED(source);

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

	record->source = source;

	record->time = app_uptime();
	record->level = level;
}

void
_app_dump_system_journal(s32 fd)
{
	struct journal_record *now = g_journal_sys.head;
	while (now) {
		dprintf(fd, "[serial:%6d][src:%16s][cs:%4d][time:%9.4f][level:%s] %s\n", 
			now->serial, now->source, now->content_size, now->time, journal_level_string(now->level), now->content
		);
		now = now->next;
	}
}

/* @SECTION(activities) */
#define APP__ACTIVITY_POOL_SIZE 8
#define APP__ACTIVITY_NAME_SIZE 64

struct app__activity
{
	struct activity_callbacks  cbs;
	void                      *opaque;
	s32                        handle;
	char                       name   [APP__ACTIVITY_NAME_SIZE];
};

/* @GLOBAL */
static struct app__activity  g_activity_pool  [APP__ACTIVITY_POOL_SIZE];
static struct app__activity *g_activity_table [APP__ACTIVITY_POOL_SIZE];
static s32                   g_activity_tail;

static inline bool
app__activity_is_available(struct app__activity *act)
{
	return act->handle < 0;
}

s32
app_activity_create(struct activity_callbacks const *cbs, char const *hint_name)
{
	if (!cbs) {
		app_log_error("Attempted to register activity '%s' without callbacks.", hint_name);
		return -1;
	}

	/* @TODO some callbacks may/should be made optional later */
	bool callback_checks_failed = false;

#define APP__CALLBACK_REQUIRED(CallbackFunc, CallbackName) \
	if (!CallbackFunc) { \
		app_log_error("Whilst registering activity '%s': required callback '%s' is missing.", hint_name, CallbackName); \
		callback_checks_failed = true; \
	}

	APP__CALLBACK_REQUIRED(cbs->on_init   , "on_init"   );
	APP__CALLBACK_REQUIRED(cbs->on_destroy, "on_destroy");

	APP__CALLBACK_REQUIRED(cbs->on_appear , "on_appear" );
	APP__CALLBACK_REQUIRED(cbs->on_vanish , "on_vanish" );
	APP__CALLBACK_REQUIRED(cbs->on_focus  , "on_focus"  );
	APP__CALLBACK_REQUIRED(cbs->on_unfocus, "on_unfocus");

	APP__CALLBACK_REQUIRED(cbs->on_input  , "on_input"  );
	APP__CALLBACK_REQUIRED(cbs->on_update , "on_update" );
	APP__CALLBACK_REQUIRED(cbs->on_render , "on_render" );

	if (callback_checks_failed) {
		return -1;
	}

	/* Check for open slot, and find a free activity if possible */
	if (g_activity_tail >= APP__ACTIVITY_POOL_SIZE) {
		app_log_error("Whilst registering activity '%s': activity pool already exhausted.", hint_name);
		return -1;
	}

	struct app__activity *fresh_activity = NULL;

	for (s32 i = 0; i < APP__ACTIVITY_POOL_SIZE; ++i) {
		struct app__activity *target = &g_activity_pool[i];
		if (app__activity_is_available(target)) {
			fresh_activity = target;
			break;
		}
	}

	if (!fresh_activity) {
		app_log_critical("Whilst registering activity '%s': activity pool contains no free activities whilst g_activity_tail says otherwise.", hint_name);
		return -1;
	}

	/* Otherwise, set this activity up */
	fresh_activity->cbs    = *cbs;
	fresh_activity->opaque = NULL;
	fresh_activity->handle = g_activity_tail++;

	memset(fresh_activity->name, 0, sizeof(fresh_activity->name));

	g_activity_table[fresh_activity->handle] = fresh_activity;

	/* call up the initializer */
	fresh_activity->cbs.on_init(fresh_activity->handle);

	return fresh_activity->handle;
}

s32
app_activity_set_opaque(s32 handle, void *opaque)
{
	if (handle < 0 || g_activity_tail <= handle) {
		app_log_error("Attempted to set opaque (&%p) for invalid activity (#%d).", opaque, handle);
		return -1;
	}
	
	struct app__activity *act = g_activity_table[handle];
	act->opaque = opaque;
	return act->handle;
}

s32
app_activity_get_opaque(s32 handle, void **opaque)
{
	if (handle < 0 || g_activity_tail <= handle) {
		app_log_error("Attempted to get opaque (&&%p) for invalid activity (#%d).", opaque, handle);
		return -1;
	}
	
	struct app__activity *act = g_activity_table[handle];
	if (opaque) *opaque = act->opaque;
	return act->handle;
}

/* @SECTION(misc_services) */
#define APP__NANO 1000000000

/* @GLOBAL */
static struct timespec g_genesis;

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

static void
app__init_services()
{
	if (!t_manager_setup()) {
		app_panic_and_die(1, "Not a TTY!");
	}

	if (clock_gettime(CLOCK_MONOTONIC, &g_genesis) < 0) {
		app_panic_and_die(1, "Check your clock captain!");
	}

	/*  */
	for (s32 i = 0; i < APP__ACTIVITY_POOL_SIZE; ++i) {
		g_activity_pool[i].handle = -1;
	}
}

static void
app__destroy_services()
{
	t_manager_cleanup();
}

/* @SECTION(app) */
#define APP__UPDATE_DELTA 8e-3
#define APP__DRAW_DELTA 8e-3

/* @GLOBAL  */
static bool            g_should_run       = true;


#ifndef APP_DEMO
extern void
bounce_create_activity();

int
main(void)
{
	/* 
	 * Setup
	 */
	app__init_services();

	bounce_create_activity();
	bounce_create_activity();

	struct frame frame;
	frame_alloc(&frame, 0, 0);

	double tm_update_last = app_uptime();
	double tm_render_last = app_uptime();

	/*
	 * Do cool stuff
	 */
	while (g_should_run) {

		double tm_now = app_uptime();
		double tm_update_delta = tm_now - tm_update_last;
		double tm_render_delta = tm_now - tm_render_last;

		if (tm_update_delta >= APP__UPDATE_DELTA) {
			tm_update_last = tm_now;

			for (s32 i = 0; i < g_activity_tail; ++i) {
				struct app__activity *act = g_activity_table[i];
				act->cbs.on_update(act->handle, tm_update_delta);
			}
		}

		if (tm_render_delta >= APP__DRAW_DELTA) {
			tm_render_last = tm_now;

			s32 term_w, term_h;
			t_query_size(&term_w, &term_h);

			frame_realloc(&frame, term_w, term_h);

			/* For simplicity, let's use the regular stack layout */
			for (s32 i = 0; i < g_activity_tail; ++i) {
				struct app__activity *act = g_activity_table[i];

				s32 const y0 = (frame.height * i) / g_activity_tail;
				s32 const y1 = (frame.height * (i+1)) / g_activity_tail;

				frame_clip_absolute(&frame, 0, y0, frame.width, y1);

				act->cbs.on_render(act->handle, &frame, tm_render_delta);
			}

			frame_zero_clip(&frame);

			t_reset();
			t_clear();
			frame_rasterize(&frame, 0, 0);
			t_flush();
		}

		switch (t_poll()) {
		case T_POLL_CODE(0, 'q'):
			g_should_run = false;
			break;
		}
	}
	
	/* 
	 * Cleanup
	 */
	frame_free(&frame);

	app__destroy_services();

	_app_dump_system_journal(STDOUT_FILENO);

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

	app__init_services();

	int const demo_status = demo();

	app__destroy_services();

	return demo_status;
}
#endif /* ifdef APP_DEMO  */
