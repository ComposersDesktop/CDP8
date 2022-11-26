/*
 * Copyright (c) 1983-2013 Martin Atkins and Composers Desktop Project Ltd
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.

    The CDP System is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The CDP System is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
 */

/*
 *	Declarations for wildcard library
 */

#define WILDCARD_H_RCSID "$Id: wildcard.h%v 1.1 1994/10/31 16:49:24 martin Exp $"
/*
 *	$Log: wildcard.h%v $
 * Revision 1.1  1994/10/31  16:49:24  martin
 * Initial revision
 *
 */

int iswildcard(char *filename);
void freeargv(char **);
char **wildexpand(char *filename);

#define MAXMATCH	(1000)

/*
 *	The basic matcher routines
 */
struct wildcard *wildcomp(char *);
void wildfree(struct wildcard *);
int SFCALLS wildmatch(char *, struct wildcard *);
char **wildmatchall(struct wildcard *);
