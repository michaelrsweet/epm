//
// "$Id: gui-common.cxx,v 1.1 2003/01/30 04:27:46 mike Exp $"
//
//   ESP Software Wizard common functions for the ESP Package Manager (EPM).
//
//   Copyright 1999-2003 by Easy Software Products.
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
// Contents:
//
//   add_depend()    - Add a dependency to a distribution.
//   add_dist()      - Add a distribution.
//   find_dist()     - Find a distribution.
//   get_installed() - Get a list of installed software products.
//   sort_dists()    - Compare two distribution names...
//

#include "gui-common.h"
#include <FL/filename.H>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


//
// 'add_depend()' - Add a dependency to a distribution.
//

void
add_depend(dist_t     *d,	// I - Distribution
           int        type,	// I - Dependency type
	   const char *name,	// I - Name of product
	   int        lowver,	// I - Lower version number
	   int        hiver)	// I - Highre version number
{
  depend_t	*temp;		// Pointer to dependency


  if (d->num_depends == 0)
    temp = (depend_t *)malloc(sizeof(depend_t));
  else
    temp = (depend_t *)realloc(d->depends,
                               sizeof(depend_t) * (d->num_depends + 1));

  if (temp == NULL)
  {
    perror("setup: Unable to allocate memory for dependency");
    exit(1);
  }

  d->depends     = temp;
  temp           += d->num_depends;
  d->num_depends ++;

  memset(temp, 0, sizeof(depend_t));
  strncpy(temp->product, name, sizeof(temp->product) - 1);
  temp->type         = type;
  temp->vernumber[0] = lowver;
  temp->vernumber[1] = hiver;
}


//
// 'add_dist()' - Add a distribution.
//

dist_t *			// O - New distribution
add_dist(int    *num_d,		// IO - Number of distributions
         dist_t **d)		// IO - Distributions
{
  dist_t	*temp;		// Pointer to current distribution


  // Add a new distribution entry...
  if (*num_d == 0)
    temp = (dist_t *)malloc(sizeof(dist_t));
  else
    temp = (dist_t *)realloc(*d, sizeof(dist_t) * (*num_d + 1));

  if (temp == NULL)
  {
    perror("setup: Unable to allocate memory for distribution");
    exit(1);
  }

  *d = temp;
  temp  += *num_d;
  (*num_d) ++;

  memset(temp, 0, sizeof(dist_t));

  return (temp);
}


//
// 'find_dist()' - Find a distribution.
//

dist_t *			// O - Pointer to distribution or NULL
find_dist(const char *name,	// I - Distribution name
          int        num_d,	// I - Number of distributions
	  dist_t     *d)	// I - Distributions
{
  while (num_d > 0)
  {
    if (strcmp(name, d->product) == 0)
      return (d);

    d ++;
    num_d --;
  }

  return (NULL);
}


//
// 'get_installed()' - Get a list of installed software products.
//

void
get_installed(void)
{
  int		i;		// Looping var
  int		num_files;	// Number of files
  dirent	**files;	// Files
  const char	*ext;		// Extension
  dist_t	*temp;		// Pointer to current distribution
  FILE		*fp;		// File to read from
  char		line[1024];	// Line from file...


  // See if there are any installed files...
  NumInstalled = 0;
  Installed    = NULL;

  if ((num_files = fl_filename_list(EPM_SOFTWARE, &files)) == 0)
    return;

  // Build a distribution list...
  for (i = 0; i < num_files; i ++)
  {
    ext = fl_filename_ext(files[i]->d_name);
    if (strcmp(ext, ".remove") == 0)
    {
      // Found a .remove script...
      snprintf(line, sizeof(line), EPM_SOFTWARE "/%s", files[i]->d_name);
      if ((fp = fopen(line, "r")) == NULL)
      {
        perror("setup: Unable to open removal script");
	exit(1);
      }

      // Add a new distribution entry...
      temp = add_dist(&NumInstalled, &Installed);

      strncpy(temp->product, files[i]->d_name, sizeof(temp->product) - 1);
      *strrchr(temp->product, '.') = '\0';	// Drop .remove

      // Read info from the removal script...
      while (fgets(line, sizeof(line), fp) != NULL)
      {
        // Only read distribution info lines...
        if (strncmp(line, "#%", 2))
	  continue;

        // Drop the trailing newline...
        line[strlen(line) - 1] = '\0';

        // Copy data as needed...
	if (strncmp(line, "#%product ", 10) == 0)
	  strncpy(temp->name, line + 10, sizeof(temp->name) - 1);
	else if (strncmp(line, "#%version ", 10) == 0)
	  sscanf(line + 10, "%31s%d", temp->version, &(temp->vernumber));
	else if (strncmp(line, "#%rootsize ", 11) == 0)
	  temp->rootsize = atoi(line + 11);
	else if (strncmp(line, "#%usrsize ", 10) == 0)
	  temp->usrsize = atoi(line + 10);
      }

      fclose(fp);
    }

    free(files[i]);
  }

  free(files);
}


//
// 'sort_dists()' - Compare two distribution names...
//

int				// O - Result of comparison
sort_dists(const dist_t *d0,	// I - First distribution
           const dist_t *d1)	// I - Second distribution
{
  return (strcmp(d0->name, d1->name));
}


//
// End of "$Id: gui-common.cxx,v 1.1 2003/01/30 04:27:46 mike Exp $".
//
