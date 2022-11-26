/*
 * Copyright (c) 1983-2013 Composers Desktop Project Ltd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
/*
 *	osbind.h - version for porting from Atari to WindowsNT, DOS and Unix
 */
#ifndef __OSBIND_INCLUDED__
#define __OSBIND_INCLUDED__
#define OSBIND_H_RCSID "$Id: osbind.h%v 1.3 1995/10/28 17:10:20 martin Exp $"
/*
 *	$Log: osbind.h%v $
 * Revision 1.3  1995/10/28  17:10:20  martin
 * Change memory size constants to user-visible #defines
 *
 * Revision 1.2  1994/12/13  00:46:19  martin
 * Change declarations for portability of Unix
 *
 * Revision 1.1  1994/10/31  15:25:06  martin
 * Initial revision
 *
 */
/* RWD note: size needs to be a signed value, so we can't use size_t */
/* NB these decls also in sfsys.h */
void *Malloc(long size);
void Mfree(void *ptr);

unsigned int hz200(void);
unsigned int hz1000(void);
unsigned int getdrivefreespace(const char *path);
char *legalfilename(char *filename);

#if !defined(_WIN32) 
char *_fullpath(char *, const char *, size_t);
#endif

#if defined unix || defined __GNUWIN32__
int _stricmp(const char *a, const char *b);
int _strnicmp(const char *a, const char *b, size_t len);

#ifndef O_BINARY
#define O_BINARY (0)
#endif

#endif

#if defined(_WIN32)  || defined __GNUWIN32__
double drand48(void);
#endif
void initrand48(void);

#define	 MIN_CDP_MEMORY_K_BBSIZE	(100)
#define  MAX_CDP_MEMORY_K_BBSIZE 	(20 * 1024)
#define  MAX_CDP_MEMORY_BBSIZE 		(20 * 1024 * 1024)
#define  DEFAULT_CDP_MEMORY_BBSIZE	(1024*1024)

#endif	/*__OSBIND_INCLUDED__*/
