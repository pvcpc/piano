/* Testing out typsetting algorithms */
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <common.h>


#if 1
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
#else
static char const * const SAMPLE = "Lorem ipsum d olor sit amet";
#endif

static void
print_typeset_flrr(char const *message, s32 width)
{
	if (width <= 1 || 4096 <= width) {
		fputs("Width out of bounds: width <= 1 || 4096 <= width", stderr);
		return;
	}

	s32 buffer_width = width + 1; /* room for the null terminator */

	char scratch [buffer_width];
	char *end    = scratch + buffer_width;
	char *zterm  = scratch + buffer_width - 1;
	char *cursor = scratch;

	char const *base = NULL;

	while (*message) {

		/* plop a space */
		base = message;
		while (*message && !isgraph(*message)) {
			++message;
		}

		if (message > base) {
			if (cursor == zterm) {
				*cursor = 0;
				puts(scratch);
				cursor = scratch;
			}
			else {
				*cursor++ = ' ';
			}
		}

		/* plop a word */
		base = message;
		while (isgraph(*message)) {
			++message;
		}

		while ((message - base) > width) {
			if ((end - cursor) <= 1) { /* we need at least 2 spaces */
				*cursor = 0;
				puts(scratch);
				cursor = scratch;
			}
			u32 const space = end - cursor - 1;
			cursor = strncpy(cursor, base, space - 1) + space - 1;
			*cursor++ = '-';
			base += space - 1;
		}
		
		if ((end - cursor) < (message - base)) {
			*cursor = 0;
			puts(scratch);
			cursor = scratch;
		}

		/* assume for the moment that the word can fit into the buffer.  */
		cursor = strncpy(cursor, base, (message - base)) + (message - base);
	}

	/* flush */
	if (scratch < cursor) {
		*cursor = 0;
		puts(scratch);
	}
}

int
demo()
{
	puts("====== FLUSHED LEFT, RAGGED RIGHT =====");
	print_typeset_flrr(SAMPLE, 32);
	return 0;
}
