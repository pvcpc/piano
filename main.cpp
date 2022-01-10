// #include <cstdio>

// extern "C"
// {
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
// }


int
main(void)
{
	/* disable "canonical mode" so we can read input as it's typed. We
	 * get two copies so we can restore the previous settings at he end 
	 * of the program.
	 */
	struct termios old, now;
	tcgetattr(STDIN_FILENO, &old);
	tcgetattr(STDIN_FILENO, &now);

	now.c_lflag &= ~(ICANON | ECHO);
	now.c_cc[VTIME] = 0; /* min of 0 deciseconds for read(3) to block */
	now.c_cc[VMIN] = 0; /* min of 0 characters for read(3) */
	tcsetattr(STDIN_FILENO, TCSANOW, &now);

	/* test out ansi commands; clear screen */
	unsigned char const cmd_clear[] = "\x1b[2J";
	write(STDOUT_FILENO, cmd_clear, sizeof(cmd_clear));

	/* move cursor to top left */
	unsigned char const cmd_mov_topleft[] = "\x1b[1;1H";
	write(STDOUT_FILENO, cmd_mov_topleft, sizeof(cmd_mov_topleft));

	/* request "device status report," i.e., current cursor position.
	 * (information comes through stdin, but we don't parse it.) */
	unsigned char const cmd_device_report[] = "\x1b[6n";
	write(STDOUT_FILENO, cmd_device_report, sizeof(cmd_device_report));

	/* read buffer of input from stdin to capture device status report */
	unsigned char buf [64];
	int nread = 0;

	/* because read(3) doesn't block, we need a while to make sure it reads. */
	while (!nread) {
		nread = read(STDIN_FILENO, buf, sizeof(buf));
	}
	for (int i = 0; i < nread; ++i) {
		printf("byte %02d: 0x%02x\n", i, buf[i]);
	}

	/* reset terminal settings back to normal */
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return 0;
}
