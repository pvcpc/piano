/* WIP demo source -- used for random testing and not for any 
 * particular demonstration.
 */
#include <soundio/soundio.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <unistd.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "keyboard.h"

/* Minimum delta in seconds between successive framebuffer 
 * rasterization operations. When t_poll() is used in non-blocking
 * mode, the UI/update loop may spin too quickly and cause serious 
 * visual artifacts when drawing to the terminal too quickly.
 *
 * On my system, a milisecond is more than enough to avoid those. This
 * is terminal dependent, I imagine.
 */
#define RASTERIZATION_DELTA_REQUIRED 1e-3

static int
soundio_testing_main();

static int
midi_parse_demo_main();

static int
frame_pattern_demo_main();

static int
frame_blend_demo_main();

static int
full_application_main();

#define SELECTED_MAIN soundio_testing_main

int
main()
{
	return SELECTED_MAIN();
}

static float const soundio__ampl = 1.0f;
static float const soundio__freq = 440.0f;
static float const soundio__rads_per_second = 2 * M_PI;
static float soundio__seconds_offset = 0.0f;

static void
soundio__testing_main_write_callback(
	struct SoundIoOutStream *stream,
	int frames_min,
	int frames_max
) {
	int err = 0;

	int frame = 0;
	int frames = frames_max; /* T_CLAMP(512, frames_min, frames_max); */
	float rate = stream->sample_rate;
	float period = 1.0f / rate;

	struct SoundIoChannelLayout *layout = &stream->layout;
	struct SoundIoChannelArea *areas;

	if ((err = soundio_outstream_begin_write(stream, &areas, &frames))) {
		fprintf(stderr, "Failed to begin writing to stream: %s\n",
			soundio_strerror(err));
		soundio_outstream_pause(stream, true);
		return;
	}

	while (frame < frames_max) {
		float f_sample = soundio__ampl * sinf(
			(soundio__seconds_offset + (frame * period)) * 
			soundio__rads_per_second * 
			soundio__freq
		) * INT16_MAX;

		int16_t sample = (int16_t) (f_sample);

		for (int i = 0; i < layout->channel_count; ++i) {
			*((int16_t *) areas[i].ptr) = sample;
			areas[i].ptr += areas[i].step;
		}

		++frame;
	}

	soundio__seconds_offset = fmodf(
		soundio__seconds_offset + (frame * period), 1.0f
	);

	if ((err = soundio_outstream_end_write(stream)) &&
		 err != SoundIoErrorUnderflow)
	{
		fprintf(stderr, "Failed to end writing to stream: %s\n",
			soundio_strerror(err));
		soundio_outstream_pause(stream, true);
		return;
	}
}

static void
soundio__testing_main_underflow_callback()
{
	static int count = 0;
	fprintf(stderr, "Underflow #%d\n", count++);
}

static int
soundio_testing_main()
{
	int stat = 0;
	int err = 0;

	struct SoundIo *sio = soundio_create();
	if (!sio) {
		fprintf(stderr, "Failed to initialize SoundIo.\n");
		stat = 1;
		goto e_sio_init;
	}

	if ((err = soundio_connect(sio))) {
		fprintf(stderr, "Failed to connect to a backend: %s\n",
			soundio_strerror(err));
		stat = 1;
		goto e_sio_backend;
	}
	
	printf("Connected to backend: %s\n",
		soundio_backend_name(sio->current_backend));
	soundio_flush_events(sio);

	struct SoundIoDevice *dev = soundio_get_output_device(
		sio, soundio_default_output_device_index(sio));
	if (!dev) {
		fprintf(stderr, "Failed to find default device.\n");
		stat = 1;
		goto e_sio_device;
	}

	if (dev->probe_error) {
		fprintf(stderr, "Failed to probe default device: %s\n",
			soundio_strerror(dev->probe_error));
		stat = 1;
		goto e_sio_device;
	}

	if (!soundio_device_supports_format(dev, SoundIoFormatS16LE)) {
		fprintf(stderr, "Default device does not support S16LE samples.\n");
		stat = 1;
		goto e_sio_device;
	}

	struct SoundIoOutStream *out = soundio_outstream_create(dev);
	if (!out) {
		fprintf(stderr, "Failed to allocate output stream.\n");
		stat = 1;
		goto e_sio_outstream;
	}

	out->write_callback = soundio__testing_main_write_callback;
	out->underflow_callback = soundio__testing_main_underflow_callback;
	out->name = "Testing Output Stream";
	out->software_latency = 0;
	out->sample_rate = 48000;
	out->format = SoundIoFormatS16LE;

	if ((err = soundio_outstream_open(out))) {
		fprintf(stderr, "Failed to open output stream: %s\n",
			soundio_strerror(err));
		stat = 1;
		goto e_sio_outstream;
	}

	if (out->layout_error) {
		fprintf(stderr, "Failed to set output stream channel layout: %s\n",
			soundio_strerror(out->layout_error));
		stat = 1;
		goto e_sio_outstream;
	}
	
	if ((err = soundio_outstream_start(out))) {
		fprintf(stderr, "Failed to start output stream: %s\n",
			soundio_strerror(err));
		stat = 1;
		goto e_sio_outstream;
	}

	while (1) {
		soundio_flush_events(sio);
	}

e_sio_outstream:
	soundio_outstream_destroy(out);
e_sio_device:
	soundio_device_unref(dev);
e_sio_backend:
	soundio_disconnect(sio);
e_sio_init:
	soundio_destroy(sio);
	return stat;
}

static int
midi_parse_demo_main()
{
	return 0;
}

static int
frame_pattern_demo_main()
{
	t_setup();

	t_reset();
	t_clear();

	struct t_frame frame;
	t_frame_create_pattern(&frame, T_FRAME_SPACEHOLDER,
		"+-------------+\n"
		"|| | ||| | | ||\n"
		"|| | ||| | | ||\n"
		"|+-+-+|+-+-+-+|\n"
		"| | | | | | | |\n"
		"| | | | | | | |\n"
		"+-+-+-+-+-+-+-+\n"
	);
	t_frame_paint(&frame, T_WASHED, T_WASHED);
	t_frame_rasterize(&frame, 0, 0);
	t_frame_destroy(&frame);

	t_cleanup();
	return 0;
}

static int
frame_blend_demo_main()
{
	t_setup();

	int32_t term_w, term_h;
	t_termsize(&term_w, &term_h);

	struct t_frame bottom;
	t_frame_create_pattern(&bottom, T_FRAME_SPACEHOLDER,
		"+--+  \n"
		"|  |  \n"
		"|  |  \n"
		"+--+  \n"
		"      \n"
		"      \n"
	);
	t_frame_paint(&bottom, T_RGB(64, 192, 255), T_RGB(255, 192, 640));

	struct t_frame top;
	t_frame_create_pattern(&top, T_FRAME_SPACEHOLDER,
		"      \n"
		"      \n"
		"  +--+\n"
		"  |  |\n"
		"  |  |\n"
		"  +--+\n"
	);
	t_frame_paint(&top, T_RGB(64, 64, 64), T_RGB(255, 255, 255));

	t_frame_blend(&bottom, &top, 
		~(T_BLEND_CH),
		T_RGB(255, 255, 255), T_RGB(255, 0, 0),
		0, 0
	);

	t_reset();
	t_clear();
	t_frame_rasterize(&bottom, 0, 0);

	t_write_z("\n");

	t_cleanup();
	return 0;
}

static int
full_application_main()
{
	enum t_status stat;

	/* initialize supports */
	stat = keyboard_support_setup();
	if (stat < 0) {
		fprintf(stderr, "Failed to initialize virtual keyboard support: %s\n",
			t_status_string(stat)
		);
		goto e_init_keyboard;
	}

	struct t_frame frame_primary;
	stat = t_frame_create(&frame_primary, 0, 0);
	if (stat < 0) {
		fprintf(stderr, "Failed to create primary framebuffer: %s\n",
			t_status_string(stat)
		);
		goto e_init_frame;
	}

	struct keyboard keyboard = {
		.midi_index_start = MIDI_INDEX(NOTE_C, 3),
		.midi_index_end   = MIDI_INDEX(NOTE_B, 5),
	};

	/* setup, ui variables, and ui loop */
	t_setup();

	double tm_graphics_last = t_elapsed();
	bool should_poll_wait = false;
	bool should_run = true;

	double tm_staccato_sustain = 0.25;
	int32_t user_octave = 4;

#define MAIN__HUMAN_STACCATO(note)     \
	keyboard_human_staccato(           \
		&keyboard,                     \
		tm_staccato_sustain,           \
		MIDI_INDEX(note, user_octave)  \
	)

	while (should_run) {
		double tm_now = t_elapsed();
		double tm_graphics_delta = tm_now - tm_graphics_last;

		if (tm_graphics_delta >= 1e-3) {
			tm_graphics_last = tm_now;

			/* update framebuffer */
			int32_t term_w, term_h;
			t_termsize(&term_w, &term_h);

			t_frame_resize(&frame_primary, term_w, term_h);
			t_frame_clear(&frame_primary);

			keyboard_draw(&frame_primary, &keyboard,
				T_RGB(128, 128, 128), T_WASHED,
				T_RGB(192, 128,  64), T_WASHED,
				0, 0
			);

			/* rasterize */
			t_reset();
			t_clear();

			t_frame_rasterize(&frame_primary, 0, 0);

#ifdef TC_DEBUG_METRICS
			t_cursor_pos(1, term_h);
			t_foreground_256_ex(0, 0, 0);
			t_background_256_ex(255, 255, 255);
			t_write_f("Stored: %u, Flushed: %u",
				t_debug_write_nstored(),
				t_debug_write_nflushed()
			);
			t_debug_write_metrics_clear();
#endif
			t_flush();
		}

		switch (t_poll(should_poll_wait)) {
		/* keyboard keybinds */
		case T_POLL_CODE(0, 'q'):
			MAIN__HUMAN_STACCATO(NOTE_C);
			break;
		case T_POLL_CODE(0, '2'):
			MAIN__HUMAN_STACCATO(NOTE_Cs);
			break;
		case T_POLL_CODE(0, 'w'):
			MAIN__HUMAN_STACCATO(NOTE_D);
			break;
		case T_POLL_CODE(0, '3'):
			MAIN__HUMAN_STACCATO(NOTE_Ds);
			break;
		case T_POLL_CODE(0, 'e'):
			MAIN__HUMAN_STACCATO(NOTE_E);
			break;
		case T_POLL_CODE(0, 'r'):
			MAIN__HUMAN_STACCATO(NOTE_F);
			break;
		case T_POLL_CODE(0, '5'):
			MAIN__HUMAN_STACCATO(NOTE_Fs);
			break;
		case T_POLL_CODE(0, 't'):
			MAIN__HUMAN_STACCATO(NOTE_G);
			break;
		case T_POLL_CODE(0, '6'):
			MAIN__HUMAN_STACCATO(NOTE_Gs);
			break;
		case T_POLL_CODE(0, 'y'):
			MAIN__HUMAN_STACCATO(NOTE_A);
			break;
		case T_POLL_CODE(0, '7'):
			MAIN__HUMAN_STACCATO(NOTE_As);
			break;
		case T_POLL_CODE(0, 'u'):
			MAIN__HUMAN_STACCATO(NOTE_B);
			break;

		/* administrative keybinds */
		case T_POLL_CODE(0, 'Q'):
			should_run = 0;
			break;
		}
	}

	/* done */
	t_frame_destroy(&frame_primary);
	t_reset();
	t_clear();
	t_cleanup();

e_init_frame:
	t_frame_destroy(&frame_primary);
e_init_keyboard:
	keyboard_support_cleanup();
	return stat > 0 ? 0 : 1;
}
