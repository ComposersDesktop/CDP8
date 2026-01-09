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

int aaio_hard_reset(void);

/**
 * Flush any remaining characters from the stdandard input.
 * 
 * @return The number of charactes removed.
 */
int aaio_flush(void);

#ifdef __cplusplus
}
#endif

#endif //_L_AAIO_
