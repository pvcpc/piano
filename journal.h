#ifndef INCLUDE__JOURNAL_H
#define INCLUDE__JOURNAL_H

#include "common.h"


enum journal_level
{
	JOURNAL_LEVEL_INF0     = 0, /* very verbose */
	JOURNAL_LEVEL_INF1     = 1, /* pretty verbose */
	JOURNAL_LEVEL_INF2     = 2, /* more verbose */
	JOURNAL_LEVEL_INF3     = 3, /* standard info */
	JOURNAL_LEVEL_WARN     = 4,
	JOURNAL_LEVEL_ERROR    = 5,
	JOURNAL_LEVEL_CRITICAL = 6,
};

struct journal_record
{
	u32                    record_size;
	u32                    approx_size;

	double                 time;
	char const            *source;
	char                  *content;
	u32                    content_size;
	u32                    serial;
	enum journal_level     level;

	/* @NOTE(max): I know linked list but I literally don't care. */
	struct journal_record *prev;
	struct journal_record *next;
};

struct journal
{
	u32                    memory_overhead_current;
	u32                    memory_overhead_threshold;
	u32                    num_records;

	struct journal_record *head;
};


static inline char const *
journal_level_string(enum journal_level level)
{
	/* we limit strings to 4 characters becauses it's obvious and it
	 * aligns log dumps nicely. */
	switch (level) {

	case JOURNAL_LEVEL_INF0:
		return "INF0";
	case JOURNAL_LEVEL_INF1:
		return "INF1";
	case JOURNAL_LEVEL_INF2:
		return "INF2";
	case JOURNAL_LEVEL_INF3:
		return "INF3";

	case JOURNAL_LEVEL_WARN:
		return "WARN";
	case JOURNAL_LEVEL_ERROR:
		return "ERRO";
	case JOURNAL_LEVEL_CRITICAL:
		return "CRIT";
	
	default:
		return "UNDF";
	}
}

s32
journal_length(struct journal *journal);

struct journal_record *
journal_create_record(struct journal *journal, u32 content_reserve_size);

#endif /*  INCLUDE__JOURNAL_H */
