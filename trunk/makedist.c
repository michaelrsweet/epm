/*
 * "$Id: makedist.c,v 1.5 1999/05/20 20:57:14 mike Exp $"
 *
 *   Patch file builder for espPrint, a collection of printer drivers.
 *
 *   Copyright 1993-1998 by Easy Software Products
 *
 *   These coded instructions, statements, and computer programs contain
 *   unpublished proprietary information of Easy Software Products, and
 *   are protected by Federal copyright law.  They may not be disclosed
 *   to third parties or copied or duplicated in any form, in whole or
 *   in part, without the prior written consent of Easy Software Products.
 *
 * Contents:
 *
 * Revision History:
 *
 *   $Log: makedist.c,v $
 *   Revision 1.5  1999/05/20 20:57:14  mike
 *   Fixed x86 check...
 *
 *   Revision 1.4  1999/05/20 20:50:35  mike
 *   Updated makedist for HPPA, SPARC, Intel CPUs.
 *
 *   Revision 1.3  1999/05/20 18:51:20  mike
 *   Removed extra rastertopcl filter.
 *
 *   Fixed strip and echo commands for Linux...
 *
 *   Revision 1.2  1999/05/20 16:16:47  mike
 *   Updated lp and lpadmin documentation.
 *
 *   Added startup script for CUPS.
 *
 *   Added README file for package.
 *
 *   Added missing files to CUPS package.
 *
 *   Updated makedist program.
 *
 *   Revision 1.1  1999/05/20 14:18:59  mike
 *   Added installation script generator (I'm sick of making a different
 *   distribution for every platform!)
 *
 *   Revision 1.3  1999/03/03  13:10:07  mike
 *   Wasn't adding a trailing 0 record.
 *   Updated code to use TAR_BLOCK and TAR_BLOCKS constants...
 *
 *   Revision 1.2  1998/10/07  20:54:40  mike
 *   Fixed bug in block padding code - was adding an extra 512 bytes if the
 *   file ended on an even 512-byte boundary.
 *
 *   Revision 1.1  1998/07/31  13:14:46  mike
 *   Initial revision
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#ifdef __sgi
#  define STRIP		"/bin/strip -f -s -k -l -h %s"
#else
#  define STRIP		"strip %s"
#endif /* __sgi */


/*
 * Structures...
 */

typedef union				/**** TAR record format ****/
{
  char	all[TAR_BLOCK];
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
static void	expand_name(char *buffer, char *name);
static int	write_header(FILE *fp, char type, int mode, int size,
		             time_t mtime, char *user, char *group,
			     char *pathname, char *linkname);
static int	write_file(FILE *fp, char *filename);


/*
 * 'main()' - Read a patch listing and produce a compressed tar file.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  FILE		*listfile,	/* File list */
		*installfile,	/* Install script */
		*removefile,	/* Remove script */
		*tarfile;	/* Tar file */
  char		tarname[255],	/* Name of tar file */
		directory[255],	/* Name of install directory */
		line[10240],	/* Line from list file */
		type,		/* File type */
		dst[255],	/* Destination path */
		src[255],	/* Source path */
		tempdst[255],	/* Temporary destination before expansion */
		tempsrc[255],	/* Temporary source before expansion */
		user[32],	/* User */
		group[32],	/* Group */
		name[256],	/* Product name */
		product[256],	/* Product description */
		version[256],	/* Product version */
		copyright[256],	/* Product copyright */
		vendor[256],	/* Vendor name */
		license[256],	/* License file to copy */
		readme[256],	/* README file to copy */
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
  * Check arguments...
  */

  if (argc != 4)
  {
    fputs("Usage: makedist product list-file version\n", stderr);
    return (1);
  }

  strcpy(name, argv[1]);
  strcpy(version, argv[3]);

 /*
  * Figure out the hardware platform and version...
  */

  uname(&platform);

#ifdef __sgi
  strcpy(platform.machine, "mips");
#elif defined(__hpux)
  strcpy(platform.machine, "hppa");
#else
  if (strstr(platform.machine, "86") != NULL)
    strcpy(platform.machine, "intel");
  else if (strncmp(platform.machine, "sun", 3) == 0)
    strcpy(platform.machine, "sparc");
#endif /* __sgi */

  for (temp = platform.sysname; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

  for (temp = platform.machine; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

  printf("sysname = %s\n", platform.sysname);
  printf("release = %s\n", platform.release);
  printf("machine = %s\n", platform.machine);

 /*
  * Open the list file...
  */

  if ((listfile = fopen(argv[2], "r")) == NULL)
  {
    perror("makedist: Unable to open list file!\n");
    return (1);
  }

 /*
  * Find the product description, etc. in the list file...
  */

  product[0]   = '\0';
  copyright[0] = '\0';
  vendor[0]    = '\0';
  license[0]   = '\0';
  readme[0]    = '\0';

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (line[0] == '%')
    {
      if (strncmp(line, "%product ", 9) == 0)
        strcpy(product, line + 9);
      else if (strncmp(line, "%copyright ", 11) == 0)
        strcpy(copyright, line + 11);
      else if (strncmp(line, "%vendor ", 8) == 0)
        strcpy(vendor, line + 8);
      else if (strncmp(line, "%license ", 9) == 0)
        strcpy(license, line + 9);
      else if (strncmp(line, "%readme ", 8) == 0)
        strcpy(readme, line + 8);
    }

  rewind(listfile);

  if (!product[0] ||
      !copyright[0] ||
      !vendor[0] ||
      !license[0] ||
      !readme[0])
  {
    fputs("makedist: Error - missing %product, %copyright, %vendor, %license,\n", stderr);
    fputs("          or %readme attributes in list file!\n", stderr);

    fclose(listfile);

    return (1);
  }

 /*
  * Create the output files...
  */

  sprintf(directory, "%s-%s-%s-%s-%s", name, version, platform.sysname,
          platform.release, platform.machine);

  sprintf(command, "rm -rf %s", directory);
  system(command);
  sprintf(command, "rm -f %s.tar.gz", directory);
  system(command);
  sprintf(command, "rm -f %s.tar.bz2", directory);
  system(command);

  mkdir(directory, 0777);

  sprintf(tarname, "%s.sw", argv[1]);
  sprintf(line, "%s/%s", directory, tarname);

  if ((tarfile = fopen(line, "wb")) == NULL)
  {
    perror("makedist: Unable to create tar file!\n");
    fclose(listfile);
    return (1);
  }

  sprintf(line, "%s/%s.install", directory, name);

  if ((installfile = fopen(line, "w")) == NULL)
  {
    perror("makedist: Unable to create install script!\n");
    fclose(listfile);
    fclose(tarfile);
    return (1);
  }

  fchmod(fileno(installfile), 0555);

  sprintf(line, "%s/%s.remove", directory, name);

  if ((removefile = fopen(line, "w")) == NULL)
  {
    perror("makedist: Unable to create remove script!\n");
    fclose(listfile);
    fclose(tarfile);
    fclose(installfile);
    return (1);
  }

  fchmod(fileno(removefile), 0555);

 /*
  * Copy the license and readme files...
  */

  sprintf(command, "/bin/cp %s %s/%s.license", license, directory, name);
  system(command);
  sprintf(command, "/bin/cp %s %s/%s.readme", readme, directory, name);
  system(command);

 /*
  * Write the installation script header...
  */

  fputs("#!/bin/sh\n", installfile);
  fprintf(installfile, "# Installation script for %s version %s.\n", product,
          version);
  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", installfile);
  fputs("	tar=\"/bin/tar xf\"\n", installfile);
  fputs("else\n", installfile);
  fputs("	tar=\"/bin/tar xPf\"\n", installfile);
  fputs("fi\n", installfile);
  fputs("if test \"`whoami`\" != \"root\"; then\n", installfile);
  fputs("	echo \"Sorry, you must be root to install this software.\"\n", installfile);
  fputs("	exit 1\n", installfile);
  fputs("fi\n", installfile);
  fprintf(installfile, "echo \"Copyright %s\"\n", copyright);
  fputs("echo \"\"\n", installfile);
  fprintf(installfile, "echo \"This installation script will install the %s\"\n",
          product);
  fprintf(installfile, "echo \"software version %s on your system.\"\n", version);
  fputs("echo \"\"\n", installfile);
  fputs("while true ; do\n", installfile);
#ifdef __linux	/* AND YET ANOTHER GNU BLUNDER */
  fputs("	echo -n \"Do you wish to continue? \"\n", installfile);
#else
  fputs("	echo \"Do you wish to continue? \\c\"\n", installfile);
#endif /* __linux */
  fputs("	read yesno\n", installfile);
  fputs("	case \"$yesno\" in\n", installfile);
  fputs("		y | yes | Y | Yes | YES)\n", installfile);
  fputs("		break\n", installfile);
  fputs("		;;\n", installfile);
  fputs("		n | no | N | No | NO)\n", installfile);
  fputs("		exit 0\n", installfile);
  fputs("		;;\n", installfile);
  fputs("		*)\n", installfile);
  fputs("		echo \"Please enter yes or no.\"\n", installfile);
  fputs("		;;\n", installfile);
  fputs("	esac\n", installfile);
  fputs("done\n", installfile);
  fprintf(installfile, "more %s.license\n", name);
  fputs("echo \"\"\n", installfile);
  fputs("while true ; do\n", installfile);
#ifdef __linux	/* AND YET ANOTHER GNU BLUNDER */
  fputs("	echo -n \"Do you agree with the terms of this license? \"\n", installfile);
#else
  fputs("	echo \"Do you agree with the terms of this license? \\c\"\n", installfile);
#endif /* __linux */
  fputs("	read yesno\n", installfile);
  fputs("	case \"$yesno\" in\n", installfile);
  fputs("		y | yes | Y | Yes | YES)\n", installfile);
  fputs("		break\n", installfile);
  fputs("		;;\n", installfile);
  fputs("		n | no | N | No | NO)\n", installfile);
  fputs("		exit 0\n", installfile);
  fputs("		;;\n", installfile);
  fputs("		*)\n", installfile);
  fputs("		echo \"Please enter yes or no.\"\n", installfile);
  fputs("		;;\n", installfile);
  fputs("	esac\n", installfile);
  fputs("done\n", installfile);
  fputs("echo \"Installing software...\"\n", installfile);
  fprintf(installfile, "$tar %s.sw\n", name);

 /*
  * Write the removal script header...
  */

  fputs("#!/bin/sh\n", removefile);
  fprintf(removefile, "# Removal script for %s version %s.\n", product,
          version);
  fputs("if test \"`whoami`\" != \"root\"; then\n", removefile);
  fputs("	echo \"Sorry, you must be root to remove this software.\"\n", removefile);
  fputs("	exit 1\n", removefile);
  fputs("fi\n", removefile);
  fprintf(removefile, "echo \"Copyright %s\"\n", copyright);
  fputs("echo \"\"\n", removefile);
  fprintf(removefile, "echo \"This removal script will remove the %s\"\n",
          product);
  fprintf(removefile, "echo \"software version %s from your system.\"\n", version);
  fputs("echo \"\"\n", removefile);
  fputs("while true ; do\n", removefile);
#ifdef __linux	/* AND YET ANOTHER GNU BLUNDER */
  fputs("	echo -n \"Do you wish to continue? \"\n", removefile);
#else
  fputs("	echo \"Do you wish to continue? \\c\"\n", removefile);
#endif /* __linux */
  fputs("	read yesno\n", removefile);
  fputs("	case \"$yesno\" in\n", removefile);
  fputs("		y | yes | Y | Yes | YES)\n", removefile);
  fputs("		break\n", removefile);
  fputs("		;;\n", removefile);
  fputs("		n | no | N | No | NO)\n", removefile);
  fputs("		exit 0\n", removefile);
  fputs("		;;\n", removefile);
  fputs("		*)\n", removefile);
  fputs("		echo \"Please enter yes or no.\"\n", removefile);
  fputs("		;;\n", removefile);
  fputs("	esac\n", removefile);
  fputs("done\n", removefile);

 /*
  * Find any removal commands in the list file...
  */

  fputs("echo \"Running pre-removal commands...\"\n", removefile);

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (strncmp(line, "%remove ", 8) == 0)
      fputs(line + 8, removefile);

  rewind(listfile);

  fputs("echo \"Removing installed files...\"\n", removefile);

 /*
  * Loop through the list file and add files...
  */

  deftime = time(NULL);
  tblocks = 0;
  skip    = 0;

  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
  {
    if (line[0] == '%')
      continue;

    if (sscanf(line, "%c%o%s%s%s%s", &type, &mode, user, group,
               tempdst, tempsrc) < 5)
    {
      fprintf(stderr, "makedist: Bad line - %s", line);
      continue;
    }

    expand_name(dst, tempdst);
    if (type != 'd')
      expand_name(src, tempsrc);

    switch (type)
    {
      case 'c' :
         /*
	  * Configuration files are extracted to the config file name with
	  * .N appended; add a bit of script magic to check if the config
	  * file already exists, and if not we move the .N to the config
	  * file location...
	  */

          fprintf(installfile, "if test ! -f %s; then\n", dst);
	  fprintf(installfile, "	/bin/mv %s.N %s\n", dst, dst);
	  fputs("fi\n", installfile);

          strcat(dst, ".N");

      case 'f' :
          if (mode & 0111)
	  {
	   /*
	    * Strip executables...
	    */

            sprintf(command, STRIP " 2>&1 >/dev/null", src);
	    system(command);
	  }

          if (stat(src, &srcstat))
	  {
	    fprintf(stderr, "makedist: Cannot stat %s - %s\n", src,
	            strerror(errno));
	    continue;
	  }

          fprintf(removefile, "/bin/rm -f %s\n", dst);

	  printf("%s -> %s...\n", src, dst);

	  if (write_header(tarfile, TAR_NORMAL, mode, srcstat.st_size,
	                   srcstat.st_mtime, user, group, dst, NULL) < 0)
	  {
	    fprintf(stderr, "makedist: Error writing file header - %s\n",
	            strerror(errno));
            fclose(listfile);
	    fclose(tarfile);
	    return (1);
	  }

	  if (write_file(tarfile, src) < 0)
	  {
	    fprintf(stderr, "makedist: Error writing file data - %s\n",
	            strerror(errno));
            fclose(listfile);
	    fclose(tarfile);
	    return (1);
	  }

	  tblocks += (srcstat.st_size + TAR_BLOCK - 1) / TAR_BLOCK + 1;
	  break;

      case 'd' :
	  printf("%s...\n", dst);

	  if (write_header(tarfile, TAR_DIR, mode, 0, deftime, user, group,
	                   dst, NULL) < 0)
	  {
	    fprintf(stderr, "makedist: Error writing directory header - %s\n",
	            strerror(errno));
            fclose(listfile);
	    fclose(tarfile);
	    return (1);
	  }

	  tblocks ++;
	  break;

      case 'l' :
          fprintf(removefile, "/bin/rm -f %s\n", dst);

	  printf("%s -> %s...\n", src, dst);

	  if (write_header(tarfile, TAR_SYMLINK, mode, 0, deftime, user, group,
	                   dst, src) < 0)
	  {
	    fprintf(stderr, "makedist: Error writing link header - %s\n",
	            strerror(errno));
            fclose(listfile);
	    fclose(tarfile);
	    return (1);
	  }

	  tblocks ++;
	  break;
    }
  }

  rewind(listfile);

 /*
  * Output the final zeroed record to indicate the end of file.
  */

  memset(padding, 0, sizeof(padding));

  fwrite(padding, 1, TAR_BLOCK, tarfile);
  tblocks ++;

 /*
  * Pad the tar file to TAR_BLOCKS blocks...  This is bullshit of course,
  * but old versions of tar need it...
  */

  tblocks = TAR_BLOCKS - (tblocks % TAR_BLOCKS);

  if (tblocks != TAR_BLOCKS)
    fwrite(padding, 1, tblocks * TAR_BLOCK, tarfile);

 /*
  * Find any installation commands in the list file...
  */

  fputs("echo \"Running post-installation commands...\"\n", installfile);

  skip = 0;
  while (get_line(line, sizeof(line), listfile, &platform, &skip) != NULL)
    if (strncmp(line, "%install ", 9) == 0)
      fputs(line + 8, installfile);

 /*
  * Close files...
  */

  fclose(listfile);
  fclose(tarfile);
  fclose(installfile);
  fclose(removefile);

 /*
  * Create the distribution archive...
  */

  puts("Creating distribution archive...");
  sprintf(command, "/bin/tar cvf %s.tar %s", directory, directory);
  system(command);
  puts("Gzipping distribution archive...");
  sprintf(command, "gzip -v9 <%s.tar >%s.tar.gz", directory, directory);
  system(command);
  puts("Bzipping distribution archive...");
  sprintf(command, "bzip2 -v9 %s.tar", directory);
  system(command);
  puts("Done!");

  return (0);
}


/*
 * 'get_line()' - Get a line from a file, filtering for uname lines...
 */

static char *
get_line(char           *buffer,
         int            size,
	 FILE           *fp,
	 struct utsname *platform,
	 int            *skip)
{
  while (fgets(buffer, size, fp) != NULL)
  {
   /*
    * Skip comment and blank lines...
    */

    if (buffer[0] == '#' || buffer[0] == '\n')
      continue;

   /*
    * See if this is a %sysname line...
    */

    if (strncmp(buffer, "%sysname ", 9) == 0)
    {
     /*
      * Yes, do filtering...
      */

      if (strstr(buffer + 9, platform->sysname) != NULL ||
          strcmp(buffer + 9, "all\n") == 0)
        *skip = 0;
      else
        *skip = 1;
    }
    else if (!*skip)
    {
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
 * 'write_header()' - Write a TAR header for the specified file...
 */

static int
write_header(FILE   *fp,
             char   type,
	     int    mode,
	     int    size,
             time_t mtime,
	     char   *user,
	     char   *group,
	     char   *pathname,
	     char   *linkname)
{
  tar_t		record;		/* TAR header record */
  int		i,		/* Looping var... */
		sum;		/* Checksum */
  char		*sumptr;	/* Pointer into header record */
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
 * 'write_file()' - Write the contents of a file...
 */

static int
write_file(FILE *fp,
           char *filename)
{
  FILE	*file;
  int	nbytes, tbytes, fill;
  char	buffer[8192];


  if ((file = fopen(filename, "rb")) == NULL)
    return (-1);

  tbytes = 0;

  while ((nbytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
    tbytes += nbytes;

    if (nbytes < sizeof(buffer))
    {
      fill = TAR_BLOCK - (nbytes & (TAR_BLOCK - 1));

      if (fill < TAR_BLOCK)
      {
        memset(buffer + nbytes, 0, fill);
        nbytes += fill;
      }
    }

    if (fwrite(buffer, 1, nbytes, fp) < nbytes)
    {
      fclose(file);
      return (-1);
    }
  }

  fclose(file);
  return (tbytes);
}


/*
 * End of "$Id: makedist.c,v 1.5 1999/05/20 20:57:14 mike Exp $".
 */
