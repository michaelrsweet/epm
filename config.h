/* config.h.  Generated automatically by configure.  */
/*
 * "$Id: config.h,v 1.1 1999/11/04 20:31:06 mike Exp $"
 *
 *   Configuration file for the ESP Package Manager (EPM).
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
 */

/*
 * Version of software...
 */

#define EPM_VERSION	"ESP Package Manager v2.0"

/*
 * Where are files stored?
 */

#define EPM_SOFTWARE	"/etc/software"

/*
 * What options does the strip command take?
 */

#define EPM_STRIP	"/bin/strip -f -s -k -l -h"

/*
 * Compiler stuff...
 */

/* #undef const */
#define __CHAR_UNSIGNED__ 1

/*
 * Do we have the strXXX() functions?
 */

#define HAVE_STRDUP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1

/*
 * Are we using a broken "echo" command that doesn't support the \c
 * escape (like GNU echo?)
 */

/* #undef HAVE_BROKEN_ECHO */

/*
 * Where is the "whoami" executable?
 */

#define EPM_WHOAMI	"/bin/whoami"

/*
 * Where is the "gzip" executable?
 */

#define EPM_GZIP	"/usr/sbin/gzip"

/*
 * End of "$Id: config.h,v 1.1 1999/11/04 20:31:06 mike Exp $".
 */
