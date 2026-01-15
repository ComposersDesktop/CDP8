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
 * Copyright (c) 2004-2005 by Daniel Aarno - <macbishop@users.sf.net>
 * M. Sc. Electrical Engineering * http://www.nada.kth.se/~bishop
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * * Relicensed from Attribution Assurance License to LGPLv2 with 
 * permission from the original author (received 2025-01-09).
 */

#include "aaio.h"

#include <stdio.h>

#include <unistd.h>
#include <termios.h>

#include <sys/ioctl.h>

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

int aaio_flush(void)
{
  int n = kbhit();
  int i;

  for(i = 0; i < n; i++)
    (void)getch();

  return n;
}
