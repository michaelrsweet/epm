/*
 * "$Id: pkg.c,v 1.2 1999/11/05 16:52:52 mike Exp $"
 *
 *   AT&T package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 1999 by Easy Software Products.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 * Contents:
 *
 *   make_pkg() - Make an AT&T software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_pkg()' - Make an AT&T software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_pkg(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  puts("Sorry, AT&T PKG format is not yet supported.");
  return (1);
}


/*
 * End of "$Id: pkg.c,v 1.2 1999/11/05 16:52:52 mike Exp $".
 */
