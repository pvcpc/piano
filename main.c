#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "t_base.h"


int
main(void)
{
	t_setup();

	while (1) {
		struct t_event ev;
		enum t_status stat = t_poll(&ev);
		
		if (stat >= 0) {
			printf("cmd: ");
			for (uint32_t i = 0; i < ev.seq_len; ++i) {
				printf("%02x ", ev.seq_buf[i]);
			}
			printf("qcode: %04x\n", ev.qcode);
			if (ev.n_params) {
				puts("\tparams:");
				for (uint32_t i = 0; i < ev.n_params; ++i) {
					printf("\t%2d: %d\n", i, ev.params[i]);
				}
			}
			if (ev.n_inters) {
				puts("\tinters:");
				for (uint32_t i = 0; i < ev.n_inters; ++i) {
					printf("\t%2d: %02x\n", i, ev.inters[i]);
				}
			}
		}
		else if (stat != T_EEMPTY) {
			fprintf(stderr, "t_poll failed: %s\n",
				t_status_string(stat));
		}
	}

	t_cleanup();
	return 0;
}
