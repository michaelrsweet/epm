/*
 * "$Id: epminstall.c,v 1.1 2001/06/21 16:05:57 mike Exp $"
 *
 *   Install program replacement for the ESP Package Manager (EPM).
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
 *   main() - Add or replace files, directories, and symlinks.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include "epmstring.h"


/*
 * Local functions...
 */

static void	info(void);
static void	usage(void);


/*
 * 'main()' - Add or replace files, directories, and symlinks.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  usage();

  return (0);
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

  puts("Usage: epminstall [options] file1 file2 ... fileN directory");
  puts("       epminstall [options] file1 file2");
  puts("       epminstall [options] -d directory1 directory2 ... directoryN");
  puts("Options:");
  puts("-g group");
  puts("    Set group of installed file(s).");
  puts("-m mode");
  puts("    Set permissions of installed file(s).");
  puts("-u owner");
  puts("    Set owner of installed file(s).");

  exit(1);
}


/*
 * End of "$Id: epminstall.c,v 1.1 2001/06/21 16:05:57 mike Exp $".
 */
