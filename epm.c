/*
 * "$Id: epm.c,v 1.4 1999/06/23 17:21:52 mike Exp $"
 *
 *   Main program source for the ESP Package Manager (EPM).
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
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/utsname.h>


/*
 * Constants...
 */

#define TAR_BLOCK	512		/* Number of bytes in a block */
#define TAR_BLOCKS	10		/* Blocking factor */

#define	TAR_MAGIC	"ustar  "	/* 7 chars and a null */

#define	TAR_OLDNORMAL	'\0'		/* Normal disk file, Unix compat */
#define	TAR_NORMAL	'0'		/* Normal disk file */
#define	TAR_LINK	'1'		/* Link to previously dumped file */
#define	TAR_SYMLINK	'2'		/* Symbolic link */
#define	TAR_CHR		'3'		/* Character special file */
#define	TAR_BLK		'4'		/* Block special file */
#define	TAR_DIR		'5'		/* Directory */
#define	TAR_FIFO	'6'		/* FIFO special file */
#define	TAR_CONTIG	'7'		/* Contiguous file */


/*
 * Structures...
 */

typedef union				/**** TAR record format ****/
{
  unsigned char	all[TAR_BLOCK];
  struct
  {
    char	pathname[100],
		mode[8],
		uid[8],
		gid[8],
		size[12],
		mtime[12],
		chksum[8],
		linkflag,
		linkname[100],
		magic[8],
		uname[32],
		gname[32],
		devmajor[8],
		devminor[8];
  }	header;
} tar_t;


/*
 * Local functions...
 */

static char	*get_line(char *buffer, int size, FILE *fp,
		          struct utsname *platform, int *skip);
static void	get_platform(struct utsname *platform);
static void	expand_name(char *buffer, char *name);
static void	usage(void);
static int	write_file(FILE *fp, char *filename);
static int	write_header(FILE *fp, char type, int mode, int size,
		             time_t mtime, char *user, char *group,
			     char *pathname, char *linkname);


/*
 * 'main()' - Read a patch listing and produce a compressed tar file.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  int		strip;		/* 1 if we should strip executables */
  int		havepatchfiles;	/* 1 if we have patch files, 0 otherwise */
  int		vernumber;	/* Version number */
  FILE		*listfile,	/* File list */
		*installfile,	/* Install script */
		*removefile,	/* Remove script */
		*swfile,	/* Distribution tar file */
		*pswfile,	/* Patch distribution tar file */
		*patchfile;	/* Patch script */
  char		prodname[256],	/* Product name */
		listname[256],	/* List file name */
		swname[255],	/* Name of distribution tar file */
		pswname[255],	/* Name of patch tar file */
		platname[255],	/* Base platform name */
		directory[255],	/* Name of install directory */
		product[256],	/* Product description */
		version[256],	/* Product version string */
		copyright[256],	/* Product copyright */
		vendor[256],	/* Vendor name */
		license[256],	/* License file to copy */
		readme[256],	/* README file to copy */
		line[10240],	/* Line from list file */
		type,		/* File type */
		dst[255],	/* Destination path */
		src[255],	/* Source path */
		tempdst[255],	/* Temporary destination before expansion */
		tempsrc[255],	/* Temporary source before expansion */
		user[32],	/* User */
		group[32],	/* Group */
		*temp;		/* Temporary pointer */
  char		command[1024];	/* Command to run */
  int		mode,		/* File permissions */
		tblocks,	/* Total number of blocks */
		skip;		/* 1 = skip files, 0 = archive files */
  time_t	deftime;	/* File creation time */
  struct stat	srcstat;	/* Source file information */
  char		padding[TAR_BLOCKS * TAR_BLOCK];
				/* Padding for tar blocks */
  struct utsname platform;	/* UNIX name info */


 /*
  * Get platform information...
  */

  get_platform(&platform);

 /*
  * Check arguments...
  */

  if (argc < 2)
    usage();

  prodname[0]  = '\0';
  listname[0]  = '\0';
  product[0]   = '\0';
  copyright[0] = '\0';
  vendor[0]    = '\0';
  license[0]   = '\0';
  readme[0]    = '\0';
  version[0]   = '\0';
  vernumber    = 0;
  strip        = 1;

  sprintf(platname, "%s-%s-%s", platform.sysname, platform.release,
          platform.machine);

  for (i = 1; i < argc; i ++)
    if (argv[i][0] == '-')
    {
     /*
      * Process a command-line option...
      */

      switch (argv[i][1])
      {
        case 'c' : /* Copyright */
	    i ++;
	    if (i >= argc)
	      usage();

            strcpy(copyright, argv[i]);
	    break;

        case 'l' : /* License */
	    i ++;
	    if (i >= argc)
	      usage();

            strcpy(license, argv[i]);
	    break;

        case 'g' : /* Don't strip */
	    strip = 0;
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

        case 'p' : /* Product name */
	    i ++;
	    if (i >= argc)
	      usage();

            strcpy(product, argv[i]);
	    break;

        case 'r' : /* Readme */
	    i ++;
	    if (i >= argc)
	      usage();

            strcpy(readme, argv[i]);
	    break;

        case 'v' : /* Version string */
	    i ++;
	    if (i >= argc)
	      usage();

            strcpy(version, argv[i]);
	    break;

        case 'V' : /* Version number */
	    i ++;
	    if (i >= argc)
	      usage();

            vernumber = atoi(argv[i]);
	    break;

        case 'e' : /* Vendor */
	    i ++;
	    if (i >= argc)
	      usage();

            strcpy(vendor, argv[i]);
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

  if (!prodname)
    usage();

  if (!listname[0])
    sprintf(listname, "%s.list", prodname);

 /*
  * Open the list file...
  */

  if ((listfile = fopen(listname, "r")) == NULL)
  {
    fprintf(stderr, "epm: Unable to open list file \"%s\" -\n          %s\n",
            listname, strerror(errno));
    return (1);
  }

 /*
  * Find any product descriptions, etc. in the list file...
  */

  puts("Searching for product information...");

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (line[0] == '%')
    {
      if (strncmp(line, "%product ", 9) == 0)
      {
        if (!product[0])
          strcpy(product, line + 9);
	else
	  fputs("epm: Ignoring %product line in list file.\n", stderr);
      }
      else if (strncmp(line, "%copyright ", 11) == 0)
      {
        if (!copyright[0])
          strcpy(copyright, line + 11);
	else
	  fputs("epm: Ignoring %copyright line in list file.\n", stderr);
      }
      else if (strncmp(line, "%vendor ", 8) == 0)
      {
        if (!vendor[0])
          strcpy(vendor, line + 8);
	else
	  fputs("epm: Ignoring %vendor line in list file.\n", stderr);
      }
      else if (strncmp(line, "%license ", 9) == 0)
      {
        if (!license[0])
          strcpy(license, line + 9);
	else
	  fputs("epm: Ignoring %license line in list file.\n", stderr);
      }
      else if (strncmp(line, "%readme ", 8) == 0)
      {
        if (!readme[0])
          strcpy(readme, line + 8);
	else
	  fputs("epm: Ignoring %readme line in list file.\n", stderr);
      }
      else if (strncmp(line, "%version ", 9) == 0)
      {
        if (!version[0])
	{
          strcpy(version, line + 9);
	  if ((temp = strchr(version, ' ')) != NULL)
	  {
	    *temp++ = '\0';
	    vernumber = atoi(temp);
	  }
	  else
	  {
	    vernumber = 0;
	    for (temp = version; *temp; temp ++)
	      if (isdigit(*temp))
	        vernumber = vernumber * 10 + *temp - '0';
          }
	}
	else
	  fputs("epm: Ignoring %version line in list file.\n", stderr);
      }
    }

  rewind(listfile);

  if (!product[0] ||
      !copyright[0] ||
      !vendor[0] ||
      !license[0] ||
      !readme[0] ||
      !version[0])
  {
    fputs("epm: Error - missing %product, %copyright, %vendor, %license,\n", stderr);
    fputs("          %readme, or %version attributes in list file!\n", stderr);

    fclose(listfile);

    return (1);
  }

 /*
  * Remove any old files that we have...
  */

  puts("Removing old product files...");

  sprintf(directory, "%s-%s-%s", prodname, version, platname);

  sprintf(command, "rm -rf %s", directory);
  system(command);
  sprintf(command, "%s.tar.gz", directory);
  unlink(command);
  sprintf(command, "%s-patch-%s-%s.tar.gz", prodname, version, platname);
  unlink(command);

 /*
  * Create the output files...
  */

  puts("Creating product files...");

  mkdir(directory, 0777);

  sprintf(swname, "%s.sw", prodname);
  sprintf(line, "%s/%s", directory, swname);

  if ((swfile = fopen(line, "wb")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n        %s\n",
            line, strerror(errno));
    return (1);
  }

  fchmod(fileno(swfile), 0444);

  sprintf(pswname, "%s.psw", prodname);
  sprintf(line, "%s/%s", directory, pswname);

  if ((pswfile = fopen(line, "wb")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n        %s\n",
            line, strerror(errno));
    return (1);
  }

  fchmod(fileno(pswfile), 0444);

  sprintf(line, "%s/%s.install", directory, prodname);

  if ((installfile = fopen(line, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n        %s\n",
            line, strerror(errno));
    return (1);
  }

  fchmod(fileno(installfile), 0555);

  sprintf(line, "%s/%s.remove", directory, prodname);

  if ((removefile = fopen(line, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n        %s\n",
            line, strerror(errno));
    return (1);
  }

  fchmod(fileno(removefile), 0555);

  sprintf(line, "%s/%s.patch", directory, prodname);

  if ((patchfile = fopen(line, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n        %s\n",
            line, strerror(errno));
    return (1);
  }

  fchmod(fileno(patchfile), 0555);

 /*
  * Copy the license and readme files...
  */

  puts("Copying license and readme files...");

  sprintf(command, "/bin/cp %s %s/%s.license", license, directory, prodname);
  system(command);
  sprintf(command, "/bin/cp %s %s/%s.readme", readme, directory, prodname);
  system(command);

 /*
  * Write the installation script header...
  */

  puts("Writing installation script preamble...");

  fputs("#!/bin/sh\n", installfile);
  fprintf(installfile, "# Installation script for %s version %s.\n", product,
          version);
  fputs("# Produced using " EPM_VERSION "; report problems to mike@easysw.com.\n",
        installfile);
  fprintf(installfile, "#%%product %s\n", product);
  fprintf(installfile, "#%%copyright %s\n", copyright);
  fprintf(installfile, "#%%version %s %d\n", version, vernumber);
  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", installfile);
  fputs("	tar=\"/bin/tar xf\"\n", installfile);
  fputs("else\n", installfile);
  fputs("	tar=\"/bin/tar xPf\"\n", installfile);
  fputs("fi\n", installfile);
  fputs("if test \"`whoami`\" != \"root\"; then\n", installfile);
  fputs("	echo \"Sorry, you must be root to install this software.\"\n", installfile);
  fputs("	exit 1\n", installfile);
  fputs("fi\n", installfile);
  fputs("if test ! \"$*\" = \"now\"; then\n", installfile);
  fprintf(installfile, "	echo \"Copyright %s\"\n", copyright);
  fputs("	echo \"\"\n", installfile);
  fprintf(installfile, "	echo \"This installation script will install the %s\"\n",
          product);
  fprintf(installfile, "	echo \"software version %s on your system.\"\n", version);
  fputs("	echo \"\"\n", installfile);
  fputs("	while true ; do\n", installfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you wish to continue? \"\n", installfile);
#else
  fputs("		echo \"Do you wish to continue? \\c\"\n", installfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", installfile);
  fputs("		case \"$yesno\" in\n", installfile);
  fputs("			y | yes | Y | Yes | YES)\n", installfile);
  fputs("			break\n", installfile);
  fputs("			;;\n", installfile);
  fputs("			n | no | N | No | NO)\n", installfile);
  fputs("			exit 0\n", installfile);
  fputs("			;;\n", installfile);
  fputs("			*)\n", installfile);
  fputs("			echo \"Please enter yes or no.\"\n", installfile);
  fputs("			;;\n", installfile);
  fputs("		esac\n", installfile);
  fputs("	done\n", installfile);
  fprintf(installfile, "	more %s.license\n", prodname);
  fputs("	echo \"\"\n", installfile);
  fputs("	while true ; do\n", installfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you agree with the terms of this license? \"\n", installfile);
#else
  fputs("		echo \"Do you agree with the terms of this license? \\c\"\n", installfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", installfile);
  fputs("		case \"$yesno\" in\n", installfile);
  fputs("			y | yes | Y | Yes | YES)\n", installfile);
  fputs("			break\n", installfile);
  fputs("			;;\n", installfile);
  fputs("			n | no | N | No | NO)\n", installfile);
  fputs("			exit 0\n", installfile);
  fputs("			;;\n", installfile);
  fputs("			*)\n", installfile);
  fputs("			echo \"Please enter yes or no.\"\n", installfile);
  fputs("			;;\n", installfile);
  fputs("		esac\n", installfile);
  fputs("	done\n", installfile);
  fputs("fi\n", installfile);
  fprintf(installfile, "if test -x " EPM_SOFTWARE "/%s.remove; then\n", prodname);
  fprintf(installfile, "	" EPM_SOFTWARE "/%s.remove now\n", prodname);
  fputs("fi\n", installfile);

  fputs("echo \"Installing software...\"\n", installfile);
  fprintf(installfile, "$tar %s.sw\n", prodname);

  fputs("if test -d " EPM_SOFTWARE "; then\n", installfile);
  fprintf(installfile, "	echo \"Removing old versions of %s software...\"\n",
          prodname);
  fprintf(installfile, "	/bin/rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);
  fputs("else\n", installfile);
  fputs("	/bin/mkdir -p " EPM_SOFTWARE "\n", installfile);
  fputs("fi\n", installfile);
  fprintf(installfile, "/bin/cp %s.remove " EPM_SOFTWARE "\n", prodname);
  fputs("echo \"Running post-installation commands...\"\n", installfile);

 /*
  * Write the patch script header...
  */

  puts("Writing patch script preamble...");

  fputs("#!/bin/sh\n", patchfile);
  fprintf(patchfile, "# Patch script for %s version %s.\n", product, version);
  fputs("# Produced using " EPM_VERSION "; report problems to mike@easysw.com.\n",
        patchfile);
  fprintf(patchfile, "#%%product %s\n", product);
  fprintf(patchfile, "#%%copyright %s\n", copyright);
  fprintf(patchfile, "#%%version %s %d\n", version, vernumber);
  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", patchfile);
  fputs("	tar=\"/bin/tar xf\"\n", patchfile);
  fputs("else\n", patchfile);
  fputs("	tar=\"/bin/tar xPf\"\n", patchfile);
  fputs("fi\n", patchfile);
  fputs("if test \"`whoami`\" != \"root\"; then\n", patchfile);
  fputs("	echo \"Sorry, you must be root to install this software.\"\n", patchfile);
  fputs("	exit 1\n", patchfile);
  fputs("fi\n", patchfile);
  fputs("if test ! \"$*\" = \"now\"; then\n", patchfile);
  fprintf(patchfile, "	echo \"Copyright %s\"\n", copyright);
  fputs("	echo \"\"\n", patchfile);
  fprintf(patchfile, "	echo \"This installation script will patch the %s\"\n",
          product);
  fprintf(patchfile, "	echo \"software to version %s on your system.\"\n", version);
  fputs("	echo \"\"\n", patchfile);
  fputs("	while true ; do\n", patchfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you wish to continue? \"\n", patchfile);
#else
  fputs("		echo \"Do you wish to continue? \\c\"\n", patchfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", patchfile);
  fputs("		case \"$yesno\" in\n", patchfile);
  fputs("			y | yes | Y | Yes | YES)\n", patchfile);
  fputs("			break\n", patchfile);
  fputs("			;;\n", patchfile);
  fputs("			n | no | N | No | NO)\n", patchfile);
  fputs("			exit 0\n", patchfile);
  fputs("			;;\n", patchfile);
  fputs("			*)\n", patchfile);
  fputs("			echo \"Please enter yes or no.\"\n", patchfile);
  fputs("			;;\n", patchfile);
  fputs("		esac\n", patchfile);
  fputs("	done\n", patchfile);
  fprintf(patchfile, "	more %s.license\n", prodname);
  fputs("	echo \"\"\n", patchfile);
  fputs("	while true ; do\n", patchfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you agree with the terms of this license? \"\n", patchfile);
#else
  fputs("		echo \"Do you agree with the terms of this license? \\c\"\n", patchfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", patchfile);
  fputs("		case \"$yesno\" in\n", patchfile);
  fputs("			y | yes | Y | Yes | YES)\n", patchfile);
  fputs("			break\n", patchfile);
  fputs("			;;\n", patchfile);
  fputs("			n | no | N | No | NO)\n", patchfile);
  fputs("			exit 0\n", patchfile);
  fputs("			;;\n", patchfile);
  fputs("			*)\n", patchfile);
  fputs("			echo \"Please enter yes or no.\"\n", patchfile);
  fputs("			;;\n", patchfile);
  fputs("		esac\n", patchfile);
  fputs("	done\n", patchfile);
  fputs("fi\n", patchfile);

  fprintf(patchfile, "if test ! -x " EPM_SOFTWARE "/%s.remove; then\n", prodname);
  fputs("	echo \"You do not appear to have the base software installed!\"\n",
        patchfile);
  fputs("	echo \"Please install the full distribution instead.\"\n", patchfile);
  fputs("	exit 1\n", patchfile);
  fputs("fi\n", patchfile);

 /*
  * Find any patch commands in the list file...
  */

  fputs("echo \"Running pre-patch commands...\"\n", patchfile);

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (strncmp(line, "%patch ", 7) == 0)
      fprintf(patchfile, "%s\n", line + 7);

  rewind(listfile);

  fputs("echo \"Patching software...\"\n", patchfile);
  fprintf(patchfile, "$tar %s.psw\n", prodname);

  fprintf(patchfile, "/bin/rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);
  fprintf(patchfile, "/bin/cp %s.remove " EPM_SOFTWARE "\n", prodname);
  fputs("echo \"Running post-installation commands...\"\n", patchfile);

 /*
  * Write the removal script header...
  */

  puts("Writing removal script preamble...");

  fputs("#!/bin/sh\n", removefile);
  fprintf(removefile, "# Removal script for %s version %s.\n", product,
          version);
  fputs("# Produced using " EPM_VERSION "; report problems to mike@easysw.com.\n",
        removefile);
  fprintf(removefile, "#%%product %s\n", product);
  fprintf(removefile, "#%%copyright %s\n", copyright);
  fprintf(removefile, "#%%version %s %d\n", version, vernumber);
  fputs("if test \"`whoami`\" != \"root\"; then\n", removefile);
  fputs("	echo \"Sorry, you must be root to remove this software.\"\n", removefile);
  fputs("	exit 1\n", removefile);
  fputs("fi\n", removefile);
  fputs("if test ! \"$*\" = \"now\"; then\n", removefile);
  fprintf(removefile, "	echo \"Copyright %s\"\n", copyright);
  fputs("	echo \"\"\n", removefile);
  fprintf(removefile, "	echo \"This removal script will remove the %s\"\n",
          product);
  fprintf(removefile, "	echo \"software version %s from your system.\"\n", version);
  fputs("	echo \"\"\n", removefile);
  fputs("	while true ; do\n", removefile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you wish to continue? \"\n", removefile);
#else
  fputs("		echo \"Do you wish to continue? \\c\"\n", removefile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", removefile);
  fputs("		case \"$yesno\" in\n", removefile);
  fputs("			y | yes | Y | Yes | YES)\n", removefile);
  fputs("			break\n", removefile);
  fputs("			;;\n", removefile);
  fputs("			n | no | N | No | NO)\n", removefile);
  fputs("			exit 0\n", removefile);
  fputs("			;;\n", removefile);
  fputs("			*)\n", removefile);
  fputs("			echo \"Please enter yes or no.\"\n", removefile);
  fputs("			;;\n", removefile);
  fputs("		esac\n", removefile);
  fputs("	done\n", removefile);
  fputs("fi\n", removefile);

 /*
  * Find any removal commands in the list file...
  */

  fputs("echo \"Running pre-removal commands...\"\n", removefile);

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (strncmp(line, "%remove ", 8) == 0)
      fprintf(removefile, "%s\n", line + 8);

  rewind(listfile);

  fputs("echo \"Removing installed files...\"\n", removefile);

 /*
  * Loop through the list file and add files...
  */

  puts("Adding product files...");

  deftime = time(NULL);
  skip    = 0;

  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
  {
    if (line[0] == '%')
    {
      if (strncmp(line, "%requires ", 10) == 0)
      {
        sscanf(line + 10, "%s", src);

        fprintf(installfile, "#%s\n", line);
        fprintf(installfile, "if test ! -f " EPM_SOFTWARE "/%s.remove; then\n", src);
	fprintf(installfile, "	echo Sorry, you must first install \\'%s\\'!\n",
	        src);
	fputs("	exit 1\n", installfile);
	fputs("fi\n", installfile);

        fprintf(patchfile, "#%s\n", line);
        fprintf(patchfile, "if test ! -f " EPM_SOFTWARE "/%s.remove; then\n", src);
	fprintf(patchfile, "	echo Sorry, you must first install \\'%s\\'!\n",
	        src);
	fputs("	exit 1\n", patchfile);
	fputs("fi\n", patchfile);
      }
      else if (strncmp(line, "%incompat ", 10) == 0)
      {
        sscanf(line + 10, "%s", src);

        fprintf(installfile, "#%s\n", line);
        fprintf(installfile, "if test -f " EPM_SOFTWARE "/%s.remove; then\n", src);
	fprintf(installfile, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	        src);
	fputs("	exit 1\n", installfile);
	fputs("fi\n", installfile);

        fprintf(patchfile, "#%s\n", line);
        fprintf(patchfile, "if test ! -f " EPM_SOFTWARE "/%s.remove; then\n",
	        line + 10);
	fprintf(patchfile, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	        line + 10);
	fputs("	exit 1\n", patchfile);
	fputs("fi\n", patchfile);
      }

      continue;
    }

    if (sscanf(line, "%c%o%s%s%s%s", &type, &mode, user, group,
               tempdst, tempsrc) < 5)
    {
      fprintf(stderr, "epm: Bad line - %s\n", line);
      continue;
    }

    expand_name(dst, tempdst);
    if (tolower(type) != 'd' && type != 'R')
      expand_name(src, tempsrc);

    switch (type)
    {
      case 'c' : /* Config file */
      case 'C' :
         /*
	  * Configuration files are extracted to the config file name with
	  * .N appended; add a bit of script magic to check if the config
	  * file already exists, and if not we move the .N to the config
	  * file location...
	  */

          fprintf(installfile, "if test ! -f %s; then\n", dst);
	  fprintf(installfile, "	/bin/mv %s.N %s\n", dst, dst);
	  fputs("fi\n", installfile);

          if (type == 'C')
	  {
            fprintf(patchfile, "if test ! -f %s; then\n", dst);
	    fprintf(patchfile, "	/bin/mv %s.N %s\n", dst, dst);
	    fputs("fi\n", patchfile);
	  }

          strcat(dst, ".N");

      case 'f' : /* Regular file */
      case 'F' :
          if ((mode & 0111) && strip)
	  {
	   /*
	    * Strip executables...
	    */

            sprintf(command, EPM_STRIP " %s 2>&1 >/dev/null", src);
	    system(command);
	  }

          if (stat(src, &srcstat))
	  {
	    fprintf(stderr, "epm: Cannot stat %s - %s\n", src,
	            strerror(errno));
	    continue;
	  }

          fprintf(removefile, "/bin/rm -f %s\n", dst);

	  printf("%s -> %s...\n", src, dst);

	  if (write_header(swfile, TAR_NORMAL, mode, srcstat.st_size,
	                   srcstat.st_mtime, user, group, dst, NULL) < 0)
	  {
	    fprintf(stderr, "epm: Error writing file header - %s\n",
	            strerror(errno));
	    return (1);
	  }

	  if (write_file(swfile, src) < 0)
	  {
	    fprintf(stderr, "epm: Error writing file data - %s\n",
	            strerror(errno));
	    return (1);
	  }

          if (isupper(type))
	  {
	    if (write_header(pswfile, TAR_NORMAL, mode, srcstat.st_size,
	                     srcstat.st_mtime, user, group, dst, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file header - %s\n",
	              strerror(errno));
	      return (1);
	    }

	    if (write_file(pswfile, src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      return (1);
	    }
	  }
	  break;

      case 'd' : /* Create directory */
      case 'D' :
	  printf("%s...\n", dst);

	  if (write_header(swfile, TAR_DIR, mode, 0, deftime, user, group,
	                   dst, NULL) < 0)
	  {
	    fprintf(stderr, "epm: Error writing directory header - %s\n",
	            strerror(errno));
	    return (1);
	  }

	  if (type == 'D')
	  {
	    if (write_header(pswfile, TAR_DIR, mode, 0, deftime, user, group,
	                     dst, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing directory header - %s\n",
	              strerror(errno));
	      return (1);
	    }
	  }
	  break;

      case 'l' : /* Link file */
      case 'L' :
          fprintf(removefile, "/bin/rm -f %s\n", dst);

	  printf("%s -> %s...\n", src, dst);

	  if (write_header(swfile, TAR_SYMLINK, mode, 0, deftime, user, group,
	                   dst, src) < 0)
	  {
	    fprintf(stderr, "epm: Error writing link header - %s\n",
	            strerror(errno));
	    return (1);
	  }

	  if (type == 'L')
	  {
	    if (write_header(pswfile, TAR_SYMLINK, mode, 0, deftime, user, group,
	                     dst, src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      return (1);
	    }
          }
	  break;

      case 'R' : /* Remove file (patch) */
          fprintf(patchfile, "/bin/rm -f %s\n", dst);
	  break;
    }
  }

  rewind(listfile);

 /*
  * Output the final zeroed record to indicate the end of file.
  */

  memset(padding, 0, sizeof(padding));

  fwrite(padding, 1, TAR_BLOCK, swfile);

  if (ftell(pswfile) > 0)
  {
    fwrite(padding, 1, TAR_BLOCK, pswfile);
    havepatchfiles = 1;
  }
  else
    havepatchfiles = 0;

 /*
  * Pad the tar files to TAR_BLOCKS blocks...  This is bullshit of course,
  * but old versions of tar need it...
  */

  tblocks = ftell(swfile) / TAR_BLOCK;
  tblocks = TAR_BLOCKS - (tblocks % TAR_BLOCKS);

  if (tblocks != TAR_BLOCKS)
    fwrite(padding, 1, tblocks * TAR_BLOCK, swfile);

  tblocks = ftell(pswfile) / TAR_BLOCK;
  tblocks = TAR_BLOCKS - (tblocks % TAR_BLOCKS);

  if (tblocks != TAR_BLOCKS)
    fwrite(padding, 1, tblocks * TAR_BLOCK, pswfile);

 /*
  * Add the "remove self" command to the removal script...
  */

  puts("Finishing removal script...");

  fprintf(removefile, "rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);

 /*
  * Find any installation commands in the list file...
  */

  puts("Finishing installation and patch scripts...");

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (strncmp(line, "%install ", 9) == 0)
    {
      fprintf(installfile, "%s\n", line + 9);
      fprintf(patchfile, "%s\n", line + 9);
    }

 /*
  * Close files...
  */

  fclose(listfile);
  fclose(swfile);
  fclose(pswfile);
  fclose(installfile);
  fclose(removefile);
  fclose(patchfile);

 /*
  * Create the distribution archives...
  */

  printf("Creating distribution archive...");
  fflush(stdout);

  sprintf(command, "/bin/tar cf %s.tar %s/%s.install %s/%s.license "
                   "%s/%s.readme %s/%s.remove %s/%s.sw", directory,
		   directory, prodname, directory, prodname,
		   directory, prodname, directory, prodname,
		   directory, prodname);
  system(command);

  sprintf(line, "%s-%s-%s.tar", prodname, version, platname);
  stat(line, &srcstat);
  if (srcstat.st_size > (1024 * 1024))
    printf(" %.1fM\n", srcstat.st_size / 1024.0 / 1024.0);
  else
    printf(" %.0fk\n", srcstat.st_size / 1024.0);

  printf("Gzipping distribution archive...");
  fflush(stdout);
  sprintf(command, "gzip -9 %s.tar", directory);
  system(command);

  sprintf(line, "%s-%s-%s.tar.gz", prodname, version, platname);
  stat(line, &srcstat);
  if (srcstat.st_size > (1024 * 1024))
    printf(" %.1fM\n", srcstat.st_size / 1024.0 / 1024.0);
  else
    printf(" %.0fk\n", srcstat.st_size / 1024.0);

 /*
  * Create the patch archives...
  */

  if (havepatchfiles)
  {
    printf("Creating patch archive...");
    fflush(stdout);
    sprintf(command, "tar cf %s-patch-%s-%s.tar %s/%s.patch "
                     "%s/%s.license %s/%s.readme %s/%s.remove %s/%s.psw",
		     prodname, version, platname,
		     directory, prodname, directory, prodname,
		     directory, prodname, directory, prodname,
		     directory, prodname);
    system(command);

    sprintf(line, "%s-patch-%s-%s.tar", prodname, version, platname);
    stat(line, &srcstat);
    if (srcstat.st_size > (1024 * 1024))
      printf(" %.1fM\n", srcstat.st_size / 1024.0 / 1024.0);
    else
      printf(" %.0fk\n", srcstat.st_size / 1024.0);

    printf("Gzipping patch archive...");
    fflush(stdout);
    sprintf(command, "gzip -9 %s-patch-%s-%s.tar", prodname, version, platname);
    system(command);

    sprintf(line, "%s-patch-%s-%s.tar.gz", prodname, version, platname);
    stat(line, &srcstat);
    if (srcstat.st_size > (1024 * 1024))
      printf(" %.1fM\n", srcstat.st_size / 1024.0 / 1024.0);
    else
      printf(" %.0fk\n", srcstat.st_size / 1024.0);
  }
  else
  {
    puts("No patch files to bundle - removing patch stuff.");

    sprintf(line, "%s/%s.patch", directory, prodname);
    unlink(line);
    sprintf(line, "%s/%s.psw", directory, prodname);
    unlink(line);
  }

 /*
  * All done!
  */

  puts("Done!");

  return (0);
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

      if (strstr(buffer + 8, platform->sysname) != NULL ||
          strcmp(buffer + 8, "all\n") == 0)
        *skip = 0;
      else
        *skip = 1;
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

#ifdef DEBUG
  printf("sysname = %s\n", platform->sysname);
  printf("release = %s\n", platform->release);
  printf("machine = %s\n", platform->machine);
#endif /* DEBUG */
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
 * 'usage()' - Show command-line usage instructions.
 */

static void
usage(void)
{
  puts("Usage: epm [-n[mrs]] [-g] [-p product-name] [-c copyright] [-l license]");
  puts("           [-r readme] [-v version] [-V vendor] product [list-file]");
  exit(1);
}


/*
 * 'write_file()' - Write the contents of a file...
 */

static int			/* O - Number of bytes written */
write_file(FILE *fp,		/* I - Tar file to write to */
           char *filename)	/* I - File to write */
{
  FILE	*file;			/* File to write */
  int	nbytes,			/* Number of bytes read */
	tbytes,			/* Total bytes read/written */
	fill;			/* Number of fill bytes needed */
  char	buffer[8192];		/* Copy buffer */


 /*
  * Try opening the file...
  */

  if ((file = fopen(filename, "rb")) == NULL)
    return (-1);

 /*
  * Copy the file to the tar file...
  */

  tbytes = 0;

  while ((nbytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
   /*
    * Zero fill the file to a 512 byte record as needed.
    */

    if (nbytes < sizeof(buffer))
    {
      fill = TAR_BLOCK - (nbytes & (TAR_BLOCK - 1));

      if (fill < TAR_BLOCK)
      {
        memset(buffer + nbytes, 0, fill);
        nbytes += fill;
      }
    }

   /*
    * Write the buffer to the file...
    */

    if (fwrite(buffer, 1, nbytes, fp) < nbytes)
    {
      fclose(file);
      return (-1);
    }

    tbytes += nbytes;
  }

 /*
  * Close the file and return...
  */

  fclose(file);
  return (tbytes);
}


/*
 * 'write_header()' - Write a TAR header for the specified file...
 */

static int			/* O - 0 on failure, 1 on success */
write_header(FILE   *fp,	/* I - Tar file to write to */
             char   type,	/* I - File type */
	     int    mode,	/* I - File permissions */
	     int    size,	/* I - File size */
             time_t mtime,	/* I - File modification time */
	     char   *user,	/* I - File owner */
	     char   *group,	/* I - File group */
	     char   *pathname,	/* I - File name */
	     char   *linkname)	/* I - File link name (for links only) */
{
  tar_t		record;		/* TAR header record */
  int		i,		/* Looping var... */
		sum;		/* Checksum */
  unsigned char	*sumptr;	/* Pointer into header record */
  struct passwd	*pwd;		/* Pointer to user record */
  struct group	*grp;		/* Pointer to group record */


 /*
  * Find the username and groupname IDs...
  */

  pwd = getpwnam(user);
  grp = getgrnam(group);

 /*
  * Format the header...
  */

  memset(&record, 0, sizeof(record));

  strcpy(record.header.pathname, pathname);
  sprintf(record.header.mode, "%-6o ", mode);
  sprintf(record.header.uid, "%o ", pwd == NULL ? 0 : pwd->pw_uid);
  sprintf(record.header.gid, "%o ", grp == NULL ? 0 : grp->gr_gid);
  sprintf(record.header.size, "%011o", size);
  sprintf(record.header.mtime, "%011o", mtime);
  memset(&(record.header.chksum), ' ', sizeof(record.header.chksum));
  record.header.linkflag = type;
  if (type == TAR_SYMLINK)
    strcpy(record.header.linkname, linkname);
  strcpy(record.header.magic, TAR_MAGIC);
  strcpy(record.header.uname, user);
  strcpy(record.header.gname, group);

 /*
  * Compute the checksum of the header...
  */

  for (i = sizeof(record), sumptr = record.all, sum = 0; i > 0; i --)
    sum += *sumptr++;

 /*
  * Put the correct checksum in place and write the header...
  */

  sprintf(record.header.chksum, "%6o", sum);

  return (fwrite(&record, 1, sizeof(record), fp));
}


/*
 * End of "$Id: epm.c,v 1.4 1999/06/23 17:21:52 mike Exp $".
 */
