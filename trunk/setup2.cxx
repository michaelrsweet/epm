//
// "$Id: setup2.cxx,v 1.27 2002/02/13 03:26:46 mike Exp $"
//
//   ESP Software Wizard main entry for the ESP Package Manager (EPM).
//
//   Copyright 1999-2001 by Easy Software Products.
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
//   main()         - Main entry for software wizard...
//   get_dists()    - Get a list of available software products.
//   install_dist() - Install a distribution...
//   list_cb()      - Handle selections in the software list.
//   load_image()   - Load the setup image file (setup.xpm)...
//   load_types()   - Load the setup image file (setup.xpm)...
//   next_cb()      - Show software selections or install software.
//   sort_dists()   - Compare two distribution names...
//   add_string()   - Add a command to an array of commands...
//   log_cb()       - Add one or more lines of text to the installation log.
//

#define _SETUP2_CXX_
#include "setup.h"
#include <FL/x.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <FL/Fl_XPM_Image.H>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifdef HAVE_SYS_MOUNT_H
#  include <sys/mount.h>
#endif // HAVE_SYS_MOUNT_H

#ifdef HAVE_SYS_STATFS_H
#  include <sys/statfs.h>
#endif // HAVE_SYS_STATFS_H

#ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
#endif // HAVE_SYS_VFS_H

#ifdef __osf__
// No prototype for statfs under Tru64...
extern "C" {
extern int statfs(const char *, struct statfs *);
}
#endif // __osf__


//
// Define a C API function type for comparisons...
//

extern "C" {
typedef int (*compare_func_t)(const void *, const void *);
}


//
// Local functions...
//

static void	log_cb(int fd, int *fdptr);
static void	update_sizes(void);


//
// 'main()' - Main entry for software wizard...
//

int			// O - Exit status
main(int  argc,		// I - Number of command-line arguments
     char *argv[])	// I - Command-line arguments
{
  Fl_Window	*w;	// Main window...


  Fl::scheme(NULL);

  if (getuid() != 0)
  {
    fl_alert("You must be logged in as root to run setup!");
    return (1);
  }

  w = make_window();

  WelcomePane->show();
  PrevButton->deactivate();

  get_installed();

  if (argc > 1)
    get_dists(argv[1]);
  else
    get_dists(".");

  load_types();
  load_image();

  w->show(1, argv);

  Fl::run();

  return (0);
}


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
// 'get_dists()' - Get a list of available software products.
//

void
get_dists(const char *d)	// I - Directory to look in
{
  int		i;		// Looping var
  int		num_files;	// Number of files
  dirent	**files;	// Files
  const char	*ext;		// Extension
  dist_t	*temp,		// Pointer to current distribution
		*installed;	// Pointer to installed product
  FILE		*fp;		// File to read from
  char		line[1024],	// Line from file...
		product[64];	// Product name...
  int		lowver, hiver;	// Version numbers for dependencies


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
      temp = add_dist(&NumDists, &Dists);

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
	  sscanf(line + 10, "%31s%d", temp->version, &(temp->vernumber));
	else if (strncmp(line, "#%rootsize ", 11) == 0)
	  temp->rootsize = atoi(line + 11);
	else if (strncmp(line, "#%usrsize ", 10) == 0)
	  temp->usrsize = atoi(line + 10);
	else if (strncmp(line, "#%incompat ", 11) == 0 ||
	         strncmp(line, "#%requires ", 11) == 0)
	{
	  lowver = 0;
	  hiver  = 0;

	  if (sscanf(line + 11, "%s%*s%d%*s%d", product, &lowver, &hiver) > 0)
	    add_depend(temp, (line[2] == 'i') ? DEPEND_INCOMPAT : DEPEND_REQUIRES,
	               product, lowver, hiver);
        }
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
    qsort(Dists, NumDists, sizeof(dist_t), (compare_func_t)sort_dists);

  for (i = 0, temp = Dists; i < NumDists; i ++, temp ++)
  {
    sprintf(line, "%s v%s", temp->name, temp->version);

    if ((installed = find_dist(temp->product, NumInstalled, Installed)) == NULL)
    {
      strcat(line, " (new)");
      SoftwareList->add(line, 0);
    }
    else if (installed->vernumber > temp->vernumber)
    {
      strcat(line, " (downgrade)");
      SoftwareList->add(line, 0);
    }
    else if (installed->vernumber == temp->vernumber)
    {
      strcat(line, " (installed)");
      SoftwareList->add(line, 0);
    }
    else
    {
      strcat(line, " (upgrade)");
      SoftwareList->add(line, 1);
    }
  }

  update_sizes();
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

  if ((num_files = filename_list(EPM_SOFTWARE, &files)) == 0)
    return;

  // Build a distribution list...
  for (i = 0; i < num_files; i ++)
  {
    ext = filename_ext(files[i]->d_name);
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

    // Set the title string...
    snprintf(command, sizeof(command), "Software License for %s", dist->name);
    LicenseLabel->label(command);

    // Load the license into the browser...
    LicenseBrowser->clear();
    LicenseBrowser->load(licfile);

    // Show the license window and wait for the user...
    LicensePane->show();
    LicenseAccept->clear();
    LicenseDecline->clear();
    NextButton->deactivate();
    CancelButton->activate();

    while (LicensePane->visible())
      Fl::wait();

    CancelButton->deactivate();
    NextButton->deactivate();

    if (LicenseDecline->value())
    {
      // Can't install without acceptance...
      liclength = 0;
      snprintf(command, sizeof(command), "License not accepted for %s!",
               dist->name);
      InstallLog->add(command);
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
  {
    // Wait for events...
    Fl::wait();

    // Check to see if the child went away...
    if (waitpid(0, &status, WNOHANG) == pid)
      break;
  }

  if (fds[0])
  {
    // Close the pipe - have all the data from the child...
    Fl::remove_fd(fds[0]);
    close(fds[0]);
  }
  else
  {
    // Get the child's exit status...
    wait(&status);
  }

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
  depend_t	*depend;


  if (SoftwareList->nchecked() == 0)
  {
    update_sizes();

    NextButton->deactivate();
    return;
  }

  for (i = 0, dist = Dists; i < NumDists; i ++, dist ++)
    if (SoftwareList->checked(i + 1))
    {
      // Check for required/incompatible products...
      for (j = 0, depend = dist->depends; j < dist->num_depends; j ++, depend ++)
        switch (depend->type)
	{
	  case DEPEND_REQUIRES :
	      if ((dist2 = find_dist(depend->product, NumDists, Dists)) != NULL)
	      {
  		// Software is in the list, is it selected?
	        k = dist2 - Dists;

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
	      else if ((dist2 = find_dist(depend->product, NumInstalled,
	                                  Installed)) == NULL)
	      {
		// Required but not installed or available!
		fl_alert("%s requires %s to be installed, but it is not available "
	        	 "for installation.", dist->name, depend->product);
		SoftwareList->checked(i + 1, 0);
		break;
	      }
	      break;

          case DEPEND_INCOMPAT :
	      if ((dist2 = find_dist(depend->product, NumInstalled,
	                             Installed)) != NULL)
	      {
		// Already installed!
		fl_alert("%s is incompatible with %s. Please remove it before "
	        	 "installing this software.", dist->name, dist2->name);
		SoftwareList->checked(i + 1, 0);
		break;
	      }
	      else if ((dist2 = find_dist(depend->product, NumDists, Dists)) != NULL)
	      {
  		// Software is in the list, is it selected?
	        k = dist2 - Dists;

		// Software is in the list, is it selected?
		if (!SoftwareList->checked(k + 1))
		  continue;

        	// Yes, tell the user...
		fl_alert("%s is incompatible with %s. Please deselect it before "
	        	 "installing this software.", dist->name, dist2->name);
		SoftwareList->checked(i + 1, 0);
		break;
	      }
	  default :
	      break;
	}
    }

  update_sizes();

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
  Fl_XPM_Image	*xpm;		// New image


  xpm = new Fl_XPM_Image("setup.xpm");

  WelcomeImage->image(xpm);
}


//
// 'load_types()' - Load the installation types from the setup.types file.
//

void
load_types(void)
{
  int		i;		// Looping var
  FILE		*fp;		// File to read from
  char		line[1024],	// Line from file
		*lineptr;	// Pointer into line
  dtype_t	*dt;		// Current install type
  dist_t	*dist;		// Distribution


  NumInstTypes = 0;

  if ((fp = fopen("setup.types", "r")) != NULL)
  {
    dt = NULL;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
      if (line[0] == '#' || line[0] == '\n')
        continue;

      lineptr = line + strlen(line) - 1;
      if (*lineptr == '\n')
        *lineptr = '\0';

      if (strncasecmp(line, "TYPE", 4) == 0 && isspace(line[4]))
      {
        // New type...
	if (NumInstTypes >= (int)(sizeof(InstTypes) / sizeof(InstTypes[0])))
	{
	  fprintf(stderr, "setup: Too many TYPEs (> %d) in setup.types!\n",
	          (int)(sizeof(InstTypes) / sizeof(InstTypes[0])));
	  fclose(fp);
	  exit(1);
	}

        for (lineptr = line + 5; isspace(*lineptr); lineptr ++);

        dt = InstTypes + NumInstTypes;
	NumInstTypes ++;

	memset(dt, 0, sizeof(dtype_t));
	strncpy(dt->label, lineptr, sizeof(dt->label) - 10);
      }
      else if (strncasecmp(line, "INSTALL", 7) == 0 && dt && isspace(line[7]))
      {
        // Install a product...
	if (dt->num_products >= (int)(sizeof(dt->products) / sizeof(dt->products[0])))
	{
	  fprintf(stderr, "setup: Too many INSTALLs (> %d) in setup.types!\n",
	          (int)(sizeof(dt->products) / sizeof(dt->products[0])));
	  fclose(fp);
	  exit(1);
	}

        for (lineptr = line + 8; isspace(*lineptr); lineptr ++);

        if ((dist = find_dist(lineptr, NumDists, Dists)) != NULL)
	{
          dt->products[dt->num_products] = dist - Dists;
	  dt->num_products ++;
	  dt->size += dist->rootsize + dist->usrsize;

          if ((dist = find_dist(lineptr, NumInstalled, Installed)) != NULL)
	    dt->size -= dist->rootsize + dist->usrsize;
	}
	else
	  fprintf(stderr, "setup: Unable to find product \"%s\" for \"%s\"!\n",
	          lineptr, dt->label);
      }
      else
      {
        fprintf(stderr, "setup: Bad line - %s\n", line);
	fclose(fp);
	exit(1);
      }
    }

    fclose(fp);
  }

  for (i = 0, dt = InstTypes; i < NumInstTypes; i ++, dt ++)
  {
    if (dt->size >= 1024)
      sprintf(dt->label + strlen(dt->label), " (+%.1fm disk space)",
              dt->size / 1024.0);
    else if (dt->size)
      sprintf(dt->label + strlen(dt->label), " (+%dk disk space)", dt->size);

    TypeButton[i]->label(dt->label);
    TypeButton[i]->show();
  }

  for (; i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])); i ++)
    TypeButton[i]->hide();
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
  static int	installing = 0;	// Installing software?


  Wizard->next();

  PrevButton->deactivate();

  if (Wizard->value() == TypePane)
  {
    if (NumInstTypes == 0)
      Wizard->next();
  }
  else if (Wizard->value() == SoftwarePane)
  {
    if (NumInstTypes)
      PrevButton->activate();

    for (i = 0; i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])); i ++)
      if (TypeButton[i]->value())
        break;

    if (i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])) &&
        InstTypes[i].num_products > 0)
      Wizard->next();
  }

  if (Wizard->value() == ConfirmPane)
  {
    ConfirmList->clear();
    PrevButton->activate();

    for (i = 0; i < NumDists; i ++)
      if (SoftwareList->checked(i + 1))
        ConfirmList->add(SoftwareList->text(i + 1));
  }
  else if (Wizard->value() == LicensePane)
    Wizard->next();

  if (Wizard->value() == InstallPane && !installing)
  {
    installing = 1;

    NextButton->deactivate();
    CancelButton->deactivate();
    CancelButton->label("Close");

    for (i = 0, progress = 0, error = 0; i < NumDists; i ++)
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

    fl_beep();

    installing = 0;
  }
  else if (Wizard->value() == SoftwarePane &&
           SoftwareList->nchecked() == 0)
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
// 'type_cb()' - Handle selections in the type list.
//

void
type_cb(Fl_Round_Button *w, void *)	// I - Radio button widget
{
  int		i;		// Looping var
  dtype_t	*dt;		// Current install type
  dist_t	*temp,		// Current software
		*installed;	// Installed software


  for (i = 0; i < (int)(sizeof(TypeButton) / sizeof(TypeButton[0])); i ++)
    if (w == TypeButton[i])
      break;

  if (i >= (int)(sizeof(TypeButton) / sizeof(TypeButton[0])))
    return;

  dt = InstTypes + i;

  SoftwareList->check_none();

  // Select all products in the install type
  for (i = 0; i < dt->num_products; i ++)
    SoftwareList->checked(dt->products[i] + 1, 1);

  // And then any upgrade products...
  for (i = 0, temp = Dists; i < NumDists; i ++, temp ++)
  {
    if ((installed = find_dist(temp->product, NumInstalled, Installed)) != NULL &&
        installed->vernumber < temp->vernumber)
      SoftwareList->checked(i + 1, 1);
  }

  update_sizes();

  NextButton->activate();
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
// 'update_size()' - Update the total +/- sizes of the installations.
//

static void
update_sizes(void)
{
  int		i;		// Looping var
  dist_t	*dist,		// Distribution
		*installed;	// Installed distribution
  int		rootsize,	// Total root size difference in kbytes
		usrsize;	// Total /usr size difference in kbytes
  struct statfs	rootpart,	// Available root partition
		usrpart;	// Available /usr partition
  int		rootfree,	// Free space on root partition
		usrfree;	// Free space on /usr partition
  static char	sizelabel[1024];// Label for selected sizes...


  // Get the sizes for the selected products...
  for (i = 0, dist = Dists, rootsize = 0, usrsize = 0;
       i < NumDists;
       i ++, dist ++)
    if (SoftwareList->checked(i + 1))
    {
      rootsize += dist->rootsize;
      usrsize  += dist->usrsize;

      if ((installed = find_dist(dist->product, NumInstalled,
                                 Installed)) != NULL)
      {
        rootsize -= installed->rootsize;
	usrsize  -= installed->usrsize;
      }
    }

  // Get the sizes of the root and /usr partition...
#if defined(__sgi) || defined(__svr4__)
  if (statfs("/", &rootpart, sizeof(rootpart), 0))
#else
  if (statfs("/", &rootpart))
#endif // __sgi || __svr4__
    rootfree = 1024;
  else
    rootfree = (int)((double)rootpart.f_bfree * (double)rootpart.f_bsize /
                     1024.0 / 1024.0 + 0.5);

#if defined(__sgi) || defined(__svr4__)
  if (statfs("/usr", &usrpart, sizeof(usrpart), 0))
#else
  if (statfs("/usr", &usrpart))
#endif // __sgi || __svr4__
    usrfree = 1024;
  else
    usrfree = (int)((double)usrpart.f_bfree * (double)usrpart.f_bsize /
                    1024.0 / 1024.0 + 0.5);

  // Display the results to the user...
  if (rootfree == usrfree)
  {
    rootsize += usrsize;

    if (rootsize >= 1024)
      snprintf(sizelabel, sizeof(sizelabel),
               "%+.1fm required, %dm available.", rootsize / 1024.0,
               rootfree);
    else
      snprintf(sizelabel, sizeof(sizelabel),
               "%+dk required, %dm available.", rootsize, rootfree);
  }
  else if (rootsize >= 1024 && usrsize >= 1024)
    snprintf(sizelabel, sizeof(sizelabel),
             "%+.1fm required on /, %dm available,\n"
             "%+.1fm required on /usr, %dm available.",
             rootsize / 1024.0, rootfree, usrsize / 1024.0, usrfree);
  else if (rootsize >= 1024)
    snprintf(sizelabel, sizeof(sizelabel),
             "%+.1fm required on /, %dm available,\n"
             "%+dk required on /usr, %dm available.",
             rootsize / 1024.0, rootfree, usrsize, usrfree);
  else if (usrsize >= 1024)
    snprintf(sizelabel, sizeof(sizelabel),
             "%+dk required on /, %dm available,\n"
             "%+.1fm required on /usr, %dm available.",
             rootsize, rootfree, usrsize / 1024.0, usrfree);
  else
    snprintf(sizelabel, sizeof(sizelabel),
             "%+dk required on /, %dm available,\n"
             "%+dk required on /usr, %dm available.",
             rootsize, rootfree, usrsize, usrfree);

  SoftwareSize->label(sizelabel);
  SoftwareSize->redraw();
}



//
// End of "$Id: setup2.cxx,v 1.27 2002/02/13 03:26:46 mike Exp $".
//
