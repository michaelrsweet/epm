/*
 * "$Id: epm.h,v 1.1 1999/11/04 20:31:07 mike Exp $"
 *
 *   Definitions for the ESP Package Manager (EPM).
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
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
 * "test" command symlink option...
 */

#if defined(__hpux) || defined(__sun)
#  define SYMLINK "-h"
#else
#  define SYMLINK "-L"
#endif /* __hpux || __sun */


/*
 * TAR constants...
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
 * Package formats...
 */

enum
{
  PACKAGE_PORTABLE,			/* Shell-script based EPM */
  PACKAGE_DEB,				/* Debian package format */
  PACKAGE_INST,				/* IRIX package format */
  PACKAGE_PKG,				/* AT&T package format (AIX, Solaris) */
  PACKAGE_RPM,				/* RedHat package format */
  PACKAGE_SWINSTALL			/* HP-UX package format */
};


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

typedef struct				/**** TAR file ****/
{
  FILE	*file;				/* File to write to */
  int	blocks;				/* Number of blocks written */
  int	compressed;			/* Compressed output? */
} tarf_t;

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
  char		product[256],		/* Product name */
		version[256],		/* Product version string */
		copyright[256],		/* Product copyright */
		vendor[256],		/* Vendor name */
		license[256],		/* License file to copy */
		readme[256];		/* README file to copy */
  int		num_descriptions;	/* Number of description strings */
  char		**descriptions;		/* Description strings */
  int		vernumber;		/* Version number */
  int		num_installs;		/* Number of installation commands */
  char		**installs;		/* Installation commands */
  int		num_removes;		/* Number of removal commands */
  char		**removes;		/* Removal commands */
  int		num_patches;		/* Number of patch commands */
  char		**patches;		/* Patch commands */
  int		num_incompats;		/* Number of incompatible products */
  char		**incompats;		/* Incompatible products/files */
  int		num_requires;		/* Number of requires products */
  char		**requires;		/* Required products/files */
  int		num_files;		/* Number of files */
  file_t	*files;			/* Files */
} dist_t;


/*
 * Globals...
 */

extern int	Verbosity;		/* Be verbose? */


/*
 * Prototypes...
 */

extern int	copy_file(const char *dst, const char *src,
		          int mode, int owner, int group);
extern void	free_dist(dist_t *dist);
extern int	make_deb(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_directory(const char *directory, int mode, int owner,
		               int group);
extern int	make_inst(const char *prodname, const char *directory,
		          const char *platname, dist_t *dist,
			  struct utsname *platform);
extern int	make_link(const char *dst, const char *src);
extern int	make_pkg(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_portable(const char *prodname, const char *directory,
		              const char *platname, dist_t *dist,
			      struct utsname *platform);
extern int	make_rpm(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_swinstall(const char *prodname, const char *directory,
		               const char *platname, dist_t *dist,
			       struct utsname *platform);
extern dist_t	*read_dist(char *filename, struct utsname *platform);
extern void	strip_execs(dist_t *dist);
extern int	tar_close(tarf_t *tar);
extern int	tar_file(tarf_t *tar, const char *filename);
extern int	tar_header(tarf_t *tar, char type, int mode, int size,
		           time_t mtime, const char *user, const char *group,
			   const char *pathname, const char *linkname);
extern tarf_t	*tar_open(const char *filename, int compress);

/*
 * End of "$Id: epm.h,v 1.1 1999/11/04 20:31:07 mike Exp $".
 */
