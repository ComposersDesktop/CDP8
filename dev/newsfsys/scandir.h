/*
 * Copyright (c) 1983-2013 Martin Atkins, Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
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
 *      Portable scandir for systems that don't have it!
 *
 *      These definitions should be safe if the routines
 *      are already declared
 */

#define SCANDIR_H_RCSID "$Id: scandir.h%v 1.1 1994/10/31 15:30:08 martin Exp $"
/*
 *      $Log: scandir.h%v $
 * Revision 1.1  1994/10/31  15:30:08  martin
 * Initial revision
 *
 */
#if defined(__MAC__)
#include <machine/types.h>
#endif
#if defined(unix)
#include <dirent.h>
#elif defined(_WIN32) || defined(__SC__)

struct dirent {
        char d_name[_MAX_PATH];
};

#else
#error "Unknown system"
#endif
#ifndef __MAC__
int scandir(const char *dir,
            struct dirent ***namelist,
            int (*selfn)(const struct dirent *d),
            int (*srtfn)(const struct dirent * const *d1,
                         const struct dirent * const *d2));

int alphasort(const struct dirent * const *d1,
              const struct dirent * const *d2);
#endif
