/*
 * "$Id: epm.c,v 1.56 2001/06/26 16:22:21 mike Exp $"
 *
 *   Main program source for the ESP Package Manager (EPM).
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
 *   main()         - Read a product list and produce a distribution.
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

int		Verbosity = 0;
int		KeepFiles = 0;
const char	*SetupProgram = EPM_LIBDIR "/setup";


/*
 * Local functions...
 */

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
  char		*namefmt,	/* Name format to use */
		platname[255],	/* Base platform name */
		prodname[256],	/* Product name */
		listname[256],	/* List file name */
		directory[255],	/* Name of install directory */
		*temp,		/* Temporary string pointer */
		*setup;		/* Setup GUI image */
  dist_t	*dist;		/* Software distribution */
  int		format;		/* Distribution format */
  static char	*formats[] =	/* Distribution format strings */
		{
		  "portable",
		  "aix",
		  "bsd",
		  "deb",
		  "inst",
		  "pkg",
		  "rpm",
		  "setld",
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

  strip       = 1;
  format      = PACKAGE_PORTABLE;
  setup       = NULL;
  namefmt     = "srm";
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
        case 'a' : /* Architecture */
	    if (argv[i][2])
	      temp = argv[i] + 2;
	    else
	    {
	      i ++;
	      if (i >= argc)
	        usage();

              temp = argv[i];
	    }

	    strncpy(platform.machine, temp, sizeof(platform.machine) - 1);
	    platform.machine[sizeof(platform.machine) - 1] = '\0';
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
	    else if (strcasecmp(temp, "aix") == 0)
	      format = PACKAGE_AIX;
	    else if (strcasecmp(temp, "bsd") == 0)
	      format = PACKAGE_BSD;
	    else if (strcasecmp(temp, "deb") == 0)
	      format = PACKAGE_DEB;
	    else if (strcasecmp(temp, "inst") == 0 ||
	             strcasecmp(temp, "tardist") == 0)
	      format = PACKAGE_INST;
	    else if (strcasecmp(temp, "pkg") == 0)
	      format = PACKAGE_PKG;
	    else if (strcasecmp(temp, "rpm") == 0)
	      format = PACKAGE_RPM;
	    else if (strcasecmp(temp, "setld") == 0)
	      format = PACKAGE_SETLD;
	    else if (strcasecmp(temp, "swinstall") == 0 ||
	             strcasecmp(temp, "depot") == 0)
	      format = PACKAGE_SWINSTALL;
	    else if (strcasecmp(temp, "native") == 0)
#if defined(__linux)
	      format = PACKAGE_RPM;
#elif defined(__sgi)
	      format = PACKAGE_INST;
#elif defined(__sun)
	      format = PACKAGE_PKG;
#elif defined(__osf__)
	      format = PACKAGE_SETLD;
#elif defined(__hpux)
	      format = PACKAGE_SWINSTALL;
#elif defined(_AIX)
              format = PACKAGE_AIX;
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	      format = PACKAGE_BSD;
#else
	      format = PACKAGE_PORTABLE;
#endif
	    else
	      usage();
	    break;

        case 'g' : /* Don't strip */
	    strip = 0;
	    break;

        case 'k' : /* Keep intermediate files */
	    KeepFiles = 1;
	    break;

        case 'n' : /* Name with sysname, machine, and/or release */
	    if (argv[i][2])
	      namefmt = argv[i] + 2;
	    else
	    {
	      i ++;
	      if (i >= argc)
	        usage();

              namefmt = argv[i];
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

        case '-' : /* --option */
	    if (strcmp(argv[i], "--keep-files") == 0)
	      KeepFiles = 1;
	    else if (strcmp(argv[i], "--setup-image") == 0)
	    {
	      i ++;
	      if (i < argc)
	        setup = argv[i];
	      else
	        usage();
            }
	    else if (strcmp(argv[i], "--setup-program") == 0)
	    {
	      i ++;
	      if (i < argc)
	        SetupProgram = argv[i];
	      else
	        usage();
            }
	    else
	      usage();
	    break;

        default :
	    usage();
	    break;
      }
    }
    else if (strchr(argv[i], '=') != NULL)
      putenv(argv[i]);
    else if (prodname[0] == '\0')
      strcpy(prodname, argv[i]);
    else if (listname[0] == '\0')
      strcpy(listname, argv[i]);
    else
      usage();

 /* 
  * Check for product name and list file...
  */

  if (!prodname[0])
    usage();

  if (!listname[0])
    snprintf(listname, sizeof(listname), "%s.list", prodname);

 /*
  * Format the build directory and platform name strings...
  */

  snprintf(directory, sizeof(directory), "%s-%s-%s", platform.sysname,
           platform.release, platform.machine);

  platname[0] = '\0';

  for (temp = namefmt; *temp != '\0'; temp ++)
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
    case PACKAGE_AIX :
        i = make_aix(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_BSD :
        i = make_bsd(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_DEB :
        if (geteuid())
	  fputs("epm: Warning - file permissions and ownership may not be correct\n"
	        "     in Debian packages unless you run EPM as root!\n", stderr);

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
    case PACKAGE_SETLD :
        if (geteuid())
	  fputs("epm: Warning - file permissions and ownership may not be correct\n"
	        "     in Tru64 packages unless you run EPM as root!\n", stderr);

        i = make_setld(prodname, directory, platname, dist, &platform);
	break;
    case PACKAGE_SWINSTALL :
        if (geteuid())
	{
	  fputs("epm: Error - HP-UX packages must be built as root!\n", stderr);
          i = 1;
	}
	else
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
 * 'info()' - Show the EPM copyright and license.
 */

static void
info(void)
{
  puts(EPM_VERSION);
  puts("Copyright 1999-2001 by Easy Software Products.");
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

  puts("Usage: epm [options] [name=value ... name=value] product [list-file]");
  puts("Options:");
  puts("-g");
  puts("    Don't strip executables in distributions.");
  puts("-f {deb,depot,inst,native,pkg,portable,rpm,setld,swinstall,tardist}");
  puts("    Set distribution format.");
  puts("-k");
  puts("    Keep intermediate files (spec files, etc.)");
  puts("-n[mrs]");
  puts("    Set distribution filename to include machine (m), OS release (r),");
  puts("    and/or OS name (s).");
  puts("-v");
  puts("    Be verbose.");
  puts("-s setup.xpm");
  puts("--setup-image setup.xpm");
  puts("    Enable the setup GUI and use \"setup.xpm\" for the setup image.");
  puts("--setup-program /foo/bar/setup");
  puts("    Use the named setup program instead of " EPM_LIBDIR "/setup.");

  exit(1);
}


/*
 * End of "$Id: epm.c,v 1.56 2001/06/26 16:22:21 mike Exp $".
 */
