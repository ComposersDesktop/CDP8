/*
 * AAIO Advanced I/O
 * -----------------
 *
 * Many people moving from Windows programming to UNIX program have problems 
 * with the missing non-blocking getch() and getche() functions provided by 
 * conio.h. This library provides the functionality of getch(), getche() and
 * kbhit(). It does not require an initialization (like curses) and does 
 * not abuse the terminal (i.e. whenever the mode of the terminal is 
 * changed its state is stored so it can be restored). For increased 
 * efficiency there exists funcionality to allow abuse of the terminal as well 
 * as functions to restore or reset the terminal when the application exits.
 */

/*
 * Copyright (c) 2004-2005 by Daniel Aarno - <macbishop@users.sf.net, aarno@google.com>
 * M. Sc. Electrical Engineering * http://www.nada.kth.se/~bishop
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * * Relicensed from Attribution Assurance License to LGPLv2 with 
 * permission from the original author (received 2025-01-09).
 */

#ifndef _L_AAIO_
#define _L_AAIO_

#define AAIO_VERSION 0.3.0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get a character from stdin. Block until a character is available 
 * but does not wait for a new line. Characters are not echoed on to 
 * screen.
 * 
 * @return The first character available cast to an int or -1 on error.
 * @see getche
 */
int getch(void);

/**
 * Get a character from stdin. Block until a character is available 
 * but does not wait for a new line. Characters are echoed on to 
 * screen.
 * 
 * @return The first character available cast to an int or -1 on error.
 * @see getch
 */
int getche(void);

/**
 * Get the number of characters available on stdin.
 *
 * @return The number of characters that can be read from stdin without 
 * blocking, -1 on error.
 * @see getch
 * @see getche
 */
int kbhit(void);

/**
 * Perform a hard reset of the terminal to cause it to go back to its 
 * original state.
 *
 * @return -1 on error, 0 on success.
 * @see reset(1)
 * @see aaio_reset
 */
int aaio_hard_reset(void);

/**
 * Reset the terminal to the state it was in when aaio_grant_tty_lock was
 * called.
 * 
 * @deprecated There is no need to use the aaio_grant_tty_lock and
 * aaio_reset functions as of version 0.2.0 an later. The speedup thay
 * can achieve is reduced to an insignificant level. The functionality
 * may be removed in later versions of the library.
 *
 * @return -1 on error, 0 on success.
 * @see aaio_hard_reset
 * @see aaio_grant_tty_lock
 */
int aaio_reset(void);

/**
 * Allow the aaio functions (getch, getche and kbhit) to "abuse" the terminal
 * (i.e. change the terminal settings without resetting them. This gives a
 * boost to performance that can be usefull in some cases. Most often,
 * however, it is not necessary or recomended to grant the tty lock to aaio.
 *
 * If the lock has been granted aaio_reset must be called before program 
 * exit to reset the terminal to its original state.
 *
 * @deprecated There is no need to use the aaio_grant_tty_lock and
 * aaio_reset functions as of version 0.2.0 an later. The speedup thay
 * can achieve is reduced to an insignificant level. The functionality
 * may be removed in later versions of the library.
 *
 * @return -1 on error, 0 on success.
 * @note The aaio library does not really get a lock on the tty, others may
 * change the tty settings, however this will corrupt the aaio library and
 * can cause undefined results.
 * @see aaio_reset
 */
int aaio_grant_tty_lock(void);
  
/**
 * Flush any remaining characters from the stdandard input.
 * 
 * @return The number of charactes removed.
 */
int aaio_flush(void);

#ifdef __cplusplus
}
#endif

#endif

