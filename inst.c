/*
 * "$Id: inst.c,v 1.1 1999/11/04 20:31:07 mike Exp $"
 *
 *   IRIX package gateway for the ESP Package Manager (EPM).
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
 *   make_inst() - Make an IRIX software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_inst()' - Make an IRIX software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_inst(const char     *prodname,	/* I - Product short name */
          const char     *directory,	/* I - Directory for distribution files */
          const char     *platname,	/* I - Platform name */
          dist_t         *dist,		/* I - Distribution information */
	  struct utsname *platform)	/* I - Platform information */
{
  return (1);
}


/*
 * End of "$Id: inst.c,v 1.1 1999/11/04 20:31:07 mike Exp $".
 */
