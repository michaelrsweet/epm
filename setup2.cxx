//
// "$Id: setup2.cxx,v 1.2 1999/12/03 22:36:54 mike Exp $"
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
#include <FL/x.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Pixmap.H>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


//
// Local functions...
//

static int	add_string(int num_strings, char ***strings, char *string);
static void	log_cb(int fd, int *fdptr);


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

  load_image();

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

    if (access(filename, X_OK))
    {
      strcat(line, " (new)");
      SoftwareList->add(line);
    }
    else
    {
      strcat(line, " (upgrade)");
      SoftwareList->add(line, 1);
    }
  }
}


//
// 'install_dist()' - Install a distribution...
//

int				// O - Install status
install_dist(const dist_t *dist)// I - Distribution to install
{
  char		command[1024];	// Command string
  int		fds[2];		// Pipe FDs
  int		status;		// Exit status
  int		pid;		// Process ID
  char		licfile[1024];	// License filename
  struct stat	licinfo;	// License file info
  static int	liclength = 0;	// Size of license file


  sprintf(command, "**** %s ****", dist->name);
  InstallLog->add(command);

  // See if we need to show the license file...
  sprintf(licfile, "%s.license", dist->product);
  if (!stat(licfile, &licinfo) && licinfo.st_size != liclength)
  {
    // Save this license's length...
    liclength = licinfo.st_size;

    // Load the license into the browser...
    LicenseBrowser->clear();
    LicenseBrowser->load(licfile);

    // Show the license window and wait for the user...
    LicenseWindow->show();
    AcceptRadio->clear();
    DeclineRadio->clear();

    while (LicenseWindow->visible())
      Fl::wait();

    if (DeclineRadio->value())
    {
      // Can't install without acceptance...
      InstallLog->add("License not accepted!");
      return (1);
    }
  }

  // Fork the command and redirect errors and info to stdout...
  pipe(fds);
  sprintf(command, "%s.install", dist->product);

  if ((pid = fork()) == 0)
  {
    // Child comes here; start by redirecting stdout and stderr...
    close(1);
    close(2);
    dup(fds[1]);
    dup(fds[1]);

    // Close the original pipes...
    close(fds[0]);
    close(fds[1]);

    // Execute the command; if an error occurs, return it...
    execl(command, command, "now", NULL);
    exit(errno);
  }
  else if (pid < 0)
  {
    // Unable to fork!
    sprintf(command, "Unable to install %s:", dist->name);
    InstallLog->add(command);

    sprintf(command, "\t%s", strerror(errno));
    InstallLog->add(command);

    close(fds[0]);
    close(fds[1]);

    return (1);
  }

  // Close the output pipe (used by the child)...
  close(fds[1]);

  // Listen for data on the input pipe...
  Fl::add_fd(fds[0], (void (*)(int, void *))log_cb, fds);

  // Loop until the child is done...
  while (fds[0])	// log_cb() will close and zero fds[0]...
    Fl::wait();

  // Get the child's exit status...
  wait(&status);

  // Return...
  return (status);
}


//
// 'list_cb()' - Handle selections in the software list.
//

void
list_cb(Fl_Check_Browser *, void *)
{
  int		i, j, k;
  dist_t	*dist,
		*dist2;
  char		filename[1024];


  if (SoftwareList->nchecked() == 0)
  {
    NextButton->deactivate();
    return;
  }

  for (i = 0, dist = Dists; i < NumDists; i ++, dist ++)
    if (SoftwareList->checked(i + 1))
    {
      // Check for requires products...
      for (j = 0; j < dist->num_requires; j ++)
      {
        // See if it is one of the products to be installed...
        for (k = 0, dist2 = Dists; k < NumDists; k ++, dist2 ++)
	  if (strcmp(dist2->product, dist->requires[j]) == 0)
	    break;

        if (k < NumDists)
	{
	  // Software is in the list, is it selected?
	  if (SoftwareList->checked(k + 1))
	    continue;

          // Nope, select it unless we're unchecked another selection...
	  if (SoftwareList->value() != (k + 1))
	    SoftwareList->checked(k + 1, 1);
	  else
	  {
	    SoftwareList->checked(i + 1, 0);
	    break;
	  }
	}
	else
	{
          // See if the requires product is already installed...
          sprintf(filename, EPM_SOFTWARE "/%s.remove", dist->requires[j]);
	  if (!access(filename, X_OK))
	    continue;

	  // Required but not installed or available!
	  fl_alert("%s requires %s to be installed, but it is not available "
	           "for installation.", dist->name, dist->requires[j]);
	  SoftwareList->checked(i + 1, 0);
	  break;
	}
      }

      // Check for incompatible products...
      for (j = 0; j < dist->num_incompats; j ++)
      {
        // See if the incompatible product is already installed...
        sprintf(filename, EPM_SOFTWARE "/%s.remove", dist->incompats[j]);
	if (!access(filename, X_OK))
	{
	  // Already installed!
	  fl_alert("%s is incompatible with %s. Please remove it before "
	           "installing this software.", dist->name, dist->incompats[j]);
	  SoftwareList->checked(i + 1, 0);
	  break;
	}

        // Then see if it is one of the products to be installed...
        for (k = 0, dist2 = Dists; k < NumDists; k ++, dist2 ++)
	  if (strcmp(dist2->product, dist->incompats[j]) == 0)
	    break;

        if (k < NumDists)
	{
	  // Software is in the list, is it selected?
	  if (!SoftwareList->checked(k + 1))
	    continue;

          // Yes, tell the user...
	  fl_alert("%s is incompatible with %s. Please deselect it before "
	           "installing this software.", dist->name, dist2->name);
	  SoftwareList->checked(i + 1, 0);
	  break;
	}
      }
    }

  if (SoftwareList->nchecked())
    NextButton->activate();
  else
    NextButton->deactivate();
}


//
// 'load_image()' - Load the setup image file (setup.xpm)...
//

void
load_image(void)
{
  FILE		*fp;		// Setup image file
  char		line[1024];	// Line from file
  Fl_Pixmap	*image;		// New image
  int		num_lines;	// Number of lines in file
  static char	*lines[1000];	// Pointer to lines


  if ((fp = fopen("setup.xpm", "r")) == NULL)
    return;

  num_lines = 0;
  while (fgets(line, sizeof(line), fp) != NULL &&
         num_lines < 1000)
  {
    if (line[0] != '\"')
      continue;

    line[strlen(line) - 3] = '\0';	/* Drop ",\n */
    lines[num_lines] = strdup(line + 1);
    num_lines ++;
  }

  fclose(fp);

  image = new Fl_Pixmap(lines);
  image->label(WelcomeImage);
}


//
// 'next_cb()' - Show software selections or install software.
//

void
next_cb(Fl_Button *, void *)
{
  int		i;		// Looping var
  int		progress;	// Progress so far...
  int		error;		// Errors?
  static char	message[1024];	// Progress message...


  Wizard->next();

  if (Wizard->value() == InstallPane)
  {
    NextButton->deactivate();
    CancelButton->deactivate();
    CancelButton->label("Close");

    for (i = 0, progress = 0; i < NumDists; i ++)
      if (SoftwareList->checked(i + 1))
      {
        sprintf(message, "Installing %s v%s...", Dists[i].name,
	        Dists[i].version);

        InstallPercent->value(100.0 * progress / SoftwareList->nchecked());
	InstallPercent->label(message);
	InstallPane->redraw();

        if ((error = install_dist(Dists + i)) != 0)
	  break;

	progress ++;
      }

    InstallPercent->value(100.0);

    if (error)
      InstallPercent->label("Installation Failed!");
    else
      InstallPercent->label("Installation Complete");

    InstallPane->redraw();

    CancelButton->activate();

    XBell(fl_display, 100);
  }
  else if (SoftwareList->nchecked() == 0)
    NextButton->deactivate();
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
// 'log_cb()' - Add one or more lines of text to the installation log.
//

static void
log_cb(int fd,			// I - Pipe to read from
       int *fdptr)		// O - Pipe to read from
{
  int		bytes;		// Bytes read/to read
  char		*bufptr;	// Pointer into buffer
  static int	bufused = 0;	// Number of bytes used
  static char	buffer[8193];	// Buffer


  bytes = 8192 - bufused;
  if ((bytes = read(fd, buffer + bufused, bytes)) <= 0)
  {
    // End of file; zero the FD to tell the install_dist() function to
    // stop...

    Fl::remove_fd(fd);
    close(fd);
    *fdptr = 0;

    if (bufused > 0)
    {
      // Add remaining text...
      buffer[bufused] = '\0';
      InstallLog->add(buffer);
      bufused = 0;
    }
  }
  else
  {
    // Add bytes to the buffer, then add lines as needed...
    bufused += bytes;
    buffer[bufused] = '\0';

    while ((bufptr = strchr(buffer, '\n')) != NULL)
    {
      *bufptr++ = '\0';
      InstallLog->add(buffer);
      strcpy(buffer, bufptr);
      bufused -= bufptr - buffer;
    }
  }

  InstallLog->bottomline(InstallLog->size());
}


//
// End of "$Id: setup2.cxx,v 1.2 1999/12/03 22:36:54 mike Exp $".
//
