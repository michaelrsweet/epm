/*
 * "$Id: epmstring.h,v 1.3 2001/06/26 16:22:22 mike Exp $"
 *
 *   String definitions for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2001 by Easy Software Products.
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

#ifndef _EPM_STRING_H_
#  define _EPM_STRING_H_

/*
 * Include necessary headers...
 */

#  include "config.h"
#  include <string.h>

#  ifdef HAVE_STRINGS_H
#    include <strings.h>
#  endif /* HAVE_STRINGS_H */

#  ifdef __cplusplus
extern "C" {
#  endif /* __cplusplus */


/*
 * Prototypes...
 */

#  ifndef HAVE_STRDUP
extern char	*strdup(const char *);
#  endif /* !HAVE_STRDUP */

#  ifndef HAVE_STRCASECMP
extern int	strcasecmp(const char *, const char *);
#  endif /* !HAVE_STRCASECMP */

#  ifndef HAVE_STRNCASECMP
extern int	strncasecmp(const char *, const char *, size_t n);
#  endif /* !HAVE_STRNCASECMP */

#  ifndef HAVE_SNPRINTF
extern int	snprintf(char *, size_t, const char *, ...);
#  endif /* !HAVE_SNPRINTF */

#  ifndef HAVE_VSNPRINTF
extern int	vsnprintf(char *, size_t, const char *, va_list);
#  endif /* !HAVE_VSNPRINTF */

#  ifdef __cplusplus
}
#  endif /* __cplusplus */

#endif /* !_EPM_STRING_H_ */

/*
 * End of "$Id: epmstring.h,v 1.3 2001/06/26 16:22:22 mike Exp $".
 */
