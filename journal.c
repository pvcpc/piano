#include <stdlib.h>

#include "journal.h"


static void
journal__fit_memory_overhead(struct journal *journal)
{
	/* @NOTE(max): for the moment, we assume that records are added in
	 * time order; realistically, we would want to check the time field
	 * of the records to determine the oldest ones we can dump into
	 * a file or something. 
	 *
	 * @NOTE(max): with enough records this routine will be called
	 * constantly which might not be what we want. 
	 */
	while (journal->_head && journal->memory_overhead_current > journal->memory_overhead_threshold) {

		struct journal_record *now = journal->_head;
		struct journal_record *next = now->next;

		journal->memory_overhead_current -= now->_approx_size;
		free(now);

		journal->_head = next;
	}
}

struct journal_record *
journal_append_record(struct journal *journal, u32 content_reserve_size)
{
	/* very simple, try to allocate the record first  */
	u32 record_size = sizeof(struct journal_record) + content_reserve_size;

	/* I believe most malloc(3) implementations align memory size on
	 * a 16-byte boundary on any modern 64-bit system. */
	u32 approx_size = ALIGN_UP(record_size, 16);

	struct journal_record *record = malloc(record_size);
	if (!record) {
		return NULL;
	}

	record->_record_size = record_size;
	record->_approx_size = approx_size;

	record->content = ((char *) record) + sizeof(struct journal_record);
	record->time    = 0;
	record->serial  = 0; /* default serial number */
	record->next    = NULL;
	record->prev    = NULL;

	/* then add it to the end there */
	struct journal_record *prev = NULL;
	struct journal_record **now = &journal->_head;

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

	/*  */
	journal__fit_memory_overhead(journal);

	return record;
}
