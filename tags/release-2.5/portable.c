/*
 * "$Id: portable.c,v 1.24 2001/03/15 16:13:35 mike Exp $"
 *
 *   Portable package gateway for the ESP Package Manager (EPM).
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
 *   make_portable()   - Make a portable software distribution package.
 *   write_commands()  - Write commands.
 *   write_common()    - Write the common shell script header.
 *   write_dist()      - Write a software distribution...
 *   write_echocheck() - Write the echo check to find the right echo options.
 *   write_install()   - Write the installation script.
 *   write_patch()     - Write the patch script.
 *   write_remove()    - Write the removal script.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	write_commands(dist_t *dist, FILE *fp, char type);
static FILE	*write_common(dist_t *dist, const char *title,
		              const char *filename);
static int	write_depends(dist_t *dist, FILE *fp);
static int	write_dist(const char *title, const char *directory,
		           const char *prodname, const char *platname,
			   dist_t *dist, const char **files,
			   const char *setup);
static int	write_echocheck(FILE *fp);
static int	write_install(dist_t *dist, const char *prodname,
		              const char *directory);
static int	write_patch(dist_t *dist, const char *prodname,
		            const char *directory);
static int	write_remove(dist_t *dist, const char *prodname,
		             const char *directory);


/*
 * 'make_portable()' - Make a portable software distribution package.
 */

int					/* O - 1 = success, 0 = fail */
make_portable(const char     *prodname,	/* I - Product short name */
              const char     *directory,/* I - Directory for distribution files */
              const char     *platname,	/* I - Platform name */
              dist_t         *dist,	/* I - Distribution information */
	      struct utsname *platform,	/* I - Platform information */
              const char     *setup)	/* I - Setup GUI image */
{
  int		i;		/* Looping var */
  int		havepatchfiles;	/* 1 if we have patch files, 0 otherwise */
  tarf_t	*tarfile;	/* Distribution tar file */
  char		swname[255],	/* Name of distribution tar file */
		pswname[255],	/* Name of patch tar file */
		filename[1024];	/* Name of temporary file */
  time_t	deftime;	/* File creation time */
  struct stat	srcstat;	/* Source file information */
  file_t	*file;		/* Software file */
  static const char	*distfiles[] =	/* Distribution files */
		{
		  "install",
		  "license",
		  "readme",
		  "remove",
		  "ss",
		  "sw",
		  NULL
		};
  static const char	*patchfiles[] =	/* Patch files */
		{
		  "license",
		  "patch",
		  "pss",
		  "psw",
		  "readme",
		  "remove",
		  NULL
		};


  (void)platform; /* Eliminates compiler warning about unused variable */

  if (Verbosity)
    puts("Creating PORTABLE distribution...");

 /*
  * See if we need to make a patch distribution...
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (isupper(file->type))
      break;

  havepatchfiles = i > 0;

 /*
  * Copy the license and readme files...
  */

  deftime = time(NULL);

  if (Verbosity)
    puts("Copying license and readme files...");

  sprintf(filename, "%s/%s.license", directory, prodname);
  if (copy_file(filename, dist->license, 0444, getuid(), getgid()))
    return (1);

  sprintf(filename, "%s/%s.readme", directory, prodname);
  if (copy_file(filename, dist->readme, 0444, getuid(), getgid()))
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
  * Create the non-shared software distribution file...
  */

  if (Verbosity)
    puts("Creating non-shared software distribution file...");

  sprintf(swname, "%s.sw", prodname);
  sprintf(filename, "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = tar_open(filename, 0)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) != 0)
      switch (tolower(file->type))
      {
	case 'f' : /* Regular file */
	case 'c' : /* Config file */
	case 'i' : /* Init script */
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

	    if (tolower(file->type) == 'c')
	      sprintf(filename, "%s.N", file->dst);
	    else if (tolower(file->type) == 'i')
	      sprintf(filename, EPM_SOFTWARE "/init.d/%s", file->dst);
	    else
              strcpy(filename, file->dst);

            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, filename);

	    if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                   srcstat.st_mtime, file->user, file->group,
			   filename, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file header - %s\n",
	              strerror(errno));
	      return (1);
	    }

	    if (tar_file(tarfile, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      return (1);
	    }
	    break;

	case 'd' : /* Create directory */
            if (Verbosity > 1)
	      printf("Directory %s...\n", file->dst);
	    break;

	case 'l' : /* Link file */
            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, file->dst);

	    if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      return (1);
	    }
	    break;
      }

  tar_close(tarfile);

 /*
  * Create the shared software distribution file...
  */

  if (Verbosity)
    puts("Creating shared software distribution file...");

  sprintf(swname, "%s.ss", prodname);
  sprintf(filename, "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = tar_open(filename, 0)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) == 0)
      switch (tolower(file->type))
      {
	case 'f' : /* Regular file */
	case 'c' : /* Config file */
	case 'i' : /* Init script */
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

	    if (tolower(file->type) == 'c')
	      sprintf(filename, "%s.N", file->dst);
	    else if (tolower(file->type) == 'i')
	      sprintf(filename, EPM_SOFTWARE "/init.d/%s", file->dst);
	    else
              strcpy(filename, file->dst);

            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, filename);

	    if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                   srcstat.st_mtime, file->user, file->group,
			   filename, NULL) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file header - %s\n",
	              strerror(errno));
	      return (1);
	    }

	    if (tar_file(tarfile, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      return (1);
	    }
	    break;

	case 'd' : /* Create directory */
            if (Verbosity > 1)
	      printf("%s...\n", file->dst);
	    break;

	case 'l' : /* Link file */
            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, file->dst);

	    if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      return (1);
	    }
	    break;
      }

  tar_close(tarfile);

 /*
  * Create the patch distribution files...
  */

  if (havepatchfiles)
  {
    if (Verbosity)
      puts("Creating non-shared software patch file...");

    sprintf(pswname, "%s.psw", prodname);
    sprintf(filename, "%s/%s", directory, pswname);

    unlink(filename);
    if ((tarfile = tar_open(filename, 0)) == NULL)
    {
      fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
              filename, strerror(errno));
      return (1);
    }

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (strncmp(file->dst, "/usr", 4) != 0)
	switch (file->type)
	{
	  case 'C' : /* Config file */
	  case 'F' : /* Regular file */
          case 'I' : /* Init script */
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

	      if (file->type == 'C')
		sprintf(filename, "%s.N", file->dst);
	      else if (file->type == 'I')
		sprintf(filename, EPM_SOFTWARE "/init.d/%s", file->dst);
	      else
        	strcpy(filename, file->dst);

              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, filename);

	      if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                     srcstat.st_mtime, file->user, file->group,
			     filename, NULL) < 0)
	      {
		fprintf(stderr, "epm: Error writing file header - %s\n",
	        	strerror(errno));
		return (1);
	      }

	      if (tar_file(tarfile, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing file data - %s\n",
	        	strerror(errno));
		return (1);
	      }
	      break;

	  case 'd' : /* Create directory */
              if (Verbosity > 1)
		printf("%s...\n", file->dst);
	      break;

	  case 'L' : /* Link file */
              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, file->dst);

	      if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                     file->user, file->group, file->dst, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing link header - %s\n",
	        	strerror(errno));
		return (1);
	      }
	      break;
	}

    tar_close(tarfile);

    if (Verbosity)
      puts("Creating shared software patch file...");

    sprintf(pswname, "%s.pss", prodname);
    sprintf(filename, "%s/%s", directory, pswname);

    unlink(filename);
    if ((tarfile = tar_open(filename, 0)) == NULL)
    {
      fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
              filename, strerror(errno));
      return (1);
    }

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (strncmp(file->dst, "/usr", 4) == 0)
	switch (file->type)
	{
	  case 'C' : /* Config file */
	  case 'F' : /* Regular file */
          case 'I' : /* Init script */
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

	      if (file->type == 'C')
		sprintf(filename, "%s.N", file->dst);
	      else if (file->type == 'I')
		sprintf(filename, EPM_SOFTWARE "/init.d/%s", file->dst);
	      else
        	strcpy(filename, file->dst);

              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, filename);

	      if (tar_header(tarfile, TAR_NORMAL, file->mode, srcstat.st_size,
	                     srcstat.st_mtime, file->user, file->group,
			     filename, NULL) < 0)
	      {
		fprintf(stderr, "epm: Error writing file header - %s\n",
	        	strerror(errno));
		return (1);
	      }

	      if (tar_file(tarfile, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing file data - %s\n",
	        	strerror(errno));
		return (1);
	      }
	      break;

	  case 'd' : /* Create directory */
              if (Verbosity > 1)
		printf("%s...\n", file->dst);
	      break;

	  case 'L' : /* Link file */
              if (Verbosity > 1)
		printf("%s -> %s...\n", file->src, file->dst);

	      if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                     file->user, file->group, file->dst, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing link header - %s\n",
	        	strerror(errno));
		return (1);
	      }
	      break;
	}

    tar_close(tarfile);
  }

 /*
  * Create the distribution archives...
  */

  write_dist("distribution", directory, prodname, platname, dist, distfiles,
             setup);

  if (havepatchfiles)
  {
    sprintf(filename, "%s-patch", dist->version);
    strcpy(dist->version, filename);

    write_dist("patch", directory, prodname, platname, dist, patchfiles, setup);
  }

 /*
  * Return!
  */

  return (0);
}


/*
 * 'write_commands()' - Write commands.
 */

static int				/* O - 0 on success, -1 on failure */
write_commands(dist_t *dist,		/* I - Distribution */
               FILE   *fp,		/* I - File pointer */
               char   type)		/* I - Type of commands to write */
{
  int			i;		/* Looping var */
  command_t		*c;		/* Current command */
  static const char	*commands[] =	/* Command strings */
			{
			  "pre-install",
			  "post-install",
			  "pre-patch",
			  "post-patch",
			  "pre-remove",
			  "post-remove"
			};


  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == type)
      break;

  if (i)
  {
    fprintf(fp, "echo Running %s commands...\n", commands[type]);

    for (; i > 0; i --, c ++)
      if (c->type == type)
        if (fprintf(fp, "%s\n", c->command) < 1)
        {
          perror("epm: Error writing command");
          return (-1);
        }
  }

  return (0);
}


/*
 * 'write_common()' - Write the common shell script header.
 */

static FILE *				/* O - File pointer */
write_common(dist_t     *dist,		/* I - Distribution */
             const char *title,		/* I - "Installation", etc... */
             const char *filename)	/* I - Script to create */
{
  int	i;				/* Looping var */
  FILE	*fp;				/* File pointer */


 /*
  * Remove any existing copy of the file...
  */

  unlink(filename);

 /*
  * Create the script file...
  */

  if ((fp = fopen(filename, "w")) == NULL)
    return (NULL);

 /*
  * Update the permissions on the file...
  */

  fchmod(fileno(fp), 0755);

 /*
  * Write the standard header...
  */

  fputs("#!/bin/sh\n", fp);
  fprintf(fp, "# %s script for %s version %s.\n", title,
          dist->product, dist->version);
  fputs("# Produced using " EPM_VERSION "; report problems to epm@easysw.com.\n",
        fp);
  fprintf(fp, "#%%product %s\n", dist->product);
  fprintf(fp, "#%%vendor %s\n", dist->vendor);
  fprintf(fp, "#%%copyright %s\n", dist->copyright);
  fprintf(fp, "#%%version %s %d\n", dist->version, dist->vernumber);
  for (i = 0; i < dist->num_descriptions; i ++)
    fprintf(fp, "#%%description %s\n", dist->descriptions[i]);
  fputs("#\n", fp);

  fputs("if test \"`" EPM_WHOAMI "`\" != \"root\"; then\n", fp);
  fprintf(fp, "	echo Sorry, you must be root to %s this software.\n",
          title[0] == 'I' ? "install" : title[0] == 'R' ? "remove" : "patch");
  fputs("	exit 1\n", fp);
  fputs("fi\n", fp);
  fprintf(fp, "echo Copyright %s\n", dist->copyright);
  fputs("umask 002\n", fp);

  write_echocheck(fp);

 /*
  * Return the file pointer...
  */

  return (fp);
}


/*
 * 'write_depends()' - Write dependencies.
 */

static int				/* O - 0 on success, - 1 on failure */
write_depends(dist_t *dist,		/* I - Distribution */
              FILE   *fp)		/* I - File pointer */
{
  int			i;		/* Looping var */
  depend_t		*d;		/* Current dependency */
  static const char	*depends[] =	/* Dependency strings */
			{
			  "requires",
			  "incompat",
			  "replaces"
			};


  for (i = 0, d= dist->depends; i < dist->num_depends; i ++, d ++)
  {
    fprintf(fp, "#%%%s %s\n", depends[d->type], d->product);

    switch (d->type)
    {
      case DEPEND_REQUIRES :
          if (d->product[0] == '/')
          {
           /*
            * Require a file...
            */

            fprintf(fp, "if test ! -r %s -a ! -h %s; then\n",
                    d->product, d->product);
            fprintf(fp, "	echo Sorry, you must first install \\'%s\\'!\n",
	            d->product);
            fputs("	exit 1\n", fp);
            fputs("fi\n", fp);
          }
          else
          {
           /*
            * Require a product...
            */

            fprintf(fp, "if test ! -x " EPM_SOFTWARE "/%s.remove; then\n",
                    d->product);
            fprintf(fp, "	if test -x %s.install; then\n",
                    d->product);
            fprintf(fp, "		echo Installing required %s software...\n",
                    d->product);
            fprintf(fp, "		./%s.install now\n", d->product);
            fputs("	else\n", fp);
            fprintf(fp, "		echo Sorry, you must first install \\'%s\\'!\n",
	            d->product);
            fputs("		exit 1\n", fp);
            fputs("	fi\n", fp);
            fputs("fi\n", fp);
          }
	  break;

      case DEPEND_INCOMPAT :
          if (d->product[0] == '/')
          {
           /*
            * Incompatible with a file...
            */

            fprintf(fp, "if test -r %s -o -h %s; then\n",
                    d->product, d->product);
            fprintf(fp, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	            d->product);
            fputs("	echo Please remove it first.\n", fp);
            fputs("	exit 1\n", fp);
            fputs("fi\n", fp);
          }
          else
          {
           /*
            * Incompatible with a product...
            */

            fprintf(fp, "if test -x " EPM_SOFTWARE "/%s.remove; then\n",
                    d->product);
            fprintf(fp, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	            d->product);
            fprintf(fp, "	echo Please remove it first by running \\'/etc/software/%s.remove\\'.\n",
	            d->product);
            fputs("	exit 1\n", fp);
            fputs("fi\n", fp);
          }
	  break;

      case DEPEND_REPLACES :
          fprintf(fp, "if test -x " EPM_SOFTWARE "/%s.remove; then\n",
                  d->product);
          fprintf(fp, "	echo Automatically replacing \\'%s\\'...\n",
	          d->product);
          fprintf(fp, "	" EPM_SOFTWARE "/%s.remove now\n",
	          d->product);
          fputs("fi\n", fp);
    }
  }

  return (0);
}


/*
 * 'write_dist()' - Write a software distribution...
 */

static int				/* O - -1 on error, 0 on success */
write_dist(const char *title,		/* I - Title to show */
           const char *directory,	/* I - Directory */
	   const char *prodname,	/* I - Product name */
           const char *platname,	/* I - Platform name */
	   dist_t     *dist,		/* I - Distribution */
	   const char **files,		/* I - Filenames */
           const char *setup)		/* I - Setup GUI image file */
{
  int		i;		/* Looping var */
  tarf_t	*tarfile;	/* Distribution tar file */
  char		filename[1024],	/* Name of temporary file */
		srcname[1024],	/* Name of source file in distribution */
		dstname[1024];	/* Name of destination file in distribution */
  struct stat	srcstat;	/* Source file information */


  if (Verbosity)
  {
    printf("Writing %s archive:", title);
    fflush(stdout);
  }

  if (platname[0])
    sprintf(filename, "%s/%s-%s-%s.tar.gz", directory, prodname, dist->version,
            platname);
  else
    sprintf(filename, "%s/%s-%s.tar.gz", directory, prodname, dist->version);

  if ((tarfile = tar_open(filename, 1)) == NULL)
  {
    if (Verbosity)
      puts("");

    fprintf(stderr, "epm: Unable to create output pipe to gzip -\n     %s\n",
            strerror(errno));
    return (-1);
  }

  for (i = 0; files[i] != NULL; i ++)
  {
    sprintf(srcname, "%s/%s.%s", directory, prodname, files[i]);
    sprintf(dstname, "%s.%s", prodname, files[i]);

    stat(srcname, &srcstat);

    if (tar_header(tarfile, TAR_NORMAL, srcstat.st_mode & (~0222),
                   srcstat.st_size, srcstat.st_mtime, "root", "root",
		   dstname, NULL) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if (tar_file(tarfile, srcname) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file data for %s -\n    %s\n",
	      dstname, strerror(errno));
      return (-1);
    }

    if (Verbosity)
    {
      printf(" %s", files[i]);
      fflush(stdout);
    }
  }

  if (setup)
  {
   /*
    * Include the ESP Software Wizard (setup)...
    */

    if (stat(SetupProgram, &srcstat))
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Unable to stat GUI setup program %s - %s\n",
	      SetupProgram, strerror(errno));
      return (-1);
    }

    if (tar_header(tarfile, TAR_NORMAL, 0555, srcstat.st_size,
	           srcstat.st_mtime, "root", "root", "setup", NULL) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if (tar_file(tarfile, SetupProgram) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file data for setup -\n    %s\n",
	      strerror(errno));
      return (-1);
    }

    if (Verbosity)
    {
      printf(" setup");
      fflush(stdout);
    }

   /*
    * And the image file...
    */

    stat(setup, &srcstat);
    if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
	           srcstat.st_mtime, "root", "root", "setup.xpm", NULL) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if (tar_file(tarfile, setup) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file data for setup.xpm -\n    %s\n",
	      strerror(errno));
      return (-1);
    }

    if (Verbosity)
    {
      printf(" setup.xpm");
      fflush(stdout);
    }
  }

  tar_close(tarfile);

  if (Verbosity)
  {
    stat(filename, &srcstat);
    if (srcstat.st_size > (1024 * 1024))
      printf(" size=%.1fM\n", srcstat.st_size / 1024.0 / 1024.0);
    else
      printf(" size=%.0fk\n", srcstat.st_size / 1024.0);
  }

  return (0);
}


/*
 * 'write_echocheck()' - Write the echo check to find the right echo options.
 */

static int				/* O - -1 on error, 0 on success */
write_echocheck(FILE *fp)		/* I - Script file */
{
 /*
  * This is a simplified version of the autoconf test for echo; basically
  * we ignore the Stardent Vistra SVR4 case, since 1) we've never heard of
  * this OS, and 2) it doesn't provide the same functionality, specifically
  * the omission of a newline when prompting the user for some text.
  */

  fputs("# Determine correct echo options...\n", fp);
  fputs("if (echo \"testing\\c\"; echo 1,2,3) | grep c >/dev/null; then\n", fp);
  fputs("	ac_n=-n\n", fp);
  fputs("	ac_c=\n", fp);
  fputs("else\n", fp);
  fputs("	ac_n=\n", fp);
  fputs("	ac_c='\\c'\n", fp);
  fputs("fi\n", fp);

  return (0);
}


/*
 * 'write_install()' - Write the installation script.
 */

static int				/* O - -1 on error, 0 on success */
write_install(dist_t     *dist,		/* I - Software distribution */
              const char *prodname,	/* I - Product name */
	      const char *directory)	/* I - Directory */
{
  int		i;		/* Looping var */
  FILE		*scriptfile;	/* Install script */
  char		filename[1024];	/* Name of temporary file */
  file_t	*file;		/* Software file */


  if (Verbosity)
    puts("Writing installation script...");

  sprintf(filename, "%s/%s.install", directory, prodname);

  if ((scriptfile = write_common(dist, "Installation", filename)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create installation script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", scriptfile);
  fputs("	tar=\"/bin/tar -xpf\"\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	tar=\"/bin/tar -xpPf\"\n", scriptfile);
  fputs("fi\n", scriptfile);

  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  fprintf(scriptfile, "	echo This installation script will install the \'%s\'\n",
          dist->product);
  fprintf(scriptfile, "	echo software version \'%s\' on your system.\n",
          dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you wish to continue? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);

  fprintf(scriptfile, "	more %s.license\n", prodname);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you agree with the terms of this license? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
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

  write_depends(dist, scriptfile);
  write_commands(dist, scriptfile, COMMAND_PRE_INSTALL);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) != 0)
      break;

  if (i)
  {
    fputs("echo Backing up old versions of non-shared files to be installed...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) != 0)
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test -d $file -o -f $file -o " SYMLINK " $file; then\n", scriptfile);
    fputs("		/bin/mv -f $file $file.O\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) == 0)
      break;

  if (i)
  {
    fputs("if test -w /usr ; then\n", scriptfile);
    fputs("	echo Backing up old versions of shared files to be installed...\n", scriptfile);

    fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) == 0)
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("		if test -d $file -o -f $file -o " SYMLINK " $file; then\n", scriptfile);
    fputs("			/bin/mv -f $file $file.O\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
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
  fputs("if echo Write Test >/usr/.writetest 2>/dev/null; then\n", scriptfile);
  fputs("/bin/rm -f /usr/.writetest\n", scriptfile);
  fprintf(scriptfile, "	$tar %s.ss\n", prodname);
  fputs("fi\n", scriptfile);

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
    fputs("		/bin/mv -f $file.N $file\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      break;

  if (i)
  {
    fputs("echo Setting up init scripts...\n", scriptfile);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", scriptfile);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", scriptfile);
    fputs("	if test -d $dir/rc3.d -o " SYMLINK " $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("else\n", scriptfile);
    fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("		if test -d $rcdir/init.d; then\n", scriptfile);
    fputs("			/bin/rm -f $rcdir/init.d/$file\n", scriptfile);
    fputs("			/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/init.d/$file\n", scriptfile);
    fputs("		else\n", scriptfile);
    fputs("			if test -d /etc/init.d; then\n", scriptfile);
    fputs("				/bin/rm -f /etc/init.d/$file\n", scriptfile);
    fputs("				/bin/ln -s " EPM_SOFTWARE "/init.d/$file /etc/init.d/$file\n", scriptfile);
    fputs("			fi\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc0.d/K00$file\n", scriptfile);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc0.d/K00$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc2.d/S99$file\n", scriptfile);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc2.d/S99$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc3.d/S99$file\n", scriptfile);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc3.d/S99$file\n", scriptfile);
    fputs("		if test -d $rcdir/rc5.d; then\n", scriptfile);
    fputs("			/bin/rm -f $rcdir/rc5.d/S99$file\n", scriptfile);
    fputs("			/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc5.d/S99$file\n", scriptfile);
    fputs("		fi\n", scriptfile);
#ifdef __sgi
    fputs("		/etc/chkconfig -f $file on\n", scriptfile);
#endif /* __sgi */
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_INSTALL);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      fprintf(scriptfile, EPM_SOFTWARE "/init.d/%s start\n", file->dst);

  fputs("echo Installation is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_patch()' - Write the patch script.
 */

static int				/* O - -1 on error, 0 on success */
write_patch(dist_t     *dist,		/* I - Software distribution */
            const char *prodname,	/* I - Product name */
	    const char *directory)	/* I - Directory */
{
  int		i;		/* Looping var */
  FILE		*scriptfile;	/* Patch script */
  char		filename[1024];	/* Name of temporary file */
  file_t	*file;		/* Software file */


  if (Verbosity)
    puts("Writing patch script...");

  sprintf(filename, "%s/%s.patch", directory, prodname);

  if ((scriptfile = write_common(dist, "Patch", filename)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create patch script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test \"`/bin/tar --help 2>&1 | grep GNU`\" = \"\"; then\n", scriptfile);
  fputs("	tar=\"/bin/tar -xpf\"\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	tar=\"/bin/tar -xpPf\"\n", scriptfile);
  fputs("fi\n", scriptfile);

  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  fprintf(scriptfile, "	echo This installation script will patch the \'%s\'\n",
          dist->product);
  fprintf(scriptfile, "	echo software to version \'%s\' on your system.\n", dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you wish to continue? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);

  fprintf(scriptfile, "	more %s.license\n", prodname);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you agree with the terms of this license? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			*)\n", scriptfile);
  fputs("			echo Please enter yes or no.\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("		esac\n", scriptfile);
  fputs("	done\n", scriptfile);
  fputs("fi\n", scriptfile);

  write_depends(dist, scriptfile);

  fprintf(scriptfile, "if test ! -x " EPM_SOFTWARE "/%s.remove; then\n",
          prodname);
  fputs("	echo You do not appear to have the base software installed!\n",
        scriptfile);
  fputs("	echo Please install the full distribution instead.\n", scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      fprintf(scriptfile, EPM_SOFTWARE "/init.d/%s stop\n", file->dst);

  write_commands(dist, scriptfile, COMMAND_PRE_PATCH);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((file->type == 'F' || file->type == 'L') &&
        strncmp(file->dst, "/usr", 4) != 0)
      break;

  if (i)
  {
    fputs("echo Backing up old versions of non-shared files to be installed...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((file->type == 'F' || file->type == 'L') &&
          strncmp(file->dst, "/usr", 4) != 0)
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test -d $file -o -f $file -o " SYMLINK " $file; then\n", scriptfile);
    fputs("		/bin/mv -f $file $file.O\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((file->type == 'F' || file->type == 'L') &&
        strncmp(file->dst, "/usr", 4) == 0)
      break;

  if (i)
  {
    fputs("if test -w /usr ; then\n", scriptfile);
    fputs("	echo Backing up old versions of shared files to be installed...\n", scriptfile);

    fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((file->type == 'F' || file->type == 'L') &&
          strncmp(file->dst, "/usr", 4) == 0)
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("		if test -d $file -o -f $file -o " SYMLINK " $file; then\n", scriptfile);
    fputs("			/bin/mv -f $file $file.O\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
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
  fputs("if echo Write Test >/usr/.writetest 2>/dev/null; then\n", scriptfile);
  fputs("/bin/rm -f /usr/.writetest\n", scriptfile);
  fprintf(scriptfile, "	$tar %s.pss\n", prodname);
  fputs("fi\n", scriptfile);

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
    fputs("		/bin/mv -f $file.N $file\n", scriptfile);
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
    fputs("	if test -d $file.O -o -f $file.O -o " SYMLINK " $file.O; then\n", scriptfile);
    fputs("		/bin/mv -f $file.O $file\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'I')
      break;

  if (i)
  {
    fputs("echo Setting up init scripts...\n", scriptfile);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", scriptfile);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", scriptfile);
    fputs("	if test -d $dir/rc3.d -o " SYMLINK " $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("else\n", scriptfile);
    fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'I')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("		if test -d $rcdir/init.d; then\n", scriptfile);
    fputs("			/bin/rm -f $rcdir/init.d/$file\n", scriptfile);
    fputs("			/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/init.d/$file\n", scriptfile);
    fputs("		else\n", scriptfile);
    fputs("			if test -d /etc/init.d; then\n", scriptfile);
    fputs("				/bin/rm -f /etc/init.d/$file\n", scriptfile);
    fputs("				/bin/ln -s " EPM_SOFTWARE "/init.d/$file /etc/init.d/$file\n", scriptfile);
    fputs("			fi\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc0.d/K00$file\n", scriptfile);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc0.d/K00$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc2.d/S99$file\n", scriptfile);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc2.d/S99$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc3.d/S99$file\n", scriptfile);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc3.d/S99$file\n", scriptfile);
    fputs("		if test -d $rcdir/rc5.d; then\n", scriptfile);
    fputs("			/bin/rm -f $rcdir/rc5.d/S99$file\n", scriptfile);
    fputs("			/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc5.d/S99$file\n", scriptfile);
    fputs("		fi\n", scriptfile);
#ifdef __sgi
    fputs("		/etc/chkconfig -f $file on\n", scriptfile);
#endif /* __sgi */
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_PATCH);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      fprintf(scriptfile, EPM_SOFTWARE "/init.d/%s start\n", file->dst);

  fputs("echo Patching is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_remove()' - Write the removal script.
 */

static int				/* O - -1 on error, 0 on success */
write_remove(dist_t     *dist,		/* I - Software distribution */
             const char *prodname,	/* I - Product name */
	     const char *directory)	/* I - Directory */
{
  int		i;		/* Looping var */
  FILE		*scriptfile;	/* Remove script */
  char		filename[1024];	/* Name of temporary file */
  file_t	*file;		/* Software file */


  if (Verbosity)
    puts("Writing removal script...");

  sprintf(filename, "%s/%s.remove", directory, prodname);

  if ((scriptfile = write_common(dist, "Removal", filename)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create removal script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  fprintf(scriptfile, "	echo This removal script will remove the \'%s\'\n",
          dist->product);
  fprintf(scriptfile, "	echo software version \'%s\' from your system.\n",
          dist->version);
  fputs("	echo \"\"\n", scriptfile);
  fputs("	while true ; do\n", scriptfile);
  fputs("		echo $ac_n \"Do you wish to continue? $ac_c\"\n", scriptfile);
  fputs("		read yesno\n", scriptfile);
  fputs("		case \"$yesno\" in\n", scriptfile);
  fputs("			y | yes | Y | Yes | YES)\n", scriptfile);
  fputs("			break\n", scriptfile);
  fputs("			;;\n", scriptfile);
  fputs("			n | no | N | No | NO)\n", scriptfile);
  fputs("			exit 1\n", scriptfile);
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

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      fprintf(scriptfile, EPM_SOFTWARE "/init.d/%s stop\n", file->dst);

  write_commands(dist, scriptfile, COMMAND_PRE_REMOVE);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      break;

  if (i)
  {
    fputs("echo Cleaning up init scripts...\n", scriptfile);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", scriptfile);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", scriptfile);
    fputs("	if test -d $dir/rc3.d -o " SYMLINK " $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("else\n", scriptfile);
    fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/init.d/$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc0.d/K00$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc2.d/S99$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc3.d/S99$file\n", scriptfile);
    fputs("		/bin/rm -f $rcdir/rc5.d/S99$file\n", scriptfile);
#ifdef __sgi
    fputs("		/bin/rm -f /etc/config/$file\n", scriptfile);
#endif /* __sgi */
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  fputs("echo Removing/restoring installed files...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) != 0)
      break;

  if (i)
  {
    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) != 0)
	fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	rm -f $file\n", scriptfile);
    fputs("	if test -d $file.O -o -f $file.O -o " SYMLINK " $file.O; then\n", scriptfile);
    fputs("		/bin/mv -f $file.O $file\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) == 0)
      break;

  if (i)
  {
    fputs("if test -w /usr ; then\n", scriptfile);
    fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) == 0)
	fprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("		rm -f $file\n", scriptfile);
    fputs("		if test -d $file.O -o -f $file.O -o " SYMLINK " $file.O; then\n", scriptfile);
    fputs("			/bin/mv -f $file.O $file\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_REMOVE);

  fprintf(scriptfile, "rm -f " EPM_SOFTWARE "/%s.remove\n", prodname);

  fputs("echo Removal is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * End of "$Id: portable.c,v 1.24 2001/03/15 16:13:35 mike Exp $".
 */
