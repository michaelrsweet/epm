/*
 * "$Id: epm.c,v 1.15 1999/08/16 15:52:39 mike Exp $"
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
 *   main()          - Read a patch listing and produce a compressed tar file.
 *   add_string()    - Add a command to an array of commands...
 *   copy_file()     - Copy a file...
 *   free_dist()     - Free memory used by a distribution.
 *   free_strings()  - Free memory used by the array of strings.
 *   get_line()      - Get a line from a file, filtering for uname lines...
 *   get_platform()  - Get the operating system information...
 *   expand_name()   - Expand a filename with environment variables.
 *   read_dist()     - Read a software distribution.
 *   usage()         - Show command-line usage instructions.
 *   write_dist()    - Write a software distribution...
 *   write_file()    - Write the contents of a file...
 *   write_header()  - Write a TAR header for the specified file...
 *   write_install() - Write the installation script.
 *   write_padding() - Write the padding block(s) to the end of the
 *   write_patch()   - Write the patch script.
 *   write_remove()  - Write the removal script.
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


typedef struct				/**** File to install ****/
{
  char	type;				/* Type of file */
  int	mode;				/* Permissions of file */
  char	user[32],			/* Owner of file */
	group[32],			/* Group of file */
	src[512],			/* Source path */
	dst[512];			/* Destination path */
} file_t;

typedef struct				/**** Distribution Structure ****/
{
  char		product[256],		/* Product description */
		version[256],		/* Product version string */
		copyright[256],		/* Product copyright */
		vendor[256],		/* Vendor name */
		license[256],		/* License file to copy */
		readme[256];		/* README file to copy */
  int		vernumber;		/* Version number */
  int		num_installs;		/* Number of installation commands */
  char		**installs;		/* Installation commands */
  int		num_removes;		/* Number of removal commands */
  char		**removes;		/* Removal commands */
  int		num_patches;		/* Number of patch commands */
  char		**patches;		/* Patch commands */
  int		num_incompats;		/* Number of incompatible products */
  char		**incompats;		/* Incompatible products */
  int		num_requires;		/* Number of requires products */
  char		**requires;		/* Required products */
  int		num_files;		/* Number of files */
  file_t	*files;			/* Files */
} dist_t;


/*
 * Local functions...
 */

static int	add_string(int num_strings, char ***strings, char *string);
static int	copy_file(char *dst, char *src);
static void	free_strings(int num_strings, char **strings);
static void	free_dist(dist_t *dist);
static char	*get_line(char *buffer, int size, FILE *fp,
		          struct utsname *platform, int *skip);
static void	get_platform(struct utsname *platform);
static void	expand_name(char *buffer, char *name);
static dist_t	*read_dist(char *filename, struct utsname *platform);
static void	usage(void);
static int	write_dist(char *title, char *directory, char *prodname,
		           char *platname, dist_t *dist, char **files);
static int	write_file(FILE *fp, char *filename);
static int	write_header(FILE *fp, char type, int mode, int size,
		             time_t mtime, char *user, char *group,
			     char *pathname, char *linkname);
static int	write_install(dist_t *dist, char *prodname, char *directory);
static int	write_padding(FILE *fp, int blocks);
static int	write_patch(dist_t *dist, char *prodname, char *directory);
static int	write_remove(dist_t *dist, char *prodname, char *directory);


/*
 * 'main()' - Read a patch listing and produce a compressed tar file.
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  int		strip;		/* 1 if we should strip executables */
  int		test;		/* 1 if we should not make archives (test scripts) */
  int		havepatchfiles;	/* 1 if we have patch files, 0 otherwise */
  FILE		*tarfile;	/* Distribution tar file */
  char		prodname[256],	/* Product name */
		listname[256],	/* List file name */
		swname[255],	/* Name of distribution tar file */
		pswname[255],	/* Name of patch tar file */
		platname[255],	/* Base platform name */
		directory[255],	/* Name of install directory */
		filename[1024],	/* Name of temporary file */
		command[1024],	/* Command to run */
		*temp;		/* Temporary string pointer */
  int		blocks,		/* Blocks in this file */
		tblocks;	/* Total number of blocks */
  time_t	deftime;	/* File creation time */
  struct stat	srcstat;	/* Source file information */
  struct utsname platform;	/* UNIX name info */
  dist_t	*dist;		/* Software distribution */
  file_t	*file;		/* Software file */
  static char	*distfiles[] =	/* Distribution files */
		{
		  "install",
		  "license",
		  "readme",
		  "remove",
		  "sw",
		  NULL
		};
  static char	*patchfiles[] =	/* Patch files */
		{
		  "license",
		  "patch",
		  "psw",
		  "readme",
		  "remove",
		  NULL
		};


 /*
  * Show program info...
  */

  puts(EPM_VERSION);
  puts("Copyright 1999 by Easy Software Products.");
  puts("");
  puts("EPM is free software and comes with ABSOLUTELY NO WARRANTY; for details");
  puts("see the GNU General Public License in the file COPYING or at");
  puts("\"http://www.fsf.org/gpl.html\".  Report all problems to \"epm@easysw.com\".");
  puts("");

 /*
  * Get platform information...
  */

  get_platform(&platform);

 /*
  * Check arguments...
  */

  if (argc < 2)
    usage();

  deftime = time(NULL);
  strip   = 1;
  test    = 0;

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

        case 't' : /* Test scripts */
	    test = 1;
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
  * Read the distribution...
  */

  if ((dist = read_dist(listname, &platform)) == NULL)
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
  * See if we need to make a patch distribution...
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (isupper(file->type))
      break;

  havepatchfiles = i > 0;

 /*
  * Make build directory...
  */

  mkdir(directory, 0777);

 /*
  * Copy the license and readme files...
  */

  puts("Copying license and readme files...");

  sprintf(filename, "%s/%s.license", directory, prodname);
  if (copy_file(filename, dist->license))
    return (1);

  sprintf(filename, "%s/%s.readme", directory, prodname);
  if (copy_file(filename, dist->readme))
    return (1);

 /*
  * Create the scripts...
  */

  if (write_install(dist, prodname, directory))
    return (1);

  if (havepatchfiles)
    if (write_patch(dist, prodname, directory))
      return (1);

  if (write_remove(dist, prodname, directory))
    return (1);

 /*
  * Create the software distribution file...
  */

  puts("Creating software distribution file...");

  sprintf(swname, "%s.sw", prodname);
  sprintf(filename, "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = fopen(filename, "wb")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  fchmod(fileno(tarfile), 0444);
  tblocks = 0;

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    switch (tolower(file->type))
    {
      case 'c' : /* Config file */
      case 'f' : /* Regular file */
          if ((file->mode & 0111) && strip)
	  {
	   /*
	    * Strip executables...
	    */

            sprintf(command, EPM_STRIP " %s 2>&1 >/dev/null", file->src);
	    system(command);
	  }

          if (stat(file->src, &srcstat))
	  {
	    fprintf(stderr, "epm: Cannot stat %s - %s\n", file->src,
	            strerror(errno));
	    continue;
	  }

         /*
	  * Configuration files are extracted to the config file name with
	  * .N appended; add a bit of script magic to check if the config
	  * file already exists, and if not we move the .N to the config
	  * file location...
	  */

          strcpy(filename, file->dst);
	  if (tolower(file->type) == 'c')
            strcat(filename, ".N");

	  printf("%s -> %s...\n", file->src, filename);

	  if (write_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                   srcstat.st_mtime, file->user, file->group,
			   filename, NULL) < 0)
	  {
	    fprintf(stderr, "epm: Error writing file header - %s\n",
	            strerror(errno));
	    return (1);
	  }

	  if ((blocks = write_file(tarfile, file->src)) < 0)
	  {
	    fprintf(stderr, "epm: Error writing file data - %s\n",
	            strerror(errno));
	    return (1);
	  }

	  tblocks += blocks + 1;
	  break;

      case 'd' : /* Create directory */
	  printf("%s...\n", file->dst);
	  break;

      case 'l' : /* Link file */
	  printf("%s -> %s...\n", file->src, file->dst);

	  if (write_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	  {
	    fprintf(stderr, "epm: Error writing link header - %s\n",
	            strerror(errno));
	    return (1);
	  }

	  tblocks ++;
	  break;
    }

  write_padding(tarfile, tblocks);
  fclose(tarfile);

 /*
  * Create the patch distribution file...
  */

  if (havepatchfiles)
  {
    puts("Creating software patch file...");

    sprintf(pswname, "%s.psw", prodname);
    sprintf(filename, "%s/%s", directory, pswname);

    unlink(filename);
    if ((tarfile = fopen(filename, "wb")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
              filename, strerror(errno));
      return (1);
    }

    fchmod(fileno(tarfile), 0444);
    tblocks = 0;

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      switch (file->type)
      {
	case 'C' : /* Config file */
	case 'F' : /* Regular file */
            if (stat(file->src, &srcstat))
	    {
	      fprintf(stderr, "epm: Cannot stat %s - %s\n", file->src,
	              strerror(errno));
	      continue;
	    }

           /*
	    * Configuration files are extracted to the config file name with
	    * .N appended; add a bit of script magic to check if the config
	    * file already exists, and if not we move the .N to the config
	    * file location...
	    */

            strcpy(filename, file->dst);
	    if (file->type == 'C')
              strcat(filename, ".N");

	    printf("%s -> %s...\n", file->src, filename);

	    if (write_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                     srcstat.st_mtime, file->user, file->group,
			     filename, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file header - %s\n",
	              strerror(errno));
	      return (1);
	    }

	    if ((blocks = write_file(tarfile, file->src)) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      return (1);
	    }

	    tblocks += blocks + 1;
	    break;

	case 'd' : /* Create directory */
	    printf("%s...\n", file->dst);
	    break;

	case 'l' : /* Link file */
	    printf("%s -> %s...\n", file->src, file->dst);

	    if (write_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                     file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      return (1);
	    }

	    tblocks ++;
	    break;
      }

    write_padding(tarfile, tblocks);
    fclose(tarfile);
  }

 /*
  * Create the distribution archives...
  */

  if (test)
    puts("Skipping archive generation because \"-t\" option was used.");
  else
  {
    write_dist("distribution", directory, prodname, platname, dist, distfiles);

    if (havepatchfiles)
    {
      sprintf(filename, "patch-%s", dist->version);
      strcpy(dist->version, filename);

      write_dist("patch", directory, prodname, platname, dist, patchfiles);
    }
  }

 /*
  * All done!
  */

  free_dist(dist);

  puts("Done!");

  return (0);
}


/*
 * 'add_string()' - Add a command to an array of commands...
 */

static int
add_string(int  num_strings,
           char ***strings,
           char *string)
{
  if (num_strings == 0)
    *strings = (char **)malloc(sizeof(char *));
  else
    *strings = (char **)realloc(*strings, (num_strings + 1) * sizeof(char *));

  (*strings)[num_strings] = strdup(string);
  return (num_strings + 1);
}


/*
 * 'copy_file()' - Copy a file...
 */

static int		/* O - 0 on success, -1 on failure */
copy_file(char *dst,	/* I - Destination file */
          char *src)	/* I - Source file */
{
  FILE	*dstfile,	/* Destination file */
	*srcfile;	/* Source file */
  char	buffer[8192];	/* Copy buffer */
  int	bytes;		/* Number of bytes read/written */


 /*
  * Open files...
  */

  if ((dstfile = fopen(dst, "wb")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create \"%s\" -\n     %s\n", dst,
            strerror(errno));
    return (-1);
  }

  if ((srcfile = fopen(src, "rb")) == NULL)
  {
    fclose(dstfile);
    unlink(dst);

    fprintf(stderr, "epm: Unable to open \"%s\" -\n     %s\n", src,
            strerror(errno));
    return (-1);
  }

 /*
  * Copy from src to dst...
  */

  while ((bytes = fread(buffer, 1, sizeof(buffer), srcfile)) > 0)
    if (fwrite(buffer, 1, bytes, dstfile) < bytes)
    {
      fprintf(stderr, "epm: Unable to write to \"%s\" -\n     %s\n", src,
              strerror(errno));

      fclose(srcfile);
      fclose(dstfile);
      unlink(dst);

      return (-1);
    }

 /*
  * Close files and return...
  */

  fclose(srcfile);
  fclose(dstfile);

  return (0);
}


/*
 * 'free_dist()' - Free memory used by a distribution.
 */

static void
free_dist(dist_t *dist)		/* I - Distribution to free */
{
  if (dist->num_files > 0)
    free(dist->files);

  free_strings(dist->num_installs, dist->installs);
  free_strings(dist->num_removes, dist->removes);
  free_strings(dist->num_patches, dist->patches);
  free_strings(dist->num_incompats, dist->incompats);
  free_strings(dist->num_requires, dist->requires);

  free(dist);
}


/*
 * 'free_strings()' - Free memory used by the array of strings.
 */

static void
free_strings(int  num_strings,
             char **strings)
{
  int	i;


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
 * 'read_dist()' - Read a software distribution.
 */

static dist_t *				/* O - New distribution */
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
	else if (strcmp(line, "%install") == 0)
	  dist->num_installs = add_string(dist->num_installs, &(dist->installs),
	                                  temp);
	else if (strcmp(line, "%remove") == 0)
	  dist->num_removes = add_string(dist->num_removes, &(dist->removes),
	                                 temp);
	else if (strcmp(line, "%patch") == 0)
	  dist->num_patches = add_string(dist->num_patches, &(dist->patches),
	                                 temp);
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
      else if (sscanf(line, "%c%o%s%s%s%s", &type, &mode, user, group,
        	      tempdst, tempsrc) < 5)
	fprintf(stderr, "epm: Bad line - %s\n", line);
      else
      {
	expand_name(dst, tempdst);
	if (tolower(type) != 'd' && type != 'R')
	  expand_name(src, tempsrc);
	else
	  src[0] = '\0';

        if (dist->num_files == 0)
	  dist->files = (file_t *)malloc(sizeof(file_t));
	else
	  dist->files = (file_t *)realloc(dist->files, sizeof(file_t) *
					               (dist->num_files + 1));

        file = dist->files + dist->num_files;
	dist->num_files ++;

        file->type = type;
	file->mode = mode;
        strcpy(file->src, src);
	strcpy(file->dst, dst);
	strcpy(file->user, user);
	strcpy(file->group, group);
      }

    fclose(listfiles[listlevel]);
    listlevel --;
  }
  while (listlevel >= 0);

  return (dist);
}


/*
 * 'usage()' - Show command-line usage instructions.
 */

static void
usage(void)
{
  puts("Usage: epm [-g] [-n[mrs]] [-t] product [list-file]");
  exit(1);
}


/*
 * 'write_dist()' - Write a software distribution...
 */

static int			/* O - -1 on error, 0 on success */
write_dist(char   *title,	/* I - Title to show */
           char   *directory,	/* I - Directory */
	   char   *prodname,	/* I - Product name */
           char   *platname,	/* I - Platform name */
	   dist_t *dist,	/* I - Distribution */
	   char   **files)	/* I - Filenames */
{
  int		i;		/* Looping var */
  FILE		*tarfile;	/* Distribution tar file */
  char		filename[1024],	/* Name of temporary file */
		srcname[1024],	/* Name of source file in distribution */
		dstname[1024],	/* Name of destination file in distribution */
		command[1024];	/* Command to run */
  int		blocks,		/* Blocks in this file */
		tblocks;	/* Total number of blocks */
  struct stat	srcstat;	/* Source file information */


  printf("Writing %s archive:", title);
  fflush(stdout);

  if (platname[0])
    sprintf(filename, "%s/%s-%s-%s.tar.gz", directory, prodname, dist->version,
            platname);
  else
    sprintf(filename, "%s/%s-%s.tar.gz", directory, prodname, dist->version);

  sprintf(command, EPM_GZIP " -9 >%s", filename);
  if ((tarfile = popen(command, "w")) == NULL)
  {
    puts("");
    fprintf(stderr, "epm: Unable to create output pipe to gzip -\n     %s\n",
            strerror(errno));
    return (-1);
  }

  tblocks = 0;
  for (i = 0; files[i] != NULL; i ++)
  {
    sprintf(srcname, "%s/%s.%s", directory, prodname, files[i]);
    sprintf(dstname, "%s.%s", prodname, files[i]);

    stat(srcname, &srcstat);
    if (write_header(tarfile, TAR_NORMAL, srcstat.st_mode, srcstat.st_size,
	             srcstat.st_mtime, "root", "root", dstname, NULL) < 0)
    {
      puts("");
      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if ((blocks = write_file(tarfile, srcname)) < 0)
    {
      puts("");
      fprintf(stderr, "epm: Error writing file data for %s -\n    %s\n",
	      dstname, strerror(errno));
      return (-1);
    }

    printf(" %s", files[i]);
    fflush(stdout);

    tblocks += blocks + 1;
  }

  write_padding(tarfile, tblocks);
  pclose(tarfile);

  stat(filename, &srcstat);
  if (srcstat.st_size > (1024 * 1024))
    printf(" size=%.1fM\n", srcstat.st_size / 1024.0 / 1024.0);
  else
    printf(" size=%.0fk\n", srcstat.st_size / 1024.0);

  return (0);
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
 * 'write_install()' - Write the installation script.
 */

static int			/* O - -1 on error, 0 on success */
write_install(dist_t *dist,	/* I - Software distribution */
              char   *prodname,	/* I - Product name */
	      char   *directory)/* I - Directory */
{
  int		i;		/* Looping var */
  FILE		*scriptfile;	/* Install script */
  char		filename[1024];	/* Name of temporary file */
  file_t	*file;		/* Software file */


  puts("Writing installation script...");

  sprintf(filename, "%s/%s.install", directory, prodname);

  unlink(filename);
  if ((scriptfile = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create installation script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fchmod(fileno(scriptfile), 0555);

  fputs("#!/bin/sh\n", scriptfile);
  fprintf(scriptfile, "# Installation script for %s version %s.\n",
          dist->product, dist->version);
  fputs("# Produced using " EPM_VERSION "; report problems to epm@easysw.com.\n",
        scriptfile);
  fprintf(scriptfile, "#%%product %s\n", dist->product);
  fprintf(scriptfile, "#%%copyright %s\n", dist->copyright);
  fprintf(scriptfile, "#%%version %s %d\n", dist->version, dist->vernumber);
  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", scriptfile);
  fputs("	tar=\"/bin/tar xf\"\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	tar=\"/bin/tar xPf\"\n", scriptfile);
  fputs("fi\n", scriptfile);
  fputs("if test \"`" EPM_WHOAMI "`\" != \"root\"; then\n", scriptfile);
  fputs("	echo Sorry, you must be root to install this software.\n", scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "echo Copyright %s\n", dist->copyright);
  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  fprintf(scriptfile, "	echo This installation script will install the %s\n",
          dist->product);
  fprintf(scriptfile, "	echo software version %s on your system.\n",
          dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you wish to continue? \"\n", scriptfile);
#else
  fputs("		echo \"Do you wish to continue? \\c\"\n", scriptfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 0\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fprintf(scriptfile, "	more %s.license\n", prodname);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you agree with the terms of this license? \"\n", scriptfile);
#else
  fputs("		echo \"Do you agree with the terms of this license? \\c\"\n", scriptfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 0\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "if test -x " EPM_SOFTWARE "/%s.remove; then\n", prodname);
  fprintf(scriptfile, "	echo Removing old versions of %s software...\n",
          prodname);
  fprintf(scriptfile, "	" EPM_SOFTWARE "/%s.remove now\n", prodname);
  fputs("fi\n", scriptfile);

  for (i = 0; i < dist->num_requires; i ++)
  {
    fprintf(scriptfile, "#%%requires: %s\n", dist->requires[i]);
    fprintf(scriptfile, "if test ! -x " EPM_SOFTWARE "/%s.remove; then\n",
            dist->requires[i]);
    fprintf(scriptfile, "	if test -x %d.install; then\n",
            dist->requires[i]);
    fprintf(scriptfile, "		echo Installing required %s software...\n",
            dist->requires[i]);
    fprintf(scriptfile, "		./%s.install now\n", dist->requires[i]);
    fputs("	else\n", scriptfile);
    fprintf(scriptfile, "		echo Sorry, you must first install \\'%s\\'!\n",
	    dist->requires[i]);
    fputs("		exit 1\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  for (i = 0; i < dist->num_incompats; i ++)
  {
    fprintf(scriptfile, "#%%incompats: %s\n", dist->incompats[i]);
    fprintf(scriptfile, "if test -x " EPM_SOFTWARE "/%s.remove; then\n",
            dist->incompats[i]);
    fprintf(scriptfile, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	    dist->incompats[i]);
    fprintf(scriptfile, "	echo Please remove it first by running \\'/etc/software/%s.remove\\'.\n",
	    dist->incompats[i]);
    fputs("	exit 1\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'f' || tolower(file->type) == 'l')
      break;

  if (i)
  {
    fputs("echo Backing up old versions of files to be installed...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'f' || tolower(file->type) == 'l')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test -d $file -o -f $file -o -h $file; then\n", scriptfile);
    fputs("		/bin/mv $file $file.O\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'd')
      break;

  if (i)
  {
    fputs("echo Creating installation directories...\n", scriptfile);

    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'd')
      {
	fprintf(scriptfile, "if test ! -d %s -a ! -f %s -a ! -h %s; then\n",
        	file->dst, file->dst, file->dst);
	fprintf(scriptfile, "	/bin/mkdir -p %s\n", file->dst);
	fputs("else\n", scriptfile);
	fprintf(scriptfile, "	if test -f %s; then\n", file->dst);
	fprintf(scriptfile, "		echo Error: %s already exists as a regular file!\n",
	        file->dst);
	fputs("		exit 1\n", scriptfile);
	fputs("	fi\n", scriptfile);
	fputs("fi\n", scriptfile);
	fprintf(scriptfile, "/bin/chown %s %s\n", file->user, file->dst);
	fprintf(scriptfile, "/bin/chgrp %s %s\n", file->group, file->dst);
	fprintf(scriptfile, "/bin/chmod %o %s\n", file->mode, file->dst);
      }
  }

  fputs("echo Installing software...\n", scriptfile);
  fprintf(scriptfile, "$tar %s.sw\n", prodname);

  fputs("if test -d " EPM_SOFTWARE "; then\n", scriptfile);
  fprintf(scriptfile, "	/bin/rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);
  fputs("else\n", scriptfile);
  fputs("	/bin/mkdir -p " EPM_SOFTWARE "\n", scriptfile);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "/bin/cp %s.remove " EPM_SOFTWARE "\n", prodname);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
      break;

  if (i)
  {
    fputs("echo Checking configuration files...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'c')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test ! -f $file; then\n", scriptfile);
    fputs("		/bin/mv $file.N $file\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  if (dist->num_installs)
  {
    fputs("echo Running post-installation commands...\n", scriptfile);

    for (i = 0; i < dist->num_installs; i ++)
      fprintf(scriptfile, "%s\n", dist->installs[i]);
  }

  fputs("echo Installation is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_padding()' - Write the padding block(s) to the end of the
 *                     tar file.
 */

static int			/* O - -1 on error, 0 on success */
write_padding(FILE *fp,		/* I - File to write to */
              int  blocks)	/* I - Number of blocks written */
{
  char	padding[TAR_BLOCKS * TAR_BLOCK];
				/* Padding for tar blocks */


 /*
  * Zero the padding record...
  */

  memset(padding, 0, sizeof(padding));

 /*
  * Write a single 0 block to signal the end of the archive...
  */

  if (fwrite(padding, 1, TAR_BLOCK, fp) < 1)
    return (-1);

  blocks ++;

 /*
  * Pad the tar files to TAR_BLOCKS blocks...  This is bullshit of course,
  * but old versions of tar need it...
  */

  blocks = TAR_BLOCKS - (blocks % TAR_BLOCKS);

  if (blocks < TAR_BLOCKS)
    if (fwrite(padding, 1, blocks * TAR_BLOCK, fp) < 1)
      return (-1);

  return (0);
}


/*
 * 'write_patch()' - Write the patch script.
 */

static int			/* O - -1 on error, 0 on success */
write_patch(dist_t *dist,	/* I - Software distribution */
            char   *prodname,	/* I - Product name */
	    char   *directory)	/* I - Directory */
{
  int		i;		/* Looping var */
  FILE		*scriptfile;	/* Patch script */
  char		filename[1024];	/* Name of temporary file */
  file_t	*file;		/* Software file */


  puts("Writing patch script...");

  sprintf(filename, "%s/%s.patch", directory, prodname);

  unlink(filename);
  if ((scriptfile = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create patch script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fchmod(fileno(scriptfile), 0555);

  fputs("#!/bin/sh\n", scriptfile);
  fprintf(scriptfile, "# Patch script for %s version %s.\n", dist->product,
          dist->version);
  fputs("# Produced using " EPM_VERSION "; report problems to epm@easysw.com.\n",
        scriptfile);
  fprintf(scriptfile, "#%%product %s\n", dist->product);
  fprintf(scriptfile, "#%%copyright %s\n", dist->copyright);
  fprintf(scriptfile, "#%%version %s %d\n", dist->version, dist->vernumber);
  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", scriptfile);
  fputs("	tar=\"/bin/tar xf\"\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	tar=\"/bin/tar xPf\"\n", scriptfile);
  fputs("fi\n", scriptfile);
  fputs("if test \"`" EPM_WHOAMI "`\" != \"root\"; then\n", scriptfile);
  fputs("	echo Sorry, you must be root to install this software.\n",
        scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "echo Copyright %s\n", dist->copyright);
  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  fprintf(scriptfile, "	echo This installation script will patch the %s\n",
          dist->product);
  fprintf(scriptfile, "	echo software to version %s on your system.\n", dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you wish to continue? \"\n", scriptfile);
#else
  fputs("		echo \"Do you wish to continue? \\c\"\n", scriptfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 0\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fprintf(scriptfile, "	more %s.license\n", prodname);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you agree with the terms of this license? \"\n", scriptfile);
#else
  fputs("		echo \"Do you agree with the terms of this license? \\c\"\n", scriptfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 0\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fputs("fi\n", scriptfile);

  fprintf(scriptfile, "if test ! -x " EPM_SOFTWARE "/%s.remove; then\n",
          prodname);
  fputs("	echo You do not appear to have the base software installed!\n",
        scriptfile);
  fputs("	echo Please install the full distribution instead.\n", scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);

  if (dist->num_removes)
  {
    fputs("echo Running pre-patch commands...\n", scriptfile);

    for (i = 0; i < dist->num_removes; i ++)
      fprintf(scriptfile, "%s\n", dist->removes[i]);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'F' || file->type == 'L')
      break;

  if (i)
  {
    fputs("echo Backing up old versions of files to be installed...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'F' || file->type == 'L')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test -d $file -o -f $file -o -h $file; then\n", scriptfile);
    fputs("		/bin/mv $file $file.O\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'D')
      break;

  if (i)
  {
    fputs("echo Creating new installation directories...\n", scriptfile);

    for (; i > 0; i --, file ++)
      if (file->type == 'D')
      {
	fprintf(scriptfile, "if test ! -d %s -a ! -f %s -a ! -h %s; then\n",
        	file->dst, file->dst, file->dst);
	fprintf(scriptfile, "	/bin/mkdir -p %s\n", file->dst);
	fputs("else\n", scriptfile);
	fprintf(scriptfile, "	if test -f %s; then\n", file->dst);
	fprintf(scriptfile, "		echo Error: %s already exists as a regular file!\n",
	        file->dst);
	fputs("		exit 1\n", scriptfile);
	fputs("	fi\n", scriptfile);
	fputs("fi\n", scriptfile);
	fprintf(scriptfile, "/bin/chown %s %s\n", file->user, file->dst);
	fprintf(scriptfile, "/bin/chgrp %s %s\n", file->group, file->dst);
	fprintf(scriptfile, "/bin/chmod %o %s\n", file->mode, file->dst);
      }
  }

  fputs("echo Patching software...\n", scriptfile);
  fprintf(scriptfile, "$tar %s.psw\n", prodname);

  fprintf(scriptfile, "/bin/rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);
  fprintf(scriptfile, "/bin/cp %s.remove " EPM_SOFTWARE "\n", prodname);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'C')
      break;

  if (i)
  {
    fputs("echo Checking configuration files...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'C')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test ! -f $file; then\n", scriptfile);
    fputs("		/bin/mv $file.N $file\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'R')
      break;

  if (i)
  {
    fputs("echo Removing files that are no longer used...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'R')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	rm -f $file\n", scriptfile);
    fputs("	if test -d $file.O -o -f $file.O -o -h $file.O; then\n", scriptfile);
    fputs("		/bin/mv $file.O $file\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  if (dist->num_patches)
  {
    fputs("echo Running post-installation commands...\n", scriptfile);

    for (i = 0; i < dist->num_patches; i ++)
      fprintf(scriptfile, "%s\n", dist->patches[i]);
  }

  fputs("echo Patching is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_remove()' - Write the removal script.
 */

static int			/* O - -1 on error, 0 on success */
write_remove(dist_t *dist,	/* I - Software distribution */
             char   *prodname,	/* I - Product name */
	     char   *directory)	/* I - Directory */
{
  int		i;		/* Looping var */
  FILE		*scriptfile;	/* Remove script */
  char		filename[1024];	/* Name of temporary file */
  file_t	*file;		/* Software file */


  puts("Writing removal script...");

  sprintf(filename, "%s/%s.remove", directory, prodname);

  unlink(filename);
  if ((scriptfile = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create removal script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fchmod(fileno(scriptfile), 0555);

  fputs("#!/bin/sh\n", scriptfile);
  fprintf(scriptfile, "# Removal script for %s version %s.\n", dist->product,
          dist->version);
  fputs("# Produced using " EPM_VERSION "; report problems to epm@easysw.com.\n",
        scriptfile);
  fprintf(scriptfile, "#%%product %s\n", dist->product);
  fprintf(scriptfile, "#%%copyright %s\n", dist->copyright);
  fprintf(scriptfile, "#%%version %s %d\n", dist->version, dist->vernumber);
  fputs("if test \"`" EPM_WHOAMI "`\" != \"root\"; then\n", scriptfile);
  fputs("	echo Sorry, you must be root to remove this software.\n", scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "echo Copyright %s\n", dist->copyright);
  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  fprintf(scriptfile, "	echo This removal script will remove the %s\n",
          dist->product);
  fprintf(scriptfile, "	echo software version %s from your system.\n",
          dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
#ifdef HAVE_BROKEN_ECHO
  fputs("		echo -n \"Do you wish to continue? \"\n", scriptfile);
#else
  fputs("		echo \"Do you wish to continue? \\c\"\n", scriptfile);
#endif /* HAVE_BROKEN_ECHO */
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 0\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fputs("fi\n", scriptfile);

 /*
  * Find any removal commands in the list file...
  */

  if (dist->num_removes)
  {
    fputs("echo Running pre-removal commands...\n", scriptfile);

    for (i = 0; i < dist->num_removes; i ++)
      fprintf(scriptfile, "%s\n", dist->removes[i]);
  }

  fputs("echo Removing/restoring installed files...\n", scriptfile);

  fputs("for file in", scriptfile);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'f' || tolower(file->type) == 'l')
      fprintf(scriptfile, " %s", file->dst);

  fputs("; do\n", scriptfile);
  fputs("	rm -f $file\n", scriptfile);
  fputs("	if test -d $file.O -o -f $file.O -o -h $file.O; then\n", scriptfile);
  fputs("		/bin/mv $file.O $file\n", scriptfile);
  fputs("	fi\n", scriptfile);
  fputs("done\n", scriptfile);

  fprintf(scriptfile, "rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);

  fputs("echo Removal is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * End of "$Id: epm.c,v 1.15 1999/08/16 15:52:39 mike Exp $".
 */
