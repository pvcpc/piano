#include <soundio/soundio.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <unistd.h>

#include "demos.h"

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "keyboard.h"

#ifdef DEMO_PAUSE_BEFORE_EXIT 
#  define DEMO__PAUSE while (t_poll() != T_POLL_CODE(0, 'q'))
#else
#  define DEMO__PAUSE
#endif

static float const demo__soundio_ampl = 1.0f;
static float const demo__soundio_freq = 440.0f;
static float const demo__soundio_rads_per_second = 2 * M_PI;
static float demo__soundio_seconds_offset = 0.0f;

static void
demo__soundio_sin_write_callback(
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
		float f_sample = demo__soundio_ampl * sinf(
			(demo__soundio_seconds_offset + (frame * period)) * 
			demo__soundio_rads_per_second * 
			demo__soundio_freq
		) * INT16_MAX;

		int16_t sample = (int16_t) (f_sample);

		for (int i = 0; i < layout->channel_count; ++i) {
			*((int16_t *) areas[i].ptr) = sample;
			areas[i].ptr += areas[i].step;
		}

		++frame;
	}

	demo__soundio_seconds_offset = fmodf(
		demo__soundio_seconds_offset + (frame * period), 1.0f
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
demo__soundio_sin_underflow_callback()
{
	static int count = 0;
	fprintf(stderr, "Underflow #%d\n", count++);
}

int
demo_soundio_sin()
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

	out->write_callback = demo__soundio_sin_write_callback;
	out->underflow_callback = demo__soundio_sin_underflow_callback;
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

int
demo_midi_parse()
{
	return 0;
}

int
demo_frame_pattern()
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

int
demo_frame_map_one()
{
	t_setup();

	struct t_frame frame;
	t_frame_create_pattern(&frame, 0,
		"+----+\n"
		"|    |\n"
		"|    |\n"
		"+----+\n"
	);
	
	t_frame_map_one(&frame, T_MAP_CH | T_MAP_ALTFG | T_MAP_ALTBG,
		T_RGB(0, 0, 0),
		T_RGB(255, 255, 255),
		' ', 'M'
	);
	
	t_reset();
	t_clear();
	t_frame_rasterize(&frame, 0, 0);

	t_cleanup();
	return 0;
}

int
demo_frame_coordinate_system()
{
	t_setup();

	struct t_frame frame;
	t_frame_create_pattern(&frame, 0,
		"+----+\n"
		"|    |\n"
		"|    |\n"
		"+----+\n"
	);

	t_reset();
	t_clear();

#if 0
	/* graphically canonical coordinate system (top left of the screen) */
	t_frame_context_set_gravity(&frame, T_GRAVITY_LEFT, T_GRAVITY_TOP);
	t_frame_context_set_alignment(&frame, T_ALIGNMENT_LEFT, T_ALIGNMENT_TOP);
	t_frame_context_set_direction(&frame, T_DIRECTION_L2R, T_DIRECTION_T2B);
	t_frame_rasterize(&frame, 0, 0);
	t_frame_rasterize(&frame, 2, 1);

	/* mathematically canonical coordinate system (bottom left of
	 * the screen). */
	t_frame_context_set_gravity(&frame, T_GRAVITY_LEFT, T_GRAVITY_BOTTOM);
	t_frame_context_set_alignment(&frame, T_ALIGNMENT_LEFT, T_ALIGNMENT_BOTTOM);
	t_frame_context_set_direction(&frame, T_DIRECTION_L2R, T_DIRECTION_B2T);
	t_frame_rasterize(&frame, 0, 0);
	t_frame_rasterize(&frame, 2, 1);

	/* center of the screen */
	t_frame_context_set_gravity(&frame, T_GRAVITY_CENTER, T_GRAVITY_CENTER);
	t_frame_context_set_alignment(&frame, T_ALIGNMENT_CENTER, T_ALIGNMENT_CENTER);
	t_frame_context_set_direction(&frame, T_DIRECTION_L2R, T_DIRECTION_T2B);
	t_frame_rasterize(&frame,  0,  0);
	t_frame_rasterize(&frame,  4,  4);
	t_frame_rasterize(&frame,  4, -4);
	t_frame_rasterize(&frame, -4,  4);
	t_frame_rasterize(&frame, -4, -4);
	
	/* top right of the screen */
	t_frame_context_set_gravity(&frame, T_GRAVITY_RIGHT, T_GRAVITY_TOP);
	t_frame_context_set_alignment(&frame, T_ALIGNMENT_RIGHT, T_ALIGNMENT_TOP);
	t_frame_context_set_direction(&frame, T_DIRECTION_R2L, T_DIRECTION_T2B);
	t_frame_rasterize(&frame, 0, 0);
	t_frame_rasterize(&frame, 2, 1);

#endif
	/* bottom right of the screen */
	t_frame_context_set_gravity(&frame, T_GRAVITY_RIGHT, T_GRAVITY_BOTTOM);
	t_frame_context_set_alignment(&frame, T_ALIGNMENT_RIGHT, T_ALIGNMENT_BOTTOM);
	t_frame_context_set_direction(&frame, T_DIRECTION_R2L, T_DIRECTION_B2T);
	t_frame_rasterize(&frame, 0, 0);
	t_frame_rasterize(&frame, 2, 1);

	t_flush();
	DEMO__PAUSE;

	t_cleanup();
	return 0;
}

int
demo_frame_typeset()
{
	t_setup();

	struct t_frame frame;
	t_frame_create(&frame, 16, 16);

	t_reset();
	t_clear();
	t_frame_rasterize(&frame, 0, 0);

	t_cleanup();
	return 0;
}

int
demo_frame_blend()
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
		0,
		T_RGB(255, 255, 255), 
		T_RGB(255, 0, 0),
		0, 0
	);

	t_reset();
	t_clear();
	t_frame_rasterize(&bottom, 0, 0);

	t_write_z("\n");

	t_cleanup();
	return 0;
}

int
demo_frame_blend_coordinate_system()
{
	t_setup();

	t_reset();
	t_clear();

	struct t_frame first;
	t_frame_create_pattern(&first, 0,
		"+--------+\n"
		"|        |\n"
		"|        |\n"
		"|        |\n"
		"|        |\n"
		"+--------+\n"
	);
	t_frame_paint(&first, T_RGB(0, 0, 0), T_RGB(255, 255, 255));

	struct t_frame second;
	t_frame_create_pattern(&second, 0,
		"+----+\n"
		"|    |\n"
		"|    |\n"
		"+----+\n"
	);
	t_frame_paint(&second, T_RGB(255, 255, 255), T_RGB(192, 0, 0));

	t_frame_context_set_gravity(&first, T_GRAVITY_RIGHT, T_GRAVITY_BOTTOM);
	t_frame_context_set_alignment(&first, T_ALIGNMENT_RIGHT, T_ALIGNMENT_BOTTOM);
	t_frame_blend(&first, &second, ~(0), 0, 0, 0, 0, 0);

	t_frame_context_reset_everything(&first);
	t_frame_rasterize(&first, 0, 0);

	t_cleanup();
	return 0;
}
