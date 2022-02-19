#include <stdlib.h>

#include "journal.h"


struct journal_record *
journal_create_record(struct journal *journal, u32 content_reserve_size)
{
	/* very simple, try to allocate the record first  */
	u32 record_size = sizeof(struct journal_record) + content_reserve_size;

	/* I believe most malloc(3) implementations align memory size on
	 * a 16-byte boundary on any modern 64-bit system. In any case,
	 * this may hopefully be used to give a more accurate statement of
	 * the true memory consumption by journals */
	u32 approx_size = ALIGN_UP(record_size, 16);

	struct journal_record *record = malloc(record_size);
	if (!record) {
		return NULL;
	}

	journal->memory_overhead_current += approx_size;

	record->record_size  = record_size;
	record->approx_size  = approx_size;

	record->content      = ((char *) record) + sizeof(struct journal_record);
	record->time         = 0;
	record->content_size = content_reserve_size;
	record->serial       = 0; /* default serial number */
	record->next         = NULL;
	record->prev         = NULL;

	/* then add it to the end there */
	struct journal_record *prev = NULL;
	struct journal_record **now = &journal->head;

	while (*now) {
		prev = *now;
		now = &(*now)->next;
	}

	*now = record;
	record->prev = prev;

	if (prev) {
		record->serial = prev->serial + 1;
		prev->next = record;
	}

	/* @LEAK figure out a nice way to clear out the journal sometime. */

	return record;
}
