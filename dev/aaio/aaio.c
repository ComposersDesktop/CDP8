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
-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1

 * Copyright (c) 2004-2005 by Daniel Aarno - <macbishop@users.sf.net>
 * M. Sc. Electrical Engineering * http://www.nada.kth.se/~bishop
 * 
 * All Rights Reserved
 * ATTRIBUTION ASSURANCE LICENSE (adapted from the original BSD license)
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the conditions below are met.
 * These conditions require a modest attribution to  (the
 * "Author"), who hopes that its promotional value may help justify the
 * thousands of dollars in otherwise billable time invested in writing
 * this and other freely available, open-source software.
 * 
 * 1. Redistributions of source code, in whole or part and with or without
 * modification (the "Code"), must prominently display this GPG-signed
 * text in verifiable form.
 * 2. Redistributions of the Code in binary form must be accompanied by
 * this GPG-signed text in any documentation and, each time the resulting
 * executable program or a program dependent thereon is launched, a
 * prominent display (e.g., splash screen or banner text) of the Author's
 * attribution information, which includes:
 * (a) Name ("Daniel Aarno"),
 * (b) Professional identification ("M. Sc. Electrical Engineering"), and
 * (c) URL ("http://www.nada.kth.se/~bishop").
 * 3. Neither the name nor any trademark of the Author may be used to
 * endorse or promote products derived from this software without specific
 * prior written permission.
 * 4. Users are entirely responsible, to the exclusion of the Author and
 * any other persons, for compliance with (1) regulations set by owners or
 * administrators of employed equipment, (2) licensing terms of any other
 * software, and (3) local regulations regarding use, including those
 * regarding import, export, and use of encryption software.
 * 
 * THIS FREE SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR OR ANY CONTRIBUTOR BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * EFFECTS OF UNAUTHORIZED OR MALICIOUS NETWORK ACCESS;
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1.2.4 (GNU/Linux)

iD8DBQFCHuXui6ECThHTSIkRAk9qAKCVs7kMSUtv5YhljeQsAA52EcjTFgCeNflz
w0lAUG3zeHQcJ+7t6tpce4s=
=qlVs
-----END PGP SIGNATURE-----
*/

#include "aaio.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <termios.h>

#include <sys/ioctl.h>

#ifdef HAVE_SYS_FILIO_H
//needed on solaris to get FIONREAD
#include <sys/filio.h>
#endif

#ifndef RESET_PATH
#define RESET_PATH "/usr/bin/reset"
#endif

static const int NOECHO = 0;

static void make_raw(struct termios *termios_p, int echo)
{
  int e = echo == ECHO ? 0 : ECHO;

  termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
			  |INLCR|IGNCR|ICRNL|IXON);
  termios_p->c_oflag &= ~OPOST;
  termios_p->c_lflag &= ~(ECHONL|ICANON|ISIG|IEXTEN|e);
  termios_p->c_cflag &= ~(CSIZE|PARENB);
  termios_p->c_cflag |= CS8;
}

static int set_stty_raw(struct termios *old, int echo)
{
  struct termios nxt;

  //Store old terminal setting
  if(tcgetattr(STDIN_FILENO, old))
    return -1;

  //Create a raw terminal setting with "echo"
  nxt = *old;
  make_raw(&nxt, echo);

  //Set the termianal mode
  if(tcsetattr(STDIN_FILENO, 0, &nxt))
    return -1;

  return 0;
}

//======================================================================

int getch(void)
{
  struct termios old;
  int c;

  //set raw, no-echo mode
  if(set_stty_raw(&old, NOECHO))
    return -1;

  //read a char
  c = getchar();
  
  //Reset terminal to old mode
  if(tcsetattr(STDIN_FILENO, 0, &old))
    return -1;

  return c;
}

int getche(void)
{
  struct termios old;
  int c;

  //set raw, echo mode
  if(set_stty_raw(&old, ECHO))
    return -1;

  //read a char
  c = getchar();
  
  //Reset terminal to old mode
  if(tcsetattr(STDIN_FILENO, 0, &old))
    return -1;
  
  return c;
}

int kbhit(void)
{
  struct termios old;
  int i;

  //set raw, echo mode
  if(set_stty_raw(&old, NOECHO))
    return -1;
  
  //Get number of tokens
  if(-1 == ioctl(STDIN_FILENO, FIONREAD, &i)) {
    //Reset terminal to old mode
    tcsetattr(STDIN_FILENO, 0, &old);
    return -1;
  }
  
  //Reset terminal to old mode
  if(tcsetattr(STDIN_FILENO, 0, &old))
    return -1;

  return i;
}

int aaio_hard_reset(void)
{
  if(system(RESET_PATH))
    return -1;
  
  return 0;
}

int aaio_reset(void)
{
  return 0;
}


int aaio_grant_tty_lock(void)
{
  return 0;
}

int aaio_flush(void)
{
  int n = kbhit();
  int i;

  for(i = 0; i < n; i++)
    (void)getch();

  return n;
}
