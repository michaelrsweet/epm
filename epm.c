/*
 * "$Id: epm.c,v 1.38 2000/01/04 13:45:40 mike Exp $"
 *
 *   Main program source for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2000 by Easy Software Products.
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
 *   main()         - Read a product list and produce a distribution.
 *   get_platform() - Get the operating system information...
 *   info()         - Show the EPM copyright and license.
 *   usage()        - Show command-line usage instructions.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Globals...
 */

int	Verbosity = 0;


/*
 * Local functions...
 */

static void	get_platform(struct utsname *platform);
static void	info(void);
static void	usage(void);


/*
 * 'main()' - Read a product list and produce a distribution.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  int		strip;		/* 1 if we should strip executables */
  struct utsname platform;	/* UNIX name info */
  char		platname[255],	/* Base platform name */
		prodname[256],	/* Product name */
		listname[256],	/* List file name */
		directory[255],	/* Name of install directory */
		filename[1024],	/* Name of temporary file */
		command[1024],	/* Command to run */
		*temp,		/* Temporary string pointer */
		*setup;		/* Setup GUI image */
  dist_t	*dist;		/* Software distribution */
  int		format;		/* Distribution format */
  static char	*formats[] =	/* Distribution format strings */
		{
		  "portable",
		  "deb",
		  "inst",
		  "pkg",
		  "rpm",
		  "swinstall"
		};


 /*
  * Get platform information...
  */

  get_platform(&platform);

 /*
  * Check arguments...
  */

  if (argc < 2)
    usage();

  strip  = 1;
  format = PACKAGE_PORTABLE;
  setup  = NULL;

  sprintf(platname, "%s-%s-%s", platform.sysname, platform.release,
          platform.machine);
  strcpy(directory, platname);
  prodname[0] = '\0';
  listname[0] = '\0';

  for (i = 1; i < argc; i ++)
    if (argv[i][0] == '-')
    {
     /*
      * Process a command-line option...
      */

      switch (argv[i][1])
      {
        case 'g' : /* Don't strip */
	    strip = 0;
	    break;

        case 'f' : /* Format */
	    if (argv[i][2])
	      temp = argv[i] + 2;
	    else
	    {
	      i ++;
	      if (i >= argc)
	        usage();

              temp = argv[i];
	    }

	    if (strcasecmp(temp, "portable") == 0)
	      format = PACKAGE_PORTABLE;
	    else if (strcasecmp(temp, "deb") == 0)
	      format = PACKAGE_DEB;
	    else if (strcasecmp(temp, "inst") == 0 ||
	             strcasecmp(temp, "tardist") == 0)
	      format = PACKAGE_INST;
	    else if (strcasecmp(temp, "pkg") == 0)
	      format = PACKAGE_PKG;
	    else if (strcasecmp(temp, "rpm") == 0)
	      format = PACKAGE_RPM;
	    else if (strcasecmp(temp, "swinstall") == 0 ||
	             strcasecmp(temp, "depot") == 0)
	      format = PACKAGE_SWINSTALL;
	    else
	      usage();
	    break;

        case 'n' : /* Name with sysname, machine, and/or release */
            platname[0] = '\0';

	    for (temp = argv[i] + 2; *temp != '\0'; temp ++)
	    {
	      if (platname[0])
		strcat(platname, "-");

	      if (*temp == 'm')
	        strcat(platname, platform.machine);
	      else if (*temp == 'r')
	        strcat(platname, platform.release);
	      else if (*temp == 's')
	        strcat(platname, platform.sysname);
	      else
	        usage();
	    }
	    break;

        case 's' : /* Use setup GUI */
	    if (argv[i][2])
	      setup = argv[i] + 2;
	    else
	    {
	      i ++;
	      if (i >= argc)
	        usage();

              setup = argv[i];
	    }
	    break;

        case 't' : /* Test scripts */
	    fputs("epm: Sorry, the \"test\" option is no longer available!\n",
	          stderr);
	    break;

        case 'v' : /* Be verbose */
	    Verbosity ++;
	    break;

        default :
	    usage();
	    break;
      }
    }
    else if (prodname[0] == '\0')
      strcpy(prodname, argv[i]);
    else if (listname[0] == '\0')
      strcpy(listname, argv[i]);
    else
      usage();

  if (!prodname[0])
    usage();

  if (!listname[0])
    sprintf(listname, "%s.list", prodname);

 /*
  * Show program info...
  */

  if (Verbosity)
    info();

 /*
  * Read the distribution...
  */

  if ((dist = read_dist(listname, &platform, formats[format])) == NULL)
    return (1);

 /*
  * Check that all requires info is present!
  */

  if (!dist->product[0] ||
      !dist->copyright[0] ||
      !dist->vendor[0] ||
      !dist->license[0] ||
      !dist->readme[0] ||
      !dist->version[0])
  {
    fputs("epm: Error - missing %product, %copyright, %vendor, %license,\n", stderr);
    fputs("     %readme, or %version attributes in list file!\n", stderr);

    free_dist(dist);

    return (1);
  }

  if (dist->num_files == 0)
  {
    fputs("epm: Error - no files for installation in list file!\n", stderr);

    free_dist(dist);

    return (1);
  }

 /*
  * Strip executables as needed...
  */

  if (strip)
  {
    if (Verbosity)
      puts("Stripping executables in distribution...");

    strip_execs(dist);
  }

 /*
  * Make build directory...
  */

  mkdir(directory, 0777);

 /*
  * Make the distribution in the correct format...
  */

  switch (format)
  {
    case PACKAGE_PORTABLE :
        i = make_portable(prodname, directory, platname, dist, &platform,
	                  setup);
	break;
    case PACKAGE_DEB :
        i = make_deb(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_INST :
        i = make_inst(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_PKG :
        i = make_pkg(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_RPM :
        i = make_rpm(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_SWINSTALL :
        i = make_swinstall(prodname, directory, platname, dist, &platform);
	break;
  }

 /*
  * All done!
  */

  free_dist(dist);

  if (i)
    puts("Packaging failed!");
  else if (Verbosity)
    puts("Done!");

  return (i);
}


/*
 * 'get_platform()' - Get the operating system information...
 */

static void
get_platform(struct utsname *platform)	/* O - Platform info */
{
  char	*temp;				/* Temporary pointer */


 /*
  * Get the system identification information...
  */

  uname(platform);

 /*
  * Adjust the CPU type accordingly...
  */

#ifdef __sgi
  strcpy(platform->machine, "mips");
#elif defined(__hpux)
  strcpy(platform->machine, "hppa");
#else
  for (temp = platform->machine; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

  if (strstr(platform->machine, "86") != NULL)
    strcpy(platform->machine, "intel");
  else if (strncmp(platform->machine, "sun", 3) == 0)
    strcpy(platform->machine, "sparc");
#endif /* __sgi */

 /*
  * Remove any extra junk from the beginning of the release number -
  * we just want the numbers thank you...
  */

  while (!isdigit(platform->release[0]) && platform->release[0])
    strcpy(platform->release, platform->release + 1);

 /*
  * Convert the operating system name to lowercase, and strip out
  * hyphens and underscores...
  */

  for (temp = platform->sysname; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

 /*
  * SunOS 5.x is really Solaris 2.x, and OSF1 is really Digital UNIX a.k.a.
  * Compaq Tru64 UNIX...
  */

  if (strcmp(platform->sysname, "sunos") == 0 &&
      platform->release[0] >= '5')
  {
    strcpy(platform->sysname, "solaris");
    platform->release[0] -= 3;
  }
  else if (strcmp(platform->sysname, "osf1") == 0)
    strcpy(platform->sysname, "dunix"); /* AKA Compaq Tru64 UNIX */
  else if (strcmp(platform->sysname, "irix64") == 0)
    strcpy(platform->sysname, "irix"); /* IRIX */

#ifdef DEBUG
  printf("sysname = %s\n", platform->sysname);
  printf("release = %s\n", platform->release);
  printf("machine = %s\n", platform->machine);
#endif /* DEBUG */
}


/*
 * 'info()' - Show the EPM copyright and license.
 */

static void
info(void)
{
  puts(EPM_VERSION);
  puts("Copyright 1999-2000 by Easy Software Products.");
  puts("");
  puts("EPM is free software and comes with ABSOLUTELY NO WARRANTY; for details");
  puts("see the GNU General Public License in the file COPYING or at");
  puts("\"http://www.fsf.org/gpl.html\".  Report all problems to \"epm@easysw.com\".");
  puts("");
}


/*
 * 'usage()' - Show command-line usage instructions.
 */

static void
usage(void)
{
  info();

  puts("Usage: epm [-g] [-f format] [-n[mrs]] [-v] product [list-file]");
  exit(1);
}


/*
 * End of "$Id: epm.c,v 1.38 2000/01/04 13:45:40 mike Exp $".
 */
