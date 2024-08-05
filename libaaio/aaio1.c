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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#ifdef HAVE_SYS_FILIO_H
//needed on solaris to get FIONREAD
#include <sys/filio.h>
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef RESET_PATH
#define RESET_PATH "/usr/bin/reset"
#endif
#ifndef STTY_PATH
#define STTY_PATH  "/bin/stty"
#endif

#define STR_SIZE 256

#define FREE(p) free(p); (p) = NULL;

///////////////////////////////////////////////////////////////////////////////

static char g_stty_lock = 0;
static char g_echo = -1;
static char *g_stty_setting = NULL;

//////////////////////////// HELPER FUNCTIONS /////////////////////////////////

static int 
get_stty_setting(char* buf, int nmemb)
{
  int p[2]; //pipe
  int stat;
  int n;
  pid_t pid;

  errno = 0;
  pipe(p); //Create a pipe

  if((pid = fork()) == 0) { //Child process
    char *const argv[] = {"stty", "-g", NULL};
    dup2(p[1], STDOUT_FILENO); //Connect the write end of the pipe to stdout
    close(p[0]); //close read end of pipe
    close(p[1]); //close read end of pipe (now connected to stdout)

    execv(STTY_PATH, argv); //Start stty process
    exit(-1); //Should never get here
  }

  if(pid <= 0) { //Fork failed
    return -1;
  }

  close(p[1]); //Close write end of pipe
  n = read(p[0], buf, nmemb - 1); //Read the data
  close(p[0]); //Close read end of pipe
  if(n >= 0) { //If we got some data, zero terminate the string
    buf[n] = '\0';
  }
  
  if(n == nmemb - 1) { //Buffer to small
    errno = ENOMEM;
    return -1;
  }

  //Wait for child to finish
  wait(&stat);

  if(n < 0) //If read failed, return error
    return -1;

  if(!WIFEXITED(stat)) { //Child did not exit OK
    errno = ECHILD;
    return -1;
  }

  //Child exit OK, we read some data and all is well here, return exit 
  //status of child
  return WEXITSTATUS(stat);
}

static int 
set_stty_setting(const char* s)
{
  char* buf;
  int l;
	
  l = strlen(STTY_PATH);
  buf = (char*)malloc(strlen(s) + l + 1);
  (void)memcpy(buf, STTY_PATH, l);
  buf[l] = ' ';
  buf[l + 1] = '\0';
  (void)strcat(buf, s);
	
  l = system(buf);
  FREE(buf);
	
  return l;
}

static int
getch_2(void)
{
  char buf[STR_SIZE];
  int i;
	
  if(0 != get_stty_setting(buf, STR_SIZE)) {
    return -1;
  }

  //set stty to raw mode, no echo
  if(0 != set_stty_setting("raw -echo")) { 
    //if not able to set stty raw mode
    return -1;
  }
  
  i = getchar();
  if(0 != set_stty_setting(buf)) { 
    //Reset stty to original mode
    return -1;
  }

  system("stty -g > /dev/null");
  
  if(i == EOF)
    return -1;
 
  return i;
}

static int
getche_2(void)
{
  char buf[STR_SIZE];
  int i;
	
  if(0 != get_stty_setting(buf, STR_SIZE)) {
    return -1;
  }

  //set stty to raw mode, no echo
  if(0 != set_stty_setting("raw echo")) { 
    //if not able to set stty raw mode
    return -1;
  }
  
  i = getchar();
  if(0 != set_stty_setting(buf)) { 
    //Reset stty to original mode
    return -1;
  }

  if(i == EOF)
    return -1; 

  return i;
}

static int
kbhit_2(void)
{
  char buf[STR_SIZE];
  int i;
  
  if(0 != get_stty_setting(buf, STR_SIZE)) {
    return -1;
  }

  //set stty to raw mode, no echo
  if(0 != set_stty_setting("raw -echo")) { 
    //if not able to set stty raw mode
    return -1;
  }
  
  if(-1 == ioctl(STDIN_FILENO, FIONREAD, &i))
    return -1;
		
  if(0 != set_stty_setting(buf)) { 
    //Reset stty to original mode
    return -1;
  }
 
  return i;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//                         EXPORTED LIBRARY ROUTINES                         //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int 
aaio_grant_tty_lock(void)
{
  if(g_stty_lock) {
    errno = EPERM;
    return -1;
  }
	
  if(g_stty_setting == NULL) {
    g_stty_setting = (char*)malloc(STR_SIZE);
    if(g_stty_setting == NULL)
      return -1;
		
    if(0 != get_stty_setting(g_stty_setting, STR_SIZE)) {
      FREE(g_stty_setting);
      return -1;
    }
  }
	
  if(set_stty_setting("raw -echo") < 0) {
    FREE(g_stty_setting);
    return -1;
  }
	
  g_stty_lock = 1;
  g_echo = 0;
	
  return 0;
}

int 
aaio_reset(void)
{
  if(set_stty_setting(g_stty_setting))
    return -1;
		
  FREE(g_stty_setting);
  g_stty_lock = 0;
  
  return 0;
}

int 
aaio_hard_reset(void)
{
  if(system(RESET_PATH))
    return -1;
		
  FREE(g_stty_setting);
  g_stty_lock = 0;
  
  return 0;
}

int 
getch(void)
{
  if(!g_stty_lock)
    return getch_2(); //Switch modes back and forth == slower
  
  //We got the lock here, abuse the stty any way we like
  if(g_echo) { //We don't want echo
    g_echo = 0;
    if(0 != system("/bin/stty -echo")) //Disable echo
      return -1;
  }
  
  return getchar();  
}

int 
getche(void)
{
  if(!g_stty_lock)
    return getche_2(); //Switch modes back and forth == slower

  //We got the lock here, abuse the stty any way we like
  if(!g_echo) { //We want echo
    g_echo = 1;
    if(0 != system("/bin/stty echo")) //Enable echo
      return -1;
  }

  return getchar();  
}

int 
kbhit(void)
{
  int i;
  
  if(!g_stty_lock)
    return kbhit_2();

  //We got the lock here, abuse the stty any way we like
  
  if(g_echo) { //If we have echo, disable it
    g_echo = 0;
    if(0 != set_stty_setting("raw -echo")) { 
      //if not able to set stty raw, no echo mode
      return -1;
    }
  }
  
  if(-1 == ioctl(STDIN_FILENO, FIONREAD, &i))
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
