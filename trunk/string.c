/*
 * "$Id: string.c,v 1.4 2001/01/19 16:16:51 mike Exp $"
 *
 *   String functions for the ESP Package Manager (EPM).
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
 *
 * Contents:
 *
 *   strdup()      - Duplicate a string.
 *   strcasecmp()  - Do a case-insensitive comparison.
 *   strncasecmp() - Do a case-insensitive comparison on up to N chars.
 */

/*
 * Include necessary headers...
 */

#include "epmstring.h"


/*
 * 'strdup()' - Duplicate a string.
 */

#  ifndef HAVE_STRDUP
char *			/* O - New string pointer */
strdup(const char *s)	/* I - String to duplicate */
{
  char	*t;		/* New string pointer */


  if (s == NULL)
    return (NULL);

  if ((t = malloc(strlen(s) + 1)) == NULL)
    return (NULL);

  return (strcpy(t, s));
}
#  endif /* !HAVE_STRDUP */


/*
 * 'strcasecmp()' - Do a case-insensitive comparison.
 */

#  ifndef HAVE_STRCASECMP
int				/* O - Result of comparison (-1, 0, or 1) */
strcasecmp(const char *s,	/* I - First string */
           const char *t)	/* I - Second string */
{
  while (*s != '\0' && *t != '\0')
  {
    if (tolower(*s) < tolower(*t))
      return (-1);
    else if (tolower(*s) > tolower(*t))
      return (1);

    s ++;
    t ++;
  }

  if (*s == '\0' && *t == '\0')
    return (0);
  else if (*s != '\0')
    return (1);
  else
    return (-1);
}
#  endif /* !HAVE_STRCASECMP */

/*
 * 'strncasecmp()' - Do a case-insensitive comparison on up to N chars.
 */

#  ifndef HAVE_STRNCASECMP
int				/* O - Result of comparison (-1, 0, or 1) */
strncasecmp(const char *s,	/* I - First string */
            const char *t,	/* I - Second string */
	    size_t     n)	/* I - Maximum number of characters to compare */
{
  while (*s != '\0' && *t != '\0' && n > 0)
  {
    if (tolower(*s) < tolower(*t))
      return (-1);
    else if (tolower(*s) > tolower(*t))
      return (1);

    s ++;
    t ++;
    n --;
  }

  if (n == 0)
    return (0);
  else if (*s == '\0' && *t == '\0')
    return (0);
  else if (*s != '\0')
    return (1);
  else
    return (-1);
}
#  endif /* !HAVE_STRNCASECMP */


/*
 * End of "$Id: string.c,v 1.4 2001/01/19 16:16:51 mike Exp $".
 */
