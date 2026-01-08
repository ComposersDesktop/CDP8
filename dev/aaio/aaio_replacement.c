#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/**
 * Internal helper to mimic AAIO's "set_stty_raw" behavior.
 * This function is called every time a user wants to check the keyboard.
 */
static int toggle_raw_mode(struct termios *original, int activate) {
	if (activate) {
		// Store the current state so we can restore it seconds later.
		if (tcgetattr(STDIN_FILENO, original) == -1) return -1;

		struct termios raw = *original;
		// ICANON: Disable line buffering (get keys immediately).
		// ECHO: Disable visual feedback of the key pressed.
		raw.c_lflag &= ~(ICANON | ECHO);
		raw.c_cc[VMIN] = 1;
		raw.c_cc[VTIME] = 0;

		return tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	} else {
		// Put the terminal back to exactly how we found it.
		return tcsetattr(STDIN_FILENO, TCSANOW, original);
	}
}

int getch(void) {
	struct termios old;

	if (toggle_raw_mode(&old, 1) == -1) return -1;
	int ch = getchar();
	toggle_raw_mode(&old, 0);

	return ch;
}

/**
 * WARNING: This implementation matches the original AAIO 'Path A' behavior.
 * It performs multiple system calls (tcgetattr/tcsetattr) every time it is called.
 * * If used in a tight 'while(!kbhit())' loop, this will cause high CPU usage
 * because it is constantly toggling the terminal driver state thousands of 
 * times per second.
 */
int kbhit(void) {
	struct termios old;
	int bytes_waiting = 0;

	if (toggle_raw_mode(&old, 1) == -1) return -1;

	// FIONREAD peeks at the OS buffer without consuming data.
	if (ioctl(STDIN_FILENO, FIONREAD, &bytes_waiting) == -1) {
		toggle_raw_mode(&old, 0);
		return -1;
	}

	toggle_raw_mode(&old, 0);
	return (bytes_waiting > 0);
}
