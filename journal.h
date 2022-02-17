#ifndef INCLUDE__JOURNAL_H
#define INCLUDE__JOURNAL_H

#include "common.h"


enum journal_level
{
	JOURNAL_LEVEL_INFO  = 0,
	JOURNAL_LEVEL_WARN  = 1,
	JOURNAL_LEVEL_ERROR = 2,
};

struct journal_record
{
	char                  *content;
	double                 time;
	s32                    serial;
	enum journal_level     level;

	/* @NOTE(max): I know linked list but I literally don't care. */
	struct journal_record *prev;
	struct journal_record *next;

	u32                    _record_size;
	u32                    _approx_size;
};

struct journal
{
	u32                    memory_overhead_current;
	u32                    memory_overhead_threshold;

	struct journal_record *_head;
};


static inline char const *
journal_level_string(enum journal_level level)
{
	/* we limit strings to 4 characters becauses it's obvious and it
	 * aligns log dumps nicely. */
	switch (level) {
	case JOURNAL_LEVEL_INFO:
		return "INFO";
	case JOURNAL_LEVEL_WARN:
		return "WARN";
	case JOURNAL_LEVEL_ERROR:
		return "ERRO";
	
	default:
		return "UNDF";
	}
}

s32
journal_length(struct journal *journal);

struct journal_record *
journal_append_record(struct journal *journal, u32 content_reserve_size);

#endif /*  INCLUDE__JOURNAL_H */
