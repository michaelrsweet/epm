/*
 * "$Id: dist.c,v 1.2 1999/11/05 16:52:52 mike Exp $"
 *
 *   Distribution functions for the ESP Package Manager (EPM).
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
 *   free_dist()    - Free memory used by a distribution.
 *   read_dist()    - Read a software distribution.
 *   add_string()   - Add a command to an array of commands...
 *   free_strings() - Free memory used by the array of strings.
 *   get_line()     - Get a line from a file, filtering for uname lines...
 *   expand_name()  - Expand a filename with environment variables.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	add_string(int num_strings, char ***strings, char *string);
static void	free_strings(int num_strings, char **strings);
static char	*get_line(char *buffer, int size, FILE *fp,
		          struct utsname *platform, int *skip);
static void	expand_name(char *buffer, char *name);


/*
 * 'add_file()' - Add a file to the distribution.
 */

file_t *		/* O - New file */
add_file(dist_t *dist)	/* I - Distribution */
{
  file_t	*file;	/* New file */


  if (dist->num_files == 0)
    dist->files = (file_t *)malloc(sizeof(file_t));
  else
    dist->files = (file_t *)realloc(dist->files, sizeof(file_t) *
					         (dist->num_files + 1));

  file = dist->files + dist->num_files;
  dist->num_files ++;

  return (file);
}


/*
 * 'free_dist()' - Free memory used by a distribution.
 */

void
free_dist(dist_t *dist)		/* I - Distribution to free */
{
  if (dist->num_files > 0)
    free(dist->files);

  free_strings(dist->num_descriptions, dist->descriptions);
  free_strings(dist->num_installs, dist->installs);
  free_strings(dist->num_removes, dist->removes);
  free_strings(dist->num_patches, dist->patches);
  free_strings(dist->num_incompats, dist->incompats);
  free_strings(dist->num_requires, dist->requires);

  free(dist);
}


/*
 * 'read_dist()' - Read a software distribution.
 */

dist_t *				/* O - New distribution */
read_dist(char           *filename,	/* I - Main distribution list file */
          struct utsname *platform)	/* I - Platform information */
{
  FILE		*listfiles[10];	/* File lists */
  int		listlevel;	/* Level in file list */
  char		line[10240],	/* Line from list file */
		type,		/* File type */
		dst[255],	/* Destination path */
		src[255],	/* Source path */
		tempdst[255],	/* Temporary destination before expansion */
		tempsrc[255],	/* Temporary source before expansion */
		user[32],	/* User */
		group[32],	/* Group */
		*temp;		/* Temporary pointer */
  int		mode,		/* File permissions */
		skip;		/* 1 = skip files, 0 = archive files */
  dist_t	*dist;		/* Distribution data */
  file_t	*file;		/* Distribution file */


 /*
  * Create a new, blank distribution...
  */

  dist = (dist_t *)calloc(sizeof(dist_t), 1);

 /*
  * Open the main list file...
  */

  if ((listfiles[0] = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "epm: Unable to open list file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (NULL);
  }

 /*
  * Find any product descriptions, etc. in the list file...
  */

  if (Verbosity)
    puts("Searching for product information...");

  skip      = 0;
  listlevel = 0;

  do
  {
    while (get_line(line, sizeof(line), listfiles[listlevel], platform, &skip) != NULL)
      if (line[0] == '%')
      {
       /*
        * Find whitespace...
	*/

        for (temp = line; !isspace(*temp) && *temp; temp ++);
	for (; isspace(*temp); *temp++ = '\0');

       /*
        * Process directive...
        */

	if (strcmp(line, "%include") == 0)
	{
	  listlevel ++;

	  if ((listfiles[listlevel] = fopen(temp, "r")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to include \"%s\" -\n     %s\n", temp,
	            strerror(errno));
	    listlevel --;
	  }
	}
	else if (strcmp(line, "%description") == 0)
	  dist->num_descriptions =
	      add_string(dist->num_descriptions, &(dist->descriptions), temp);
	else if (strcmp(line, "%install") == 0)
	  dist->num_installs =
	      add_string(dist->num_installs, &(dist->installs), temp);
	else if (strcmp(line, "%remove") == 0)
	  dist->num_removes =
	      add_string(dist->num_removes, &(dist->removes), temp);
	else if (strcmp(line, "%patch") == 0)
	  dist->num_patches =
	      add_string(dist->num_patches, &(dist->patches), temp);
        else if (strcmp(line, "%product") == 0)
	{
          if (!dist->product[0])
            strcpy(dist->product, temp);
	  else
	    fputs("epm: Ignoring %product line in list file.\n", stderr);
	}
	else if (strcmp(line, "%copyright") == 0)
	{
          if (!dist->copyright[0])
            strcpy(dist->copyright, temp);
	  else
	    fputs("epm: Ignoring %copyright line in list file.\n", stderr);
	}
	else if (strcmp(line, "%vendor") == 0)
	{
          if (!dist->vendor[0])
            strcpy(dist->vendor, temp);
	  else
	    fputs("epm: Ignoring %vendor line in list file.\n", stderr);
	}
	else if (strcmp(line, "%license") == 0)
	{
          if (!dist->license[0])
            strcpy(dist->license, temp);
	  else
	    fputs("epm: Ignoring %license line in list file.\n", stderr);
	}
	else if (strcmp(line, "%readme") == 0)
	{
          if (!dist->readme[0])
            strcpy(dist->readme, temp);
	  else
	    fputs("epm: Ignoring %readme line in list file.\n", stderr);
	}
	else if (strcmp(line, "%version") == 0)
	{
          if (!dist->version[0])
	  {
            strcpy(dist->version, temp);
	    if ((temp = strchr(dist->version, ' ')) != NULL)
	    {
	      *temp++ = '\0';
	      dist->vernumber = atoi(temp);
	    }
	    else
	    {
	      dist->vernumber = 0;
	      for (temp = dist->version; *temp; temp ++)
		if (isdigit(*temp))
	          dist->vernumber = dist->vernumber * 10 + *temp - '0';
            }
	  }
	}
	else if (strcmp(line, "%incompat") == 0)
	  dist->num_incompats = add_string(dist->num_incompats,
	                                   &(dist->incompats), temp);
	else if (strcmp(line, "%requires") == 0)
	  dist->num_requires = add_string(dist->num_requires, &(dist->requires),
	                                  temp);
	else
	{
	  fprintf(stderr, "epm: Unknown directive \"%s\" ignored!\n", line);
	  fprintf(stderr, "     %s %s\n", line, temp);
	}
      }
      else if (sscanf(line, "%c%o%15s%15s%254s%254s", &type, &mode, user, group,
        	      tempdst, tempsrc) < 5)
	fprintf(stderr, "epm: Bad line - %s\n", line);
      else
      {
	expand_name(dst, tempdst);
	if (tolower(type) != 'd' && type != 'R')
	  expand_name(src, tempsrc);
	else
	  src[0] = '\0';

        file = add_file(dist);

        file->type = type;
	file->mode = mode;
        strcpy(file->src, src);
	strcpy(file->dst, dst);
	strcpy(file->user, user);

#ifdef __osf__ /* Remap group "sys" to "system" */
        if (strcmp(group, "sys") == 0)
	  strcpy(group, "system");
#elif defined(__linux) /* Remap group "sys" to "root" */
        if (strcmp(group, "sys") == 0)
	  strcpy(group, "root");
#endif /* __osf__ */

	strcpy(file->group, group);
      }

    fclose(listfiles[listlevel]);
    listlevel --;
  }
  while (listlevel >= 0);

  return (dist);
}


/*
 * 'add_string()' - Add a command to an array of commands...
 */

static int			/* O - Number of strings */
add_string(int  num_strings,	/* I - Number of strings */
           char ***strings,	/* O - Pointer to string pointer array */
           char *string)	/* I - String to add */
{
  if (num_strings == 0)
    *strings = (char **)malloc(sizeof(char *));
  else
    *strings = (char **)realloc(*strings, (num_strings + 1) * sizeof(char *));

  (*strings)[num_strings] = strdup(string);
  return (num_strings + 1);
}


/*
 * 'free_strings()' - Free memory used by the array of strings.
 */

static void
free_strings(int  num_strings,	/* I - Number of strings */
             char **strings)	/* I - Pointer to string pointers */
{
  int	i;			/* Looping var */


  for (i = 0; i < num_strings; i ++)
    free(strings[i]);

  if (num_strings)
    free(strings);
}


/*
 * 'get_line()' - Get a line from a file, filtering for uname lines...
 */

static char *				/* O - String read or NULL at EOF */
get_line(char           *buffer,	/* I - Buffer to read into */
         int            size,		/* I - Size of buffer */
	 FILE           *fp,		/* I - File to read from */
	 struct utsname *platform,	/* I - Platform information */
	 int            *skip)		/* IO - Skip lines? */
{
  int	op,				/* Operation (0 = OR, 1 = AND) */
	namelen,			/* Length of system name + version */
	len,				/* Length of string */
	match;				/* 1 = match, 0 = not */
  char	*ptr,				/* Pointer into value */
	*bufptr,			/* Pointer into buffer */
	namever[255],			/* Name + version */
	value[255];			/* Value string */


  while (fgets(buffer, size, fp) != NULL)
  {
   /*
    * Skip comment and blank lines...
    */

    if (buffer[0] == '#' || buffer[0] == '\n')
      continue;

   /*
    * See if this is a %system line...
    */

    if (strncmp(buffer, "%system ", 8) == 0)
    {
     /*
      * Yes, do filtering...
      */

      *skip = 0;

      if (strcmp(buffer + 8, "all\n") != 0)
      {
	namelen = strlen(platform->sysname);
        bufptr  = buffer + 8;
	sprintf(namever, "%s-%s", platform->sysname, platform->release);

        *skip = *bufptr != '!';

        while (*bufptr)
	{
          if (*bufptr == '!')
	  {
	    op = 1;
	    bufptr ++;
	  }
	  else
	    op = 0;

	  for (ptr = value; *bufptr && !isspace(*bufptr) &&
	                        (ptr - value) < (sizeof(value) - 1);)
	    *ptr++ = *bufptr++;

	  *ptr = '\0';
	  while (isspace(*bufptr))
	    bufptr ++;

          if ((ptr = strchr(value, '-')) != NULL)
	    len = ptr - value;
	  else
	    len = strlen(value);

          if (len < namelen)
	    match = 0;
	  else
	    match = strncasecmp(value, namever, strlen(value)) == 0;

          if (op)
	    *skip |= match;
	  else
	    *skip &= !match;
        }
      }
    }
    else if (!*skip)
    {
     /*
      * Otherwise strip any trailing newlines and return the string!
      */

      if (buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';

      return (buffer);
    }
  }

  return (NULL);
}


/*
 * 'expand_name()' - Expand a filename with environment variables.
 */

static void
expand_name(char *buffer,	/* O - Output string */
            char *name)		/* I - Input string */
{
  char	var[255],		/* Environment variable name */
	*varptr;		/* Current position in name */


  while (*name != '\0')
  {
    if (*name == '$')
    {
      name ++;
      for (varptr = var; strchr("/ \t-", *name) == NULL && *name != '\0';)
        *varptr++ = *name++;

      *varptr = '\0';

      if ((varptr = getenv(var)) != NULL)
      {
        strcpy(buffer, varptr);
	buffer += strlen(buffer);
      }
    }
    else
      *buffer++ = *name++;
  }

  *buffer = '\0';
}


/*
 * End of "$Id: dist.c,v 1.2 1999/11/05 16:52:52 mike Exp $".
 */
