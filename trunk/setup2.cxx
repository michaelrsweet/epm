//
// "$Id: setup2.cxx,v 1.1 1999/12/02 22:27:41 mike Exp $"
//
//   ESP Software Wizard main entry for the ESP Package Manager (EPM).
//
//   Copyright 1999 by Easy Software Products.
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
//

#define _SETUP2_CXX_
#include "setup.h"
#include <FL/filename.H>
#include <unistd.h>
#include <fcntl.h>


//
// Local functions...
//

static int	add_string(int num_strings, char ***strings, char *string);


//
// 'main()' - Main entry for software wizard...
//

int			// O - Exit status
main(int  argc,		// I - Number of command-line arguments
     char *argv[])	// I - Command-line arguments
{
  Fl_Window	*w;	// Main window...


  w = make_window();

  if (argc > 1)
    get_dists(argv[1]);
  else
    get_dists(".");

  w->show(1, argv);

  Fl::run();

  return (0);
}


//
// 'get_dists()' - Get a list of available software products.
//

void
get_dists(const char *d)	// I - Directory to look in
{
  int		i;		// Looping var
  int		num_files;	// Number of files
  dirent	**files;	// Files
  const char	*ext;		// Extension
  dist_t	*temp;		// Pointer to current distribution
  FILE		*fp;		// File to read from
  char		line[1024];	// Line from file...
  char		filename[1024];	// Filename


  // Get the files in the specified directory...
  if (chdir(d))
  {
    perror("setup: Unable to change to distribution directory");
    exit(1);
  }

  if ((num_files = filename_list(".", &files)) == 0)
  {
    fputs("setup: Error - no software products found!\n", stderr);
    exit(1);
  }

  // Build a distribution list...
  NumDists = 0;
  Dists    = (dist_t *)0;

  for (i = 0; i < num_files; i ++)
  {
    ext = filename_ext(files[i]->d_name);
    if (strcmp(ext, ".install") == 0)
    {
      // Found a .install script...
      if ((fp = fopen(files[i]->d_name, "r")) == NULL)
      {
        perror("setup: Unable to open installation script");
	exit(1);
      }

      // Add a new distribution entry...
      if (NumDists == 0)
        temp = (dist_t *)malloc(sizeof(dist_t));
      else
        temp = (dist_t *)realloc(Dists, sizeof(dist_t) * (NumDists + 1));

      if (temp == NULL)
      {
        perror("setup: Unable to allocate memory for distribution");
	exit(1);
      }

      Dists = temp;
      temp  += NumDists;
      NumDists ++;

      memset(temp, 0, sizeof(dist_t));
      strncpy(temp->product, files[i]->d_name, sizeof(temp->product) - 1);
      *strrchr(temp->product, '.') = '\0';	// Drop .install

      // Read info from the installation script...
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
	  sscanf(line + 10, "%31s", temp->version);
	else if (strncmp(line, "#%incompat ", 11) == 0)
	  temp->num_incompats = add_string(temp->num_incompats,
	                                   &(temp->incompats), line + 11);
	else if (strncmp(line, "#%requires ", 11) == 0)
	  temp->num_requires = add_string(temp->num_requires,
	                                   &(temp->requires), line + 11);
      }

      fclose(fp);
    }

    free(files[i]);
  }

  free(files);

  if (NumDists == 0)
  {
    fputs("setup: No distributions to install!\n", stderr);
    exit(1);
  }

  if (NumDists > 1)
    qsort(Dists, NumDists, sizeof(dist_t),
          (int (*)(const void *, const void *))sort_dists);

  for (i = 0, temp = Dists; i < NumDists; i ++, temp ++)
  {
    sprintf(filename, EPM_SOFTWARE "/%s.remove", temp->product);
    sprintf(line, "%s v%s", temp->name, temp->version);

    SoftwareList->add(line, !access(filename, X_OK));
  }
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
// 'install_dist()' - Install a distribution...
//

void
install_dist(const dist_t *dist)// I - Distribution to install
{
  char	command[1024];		// Command string
  FILE	*fp;			// Pipe to command


  sprintf(command, "./%s.install now", dist->product);
  system(command);

#if 0
  if ((fp = popen(command, "r")) == NULL)
    return;
#endif // 0
}


//
// 'add_string()' - Add a command to an array of commands...
//

static int			// O - Number of strings
add_string(int  num_strings,	// I - Number of strings
           char ***strings,	// O - Pointer to string pointer array
           char *string)	// I - String to add
{
  if (num_strings == 0)
    *strings = (char **)malloc(sizeof(char *));
  else
    *strings = (char **)realloc(*strings, (num_strings + 1) * sizeof(char *));

  (*strings)[num_strings] = strdup(string);
  return (num_strings + 1);
}


//
// End of "$Id: setup2.cxx,v 1.1 1999/12/02 22:27:41 mike Exp $".
//
