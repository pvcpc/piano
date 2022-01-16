#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>


static char const *format = "esc[%d;%d;%d;%dX";

/* 256 meguh byte */
static uint8_t large_dataset [1 << 28];

int
writef(
	uint8_t *buffer,
	uint32_t length,
	uint8_t const *format,
	...
) {
	if (!length) {
		return 0;
	}
	uint8_t const *base = buffer;
	uint8_t *end = buffer + length-1;
	/* parse */
	va_list vl;
	va_start(vl, format);
	while (*format && buffer < end) {
		switch (*format) {
		case '%': {
			uint8_t dec [64];
			uint32_t dec_p = 0;
			int arg = va_arg(vl, int);
			while (arg) {
				dec[dec_p++] = arg % 10;
				arg /= 10;
			}
			while (buffer < end && dec_p > 0) {
				*buffer++ = dec[--dec_p] + '0';
			}
			break;
		}
		}
		++format;
	}
	*buffer = 0;
	va_end(vl);
	return buffer - base;
}

int
main(void)
{
#if 0 /* testing writef */
	char buffer [64];
	writef((uint8_t *) buffer, 64, (uint8_t const *) format,
		57, 45, -8325, -12487325
	);
	puts(buffer);
#endif

#if 1
	/* generate random noise */
	puts("Generating random data");
	srand(time(NULL));

	for (uint32_t i = 0; i < sizeof(large_dataset); ++i) {
		large_dataset[i] = rand() & 0xff;
	}
	puts("Done generating random data");

	/* setup bench */
	struct timespec begin, end;
	uint8_t buffer [256];

	/* benchmark snprintf */
	puts("Benchmarking snprintf");
	double snprintf_time = 0;

	for (uint32_t i = 0; i < sizeof(large_dataset); i += 4) {
		clock_gettime(CLOCK_MONOTONIC, &begin);
		snprintf((char *) buffer, 256, format, 
			large_dataset[i+0],
			large_dataset[i+1],
			large_dataset[i+2],
			large_dataset[i+3]
		);
		clock_gettime(CLOCK_MONOTONIC, &end);
		snprintf_time += 1e-9 * (
			(end.tv_sec - begin.tv_sec) * 1e9 + 
			(end.tv_nsec - begin.tv_nsec)
		);
	}
	puts("Done snprintf");

	/* benchmark writef */
	puts("Benchmarking writef");
	double writef_time = 0;

	for (uint32_t i = 0; i < sizeof(large_dataset); i += 4) {
		clock_gettime(CLOCK_MONOTONIC, &begin);
		writef(buffer, 256, (uint8_t const*) format, 
			large_dataset[i+0],
			large_dataset[i+1],
			large_dataset[i+2],
			large_dataset[i+3]
		);
		clock_gettime(CLOCK_MONOTONIC, &end);
		writef_time += 1e-9 * (
			(end.tv_sec - begin.tv_sec) * 1e9 + 
			(end.tv_nsec - begin.tv_nsec)
		);
	}
	puts("Done writef");

	/* results */
	printf("snprintf_time: %.2fs\n", snprintf_time);
	printf("writef_time: %.2fs\n", writef_time);
#endif
}
