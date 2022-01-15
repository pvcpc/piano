#ifndef INCLUDE_T_BASE_H
#define INCLUDE_T_BASE_H

#define T_PARAMS_MAX 16
#define T_INTERS_MAX 16
#define T_SEQUENCE_BUF_SIZE 16

enum t_status
{
	T_EUNKNOWN = -4,
	T_EEMPTY   = -3,
	T_ENOOPS   = -2,
	T_ENULL    = -1,
	T_OK       =  0,
};

enum t_event_mod
{
	T_META    = 0x01,
	T_CONTROL = 0x02,
	T_ALT     = 0x04,
	T_SHIFT   = 0x08,
};

struct t_event
{
	uint8_t  mod;
	uint8_t  val;

	uint8_t  params [T_PARAMS_MAX];
	uint8_t  inters [T_INTERS_MAX];
	uint32_t n_params;
	uint32_t n_inters;

	uint8_t  seq_buf [T_SEQUENCE_BUF_SIZE];
	uint32_t seq_len;
};

void
t_setup();

void
t_cleanup();

char const *
t_status_string(
	enum t_status stat
);

enum t_status
t_poll(
	struct t_event *out
);


#endif /* INCLUDE_T_BASE_H */
