/*
 * "$Id: epm.h,v 1.19 2001/06/25 17:27:17 mike Exp $"
 *
 *   Definitions for the ESP Package Manager (EPM).
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
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "epmstring.h"
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <sys/utsname.h>

#  if HAVE_DIRENT_H
#    include <dirent.h>
typedef struct dirent DIRENT;
#    define NAMLEN(dirent) strlen((dirent)->d_name)
#  else
#    if HAVE_SYS_NDIR_H
#      include <sys/ndir.h>
#    endif
#    if HAVE_SYS_DIR_H
#      include <sys/dir.h>
#    endif
#    if HAVE_NDIR_H
#      include <ndir.h>
#    endif
typedef struct direct DIRENT;
#    define NAMLEN(dirent) (dirent)->d_namlen
#  endif


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
  PACKAGE_BSD,				/* BSD package format */
  PACKAGE_DEB,				/* Debian package format */
  PACKAGE_INST,				/* IRIX package format */
  PACKAGE_PKG,				/* AT&T package format (AIX, Solaris) */
  PACKAGE_RPM,				/* RedHat package format */
  PACKAGE_SETLD,			/* Tru64 package format */
  PACKAGE_SWINSTALL			/* HP-UX package format */
};

/*
 * Command types...
 */

enum
{
  COMMAND_PRE_INSTALL,			/* Command to run before install */
  COMMAND_POST_INSTALL,			/* Command to run after install */
  COMMAND_PRE_PATCH,			/* Command to run before patch */
  COMMAND_POST_PATCH,			/* Command to run after patch */
  COMMAND_PRE_REMOVE,			/* Command to run before remove */
  COMMAND_POST_REMOVE			/* Command to run after remove */
};

/*
 * Dependency types...
 */

enum
{
  DEPEND_REQUIRES,			/* This product requires */
  DEPEND_INCOMPAT,			/* This product is incompatible with */
  DEPEND_REPLACES			/* This product replaces */
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

typedef struct				/**** Install/Patch/Remove Commands ****/
{
  char	type;				/* Command type */
  char	*command;			/* Command string */
} command_t;

typedef struct				/**** Dependencies ****/
{
  char	type;				/* Dependency type */
  char	product[256];			/* Product name */
  char	version[2][256];		/* Product version string */
  int	vernumber[2];			/* Product version number */
} depend_t;

typedef struct				/**** Distribution Structure ****/
{
  char		product[256],		/* Product name */
		version[256],		/* Product version string */
		copyright[256],		/* Product copyright */
		vendor[256],		/* Vendor name */
		packager[256],		/* Packager name */
		license[256],		/* License file to copy */
		readme[256];		/* README file to copy */
  int		num_descriptions;	/* Number of description strings */
  char		**descriptions;		/* Description strings */
  int		vernumber;		/* Version number */
  int		relnumber;		/* Release number */
  int		num_commands;		/* Number of commands */
  command_t	*commands;		/* Commands */
  int		num_depends;		/* Number of dependencies */
  depend_t	*depends;		/* Dependencies */
  int		num_files;		/* Number of files */
  file_t	*files;			/* Files */
} dist_t;


/*
 * Globals...
 */

extern int		Verbosity;	/* Be verbose? */
extern int		KeepFiles;	/* Keep intermediate files? */
extern const char	*SetupProgram;	/* Setup program */


/*
 * Prototypes...
 */

extern void	add_command(dist_t *dist, char type, const char *command);
extern void	add_depend(dist_t *dist, char type, const char *line);
extern file_t	*add_file(dist_t *dist);
extern int	copy_file(const char *dst, const char *src,
		          int mode, int owner, int group);
extern void	free_dist(dist_t *dist);
extern int	make_bsd(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
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
			      struct utsname *platform, const char *setup);
extern int	make_rpm(const char *prodname, const char *directory,
		         const char *platname, dist_t *dist,
			 struct utsname *platform);
extern int	make_setld(const char *prodname, const char *directory,
		           const char *platname, dist_t *dist,
			   struct utsname *platform);
extern int	make_swinstall(const char *prodname, const char *directory,
		               const char *platname, dist_t *dist,
			       struct utsname *platform);
extern dist_t	*read_dist(const char *filename, struct utsname *platform,
		           const char *format);
extern void	sort_dist_files(dist_t *dist);
extern void	strip_execs(dist_t *dist);
extern int	tar_close(tarf_t *tar);
extern int	tar_directory(tarf_t *tar, const char *srcpath,
		              const char *dstpath);
extern int	tar_file(tarf_t *tar, const char *filename);
extern int	tar_header(tarf_t *tar, char type, int mode, int size,
		           time_t mtime, const char *user, const char *group,
			   const char *pathname, const char *linkname);
extern tarf_t	*tar_open(const char *filename, int compress);


/*
 * End of "$Id: epm.h,v 1.19 2001/06/25 17:27:17 mike Exp $".
 */
