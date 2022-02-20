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

/* @SECTION(activities) */
#define APP__ACTIVITY_POOL_SIZE 8

struct app__activity
{
	struct activity_callbacks  cbs;
	void                      *opaque;
	char const                *name;
	s32                        handle;
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
app_activity_init(struct activity_callbacks const *cbs, void *opaque, char const *name)
{
	/* @TODO should name be required? I think it makes sense */
	if (!name) {
		app_log_error("Attempted to register an activity without a name.");
		return -1;
	}

	if (!cbs) {
		app_log_error("Attempted to register activity '%s' without callbacks.", name);
		return -1;
	}

	/* @TODO some callbacks may/should be made optional later */
	bool callback_checks_failed = false;

#define APP__CALLBACK_REQUIRED(CallbackFunc, CallbackName) \
	if (!CallbackFunc) { \
		app_log_error("Whilst registering activity '%s': required callback '%s' is missing.", name, CallbackName); \
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
		app_log_error("Whilst registering activity '%s': activity pool already exhausted.", name);
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
		app_log_critical("Whilst registering activity '%s': activity pool contains no free activities whilst g_activity_tail says otherwise.", name);
		return -1;
	}

	/* Otherwise, set this activity up */
	fresh_activity->cbs    = *cbs;
	fresh_activity->opaque = opaque;
	fresh_activity->name   = name;
	fresh_activity->handle = g_activity_tail++;

	return fresh_activity->handle;
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

	// g_journal_sys.memory_overhead_threshold = KILO(64);
}

static void
app__destroy_services()
{
	t_manager_cleanup();
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

		u16 poll_code = t_poll();

		/* check for system keybinds */
		switch (poll_code) {

		}

		/* dispatch to active activity */
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
