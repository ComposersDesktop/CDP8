// Used on POSIX and not Windows since this is reimplementation of Windows specific functions

/**
 * * Functional replacement for `getch` and `kbhit` in AAIO Advanced I/O library (aaio). 
 * This file provides the same funtionality while removing licensing restrictions.
 */
#ifndef _L_AAIO_
#define _L_AAIO_

/**
 * Reads a single character from stdin without echoing it to the terminal.
 * * Implementation: Temporarily disables ICANON and ECHO via termios.
 * This matches the "Heavy Toggle" behavior of the original library, 
 * ensuring the terminal is returned to a 'normal' state immediately after.
 * * @return The character read, or -1 on system error.
 */
int getch(void);

/**
 * Checks if a key has been pressed (non-blocking).
 * * Implementation: Toggles terminal mode and uses ioctl(FIONREAD).
 * * WARNING: Calling this in a tight 'while(!kbhit())' loop will cause high 
 * CPU usage due to repeated system calls (`tcsetattr`). It is recommended 
 * to add a small sleep (usleep) in your application loops.
 * * @return 1 if a character is waiting, 0 if empty, -1 on error.
 */
int kbhit(void);
#endif //_L_AAIO_
