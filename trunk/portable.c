/*
 * "$Id: portable.c,v 1.79 2002/12/17 18:57:56 swdev Exp $"
 *
 *   Portable package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2003 by Easy Software Products.
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
 *   make_portable()      - Make a portable software distribution package.
 *   write_commands()     - Write commands.
 *   write_common()       - Write the common shell script header.
 *   write_dist()         - Write a software distribution...
 *   write_confcheck()    - Write the echo check to find the right echo options.
 *   write_install()      - Write the installation script.
 *   write_patch()        - Write the patch script.
 *   write_remove()       - Write the removal script.
 *   write_space_checks() - Write disk space checks for the installer.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	write_commands(dist_t *dist, FILE *fp, int type);
static FILE	*write_common(dist_t *dist, const char *title,
			      int rootsize, int usrsize,
		              const char *filename);
static int	write_depends(dist_t *dist, FILE *fp);
static int	write_dist(const char *title, const char *directory,
		           const char *prodname, const char *platname,
			   dist_t *dist, const char **files,
			   const char *setup, const char *types);
static int	write_confcheck(FILE *fp);
static int	write_install(dist_t *dist, const char *prodname,
			      int rootsize, int usrsize,
		              const char *directory);
static int	write_patch(dist_t *dist, const char *prodname,
			    int rootsize, int usrsize,
		            const char *directory);
static int	write_remove(dist_t *dist, const char *prodname,
			     int rootsize, int usrsize,
		             const char *directory);
static int	write_space_checks(const char *prodname, FILE *fp,
		                   const char *sw, const char *ss);


/*
 * 'make_portable()' - Make a portable software distribution package.
 */

int					/* O - 1 = success, 0 = fail */
make_portable(const char     *prodname,	/* I - Product short name */
              const char     *directory,/* I - Directory for distribution files */
              const char     *platname,	/* I - Platform name */
              dist_t         *dist,	/* I - Distribution information */
	      struct utsname *platform,	/* I - Platform information */
              const char     *setup,	/* I - Setup GUI image */
              const char     *types)	/* I - Setup GUI install types */
{
  int		i;			/* Looping var */
  int		havepatchfiles;		/* 1 if we have patch files, 0 otherwise */
  tarf_t	*tarfile;		/* Distribution tar file */
  char		swname[255],		/* Name of distribution tar file */
		pswname[255],		/* Name of patch tar file */
		filename[1024];		/* Name of temporary file */
  time_t	deftime;		/* File creation time */
  struct stat	srcstat;		/* Source file information */
  file_t	*file;			/* Software file */
  int		rootsize,		/* Size of files in root partition */
		usrsize;		/* Size of files in /usr partition */
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
    if (isupper((int)file->type))
      break;

  havepatchfiles = i > 0;

 /*
  * Copy the license and readme files...
  */

  deftime = time(NULL);

  if (Verbosity)
    puts("Copying license and readme files...");

  snprintf(filename, sizeof(filename), "%s/%s.license", directory, prodname);
  if (copy_file(filename, dist->license, 0444, getuid(), getgid()))
    return (1);

  snprintf(filename, sizeof(filename), "%s/%s.readme", directory, prodname);
  if (copy_file(filename, dist->readme, 0444, getuid(), getgid()))
    return (1);

 /*
  * Create the non-shared software distribution file...
  */

  if (Verbosity)
    puts("Creating non-shared software distribution file...");

  snprintf(swname, sizeof(swname), "%s.sw", prodname);
  snprintf(filename, sizeof(filename), "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = tar_open(filename, 0)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files, rootsize = 0;
       i > 0;
       i --, file ++)
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
	      tar_close(tarfile);
	      return (1);
	    }

            rootsize += (srcstat.st_size + 1023) / 1024;

           /*
	    * Configuration files are extracted to the config file name with
	    * .N appended; add a bit of script magic to check if the config
	    * file already exists, and if not we copy the .N to the config
	    * file location...
	    */

	    if (tolower(file->type) == 'c')
	      snprintf(filename, sizeof(filename), "%s.N", file->dst);
	    else if (tolower(file->type) == 'i')
	      snprintf(filename, sizeof(filename), "%s/init.d/%s", SoftwareDir,
	               file->dst);
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
	      tar_close(tarfile);
	      return (1);
	    }

	    if (tar_file(tarfile, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }
	    break;

	case 'd' : /* Create directory */
            if (Verbosity > 1)
	      printf("Directory %s...\n", file->dst);

            rootsize ++;
	    break;

	case 'l' : /* Link file */
            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, file->dst);

	    if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

            rootsize ++;
	    break;
      }

  tar_close(tarfile);

 /*
  * Create the shared software distribution file...
  */

  if (Verbosity)
    puts("Creating shared software distribution file...");

  snprintf(swname, sizeof(swname), "%s.ss", prodname);
  snprintf(filename, sizeof(filename), "%s/%s", directory, swname);

  unlink(filename);
  if ((tarfile = tar_open(filename, 0)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files, usrsize = 0;
       i > 0;
       i --, file ++)
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
	      tar_close(tarfile);
	      return (1);
	    }

            usrsize += (srcstat.st_size + 1023) / 1024;

           /*
	    * Configuration files are extracted to the config file name with
	    * .N appended; add a bit of script magic to check if the config
	    * file already exists, and if not we copy the .N to the config
	    * file location...
	    */

	    if (tolower(file->type) == 'c')
	      snprintf(filename, sizeof(filename), "%s.N", file->dst);
	    else if (tolower(file->type) == 'i')
	      snprintf(filename, sizeof(filename), "%s/init.d/%s", SoftwareDir,
	               file->dst);
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
	      tar_close(tarfile);
	      return (1);
	    }

	    if (tar_file(tarfile, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing file data - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }
	    break;

	case 'd' : /* Create directory */
            if (Verbosity > 1)
	      printf("%s...\n", file->dst);

	    usrsize ++;
	    break;

	case 'l' : /* Link file */
            if (Verbosity > 1)
	      printf("%s -> %s...\n", file->src, file->dst);

	    if (tar_header(tarfile, TAR_SYMLINK, file->mode, 0, deftime,
	                   file->user, file->group, file->dst, file->src) < 0)
	    {
	      fprintf(stderr, "epm: Error writing link header - %s\n",
	              strerror(errno));
	      tar_close(tarfile);
	      return (1);
	    }

	    usrsize ++;
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

    snprintf(pswname, sizeof(pswname), "%s.psw", prodname);
    snprintf(filename, sizeof(filename), "%s/%s", directory, pswname);

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
		tar_close(tarfile);
		return (1);
	      }

             /*
	      * Configuration files are extracted to the config file name with
	      * .N appended; add a bit of script magic to check if the config
	      * file already exists, and if not we copy the .N to the config
	      * file location...
	      */

	      if (file->type == 'C')
		snprintf(filename, sizeof(filename), "%s.N", file->dst);
	      else if (file->type == 'I')
		snprintf(filename, sizeof(filename), "%s/init.d/%s", SoftwareDir,
		         file->dst);
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
		tar_close(tarfile);
		return (1);
	      }

	      if (tar_file(tarfile, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing file data - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
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
		tar_close(tarfile);
		return (1);
	      }
	      break;
	}

    tar_close(tarfile);

    if (Verbosity)
      puts("Creating shared software patch file...");

    snprintf(pswname, sizeof(pswname), "%s.pss", prodname);
    snprintf(filename, sizeof(filename), "%s/%s", directory, pswname);

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
		tar_close(tarfile);
		return (1);
	      }

             /*
	      * Configuration files are extracted to the config file name with
	      * .N appended; add a bit of script magic to check if the config
	      * file already exists, and if not we copy the .N to the config
	      * file location...
	      */

	      if (file->type == 'C')
		snprintf(filename, sizeof(filename), "%s.N", file->dst);
	      else if (file->type == 'I')
		snprintf(filename, sizeof(filename), "%s/init.d/%s",
		         SoftwareDir, file->dst);
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
		tar_close(tarfile);
		return (1);
	      }

	      if (tar_file(tarfile, file->src) < 0)
	      {
		fprintf(stderr, "epm: Error writing file data - %s\n",
	        	strerror(errno));
		tar_close(tarfile);
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
		tar_close(tarfile);
		return (1);
	      }
	      break;
	}

    tar_close(tarfile);
  }

 /*
  * Create the scripts...
  */

  if (write_install(dist, prodname, rootsize, usrsize, directory))
    return (1);

  if (havepatchfiles)
    if (write_patch(dist, prodname, rootsize, usrsize, directory))
      return (1);

  if (write_remove(dist, prodname, rootsize, usrsize, directory))
    return (1);

 /*
  * Create the distribution archives...
  */

  if (write_dist("distribution", directory, prodname, platname, dist,
                 distfiles, setup, types))
    return (1);

  if (havepatchfiles)
  {
    snprintf(filename, sizeof(filename), "%s-patch", dist->version);
    strcpy(dist->version, filename);

    if (write_dist("patch", directory, prodname, platname, dist, patchfiles,
                   setup, types))
      return (1);
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
               int    type)		/* I - Type of commands to write */
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
             int        rootsize,	/* I - Size of root files in kbytes */
	     int        usrsize,	/* I - Size of /usr files in kbytes */
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
  fprintf(fp, "#%%rootsize %d\n", rootsize);
  fprintf(fp, "#%%usrsize %d\n", usrsize);
  fputs("#\n", fp);

  fputs("PATH=/usr/xpg4/bin:/bin:/usr/bin:/usr/ucb:${PATH}\n", fp);
  fputs("SHELL=/bin/sh\n", fp);
  fputs("case \"`id`\" in\n", fp);
  fputs("\tuid=0*)\n", fp);
  fputs("\t\t;;\n", fp);
  fputs("\t*)\n", fp);
  fprintf(fp, "\t\techo Sorry, you must be root to %s this software.\n",
          title[0] == 'I' ? "install" : title[0] == 'R' ? "remove" : "patch");
  fputs("\t\texit 1\n", fp);
  fputs("\t\t;;\n", fp);
  fputs("esac\n", fp);
  qprintf(fp, "echo Copyright %s\n", dist->copyright);
  fprintf(fp, "# Reset umask for %s...\n",
          title[0] == 'I' ? "install" : title[0] == 'R' ? "remove" : "patch");
  fputs("umask 002\n", fp);

  write_confcheck(fp);

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
			  "replaces",
			  "provides"
			};


  for (i = 0, d= dist->depends; i < dist->num_depends; i ++, d ++)
  {
    fprintf(fp, "#%%%s %s %d %d\n", depends[(int)d->type], d->product,
            d->vernumber[0], d->vernumber[1]);

    switch (d->type)
    {
      case DEPEND_REQUIRES :
          if (d->product[0] == '/')
          {
           /*
            * Require a file...
            */

            qprintf(fp, "if test ! -r %s -a ! -h %s; then\n",
                    d->product, d->product);
            qprintf(fp, "	echo Sorry, you must first install \\'%s\\'!\n",
	            d->product);
            fputs("	exit 1\n", fp);
            fputs("fi\n", fp);
          }
          else
          {
           /*
            * Require a product...
            */

            fprintf(fp, "if test ! -x %s/%s.remove; then\n",
                    SoftwareDir, d->product);
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

            if (d->vernumber[0] > 0 || d->vernumber[1] < INT_MAX)
	    {
	     /*
	      * Do version number checking...
	      */

              fprintf(fp, "installed=`grep \'^#%%version\' "
	                  "%s/%s.remove | awk \'{print $3}\'`\n",
                      SoftwareDir, d->product);

              fputs("if test x$installed = x; then\n", fp);
	      fputs("	installed=0\n", fp);
	      fputs("fi\n", fp);

	      fprintf(fp, "if test $installed -lt %d -o $installed -gt %d; then\n",
	              d->vernumber[0], d->vernumber[1]);
              fprintf(fp, "	if test -x %s.install; then\n",
                      d->product);
              fprintf(fp, "		echo Installing required %s software...\n",
                      d->product);
              fprintf(fp, "		./%s.install now\n", d->product);
              fputs("	else\n", fp);
              fprintf(fp, "		echo Sorry, you must first install \\'%s\\' version %s to %s!\n",
	              d->product, d->version[0], d->version[1]);
              fputs("		exit 1\n", fp);
              fputs("	fi\n", fp);
              fputs("fi\n", fp);
	    }
          }
	  break;

      case DEPEND_INCOMPAT :
          if (d->product[0] == '/')
          {
           /*
            * Incompatible with a file...
            */

            qprintf(fp, "if test -r %s -o -h %s; then\n",
                    d->product, d->product);
            qprintf(fp, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
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

            fprintf(fp, "if test -x %s/%s.remove; then\n",
                    SoftwareDir, d->product);

            if (d->vernumber[0] > 0 || d->vernumber[1] < INT_MAX)
	    {
	     /*
	      * Do version number checking...
	      */

              fprintf(fp, "	installed=`grep \'^#%%version\' "
	                  "%s/%s.remove | awk \'{print $3}\'`\n",
		      SoftwareDir, d->product);

              fputs("	if test x$installed = x; then\n", fp);
	      fputs("		installed=0\n", fp);
	      fputs("	fi\n", fp);

	      fprintf(fp, "	if test $installed -ge %d -a $installed -le %d; then\n",
	              d->vernumber[0], d->vernumber[1]);
              fprintf(fp, "		echo Sorry, this software is incompatible with \\'%s\\' version %s to %s!\n",
	              d->product, d->version[0], d->version[1]);
              fprintf(fp, "		echo Please remove it first by running \\'%s/%s.remove\\'.\n",
	              SoftwareDir, d->product);
              fputs("		exit 1\n", fp);
              fputs("	fi\n", fp);
	    }
	    else
	    {
              fprintf(fp, "	echo Sorry, this software is incompatible with \\'%s\\'!\n",
	              d->product);
              fprintf(fp, "	echo Please remove it first by running \\'%s/%s.remove\\'.\n",
	              SoftwareDir, d->product);
              fputs("	exit 1\n", fp);
	    }

            fputs("fi\n", fp);
          }
	  break;

      case DEPEND_REPLACES :
          fprintf(fp, "if test -x %s/%s.remove; then\n", SoftwareDir,
	          d->product);

          if (d->vernumber[0] > 0 || d->vernumber[1] < INT_MAX)
	  {
	   /*
	    * Do version number checking...
	    */

            fprintf(fp, "	installed=`grep \'^#%%version\' "
	                "%s/%s.remove | awk \'{print $3}\'`\n",
                    SoftwareDir, d->product);

            fputs("	if test x$installed = x; then\n", fp);
	    fputs("		installed=0\n", fp);
	    fputs("	fi\n", fp);

	    fprintf(fp, "	if test $installed -ge %d -a $installed -le %d; then\n",
	            d->vernumber[0], d->vernumber[1]);
            fprintf(fp, "		echo Automatically replacing \\'%s\\'...\n",
	            d->product);
            fprintf(fp, "		%s/%s.remove now\n",
	            SoftwareDir, d->product);
            fputs("	fi\n", fp);
	  }
	  else
	  {
            fprintf(fp, "	echo Automatically replacing \\'%s\\'...\n",
	            d->product);
            fprintf(fp, "	%s/%s.remove now\n",
	            SoftwareDir, d->product);
          }

          fputs("fi\n", fp);
	  break;
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
           const char *setup,		/* I - Setup GUI image */
           const char *types)		/* I - Setup GUI install types */
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

  if (dist->relnumber)
  {
    if (platname[0])
      snprintf(filename, sizeof(filename), "%s/%s-%s-%d-%s.tar.gz", directory, prodname,
              dist->version, dist->relnumber, platname);
    else
      snprintf(filename, sizeof(filename), "%s/%s-%s-%d.tar.gz", directory, prodname,
              dist->version, dist->relnumber);
  }
  else if (platname[0])
    snprintf(filename, sizeof(filename), "%s/%s-%s-%s.tar.gz", directory, prodname,
            dist->version, platname);
  else
    snprintf(filename, sizeof(filename), "%s/%s-%s.tar.gz", directory, prodname, dist->version);

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
    snprintf(srcname, sizeof(srcname), "%s/%s.%s", directory, prodname, files[i]);
    snprintf(dstname, sizeof(dstname), "%s.%s", prodname, files[i]);

    stat(srcname, &srcstat);

    if (srcstat.st_size == 0)
      continue;

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

   /*
    * And the types file...
    */

    if (types)
    {
      stat(types, &srcstat);
      if (tar_header(tarfile, TAR_NORMAL, 0444, srcstat.st_size,
	             srcstat.st_mtime, "root", "root", "setup.types", NULL) < 0)
      {
	if (Verbosity)
          puts("");

	fprintf(stderr, "epm: Error writing file header - %s\n",
		strerror(errno));
	return (-1);
      }

      if (tar_file(tarfile, types) < 0)
      {
	if (Verbosity)
          puts("");

	fprintf(stderr, "epm: Error writing file data for setup.types -\n    %s\n",
		strerror(errno));
	return (-1);
      }

      if (Verbosity)
      {
	printf(" setup.types");
	fflush(stdout);
      }
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
 * 'write_confcheck()' - Write the echo check to find the right echo options.
 */

static int				/* O - -1 on error, 0 on success */
write_confcheck(FILE *fp)		/* I - Script file */
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

 /*
  * This is a check for the correct options to use with the "tar"
  * command.
  */

  fputs("# Determine correct extract options for the tar command...\n", fp);
  fputs("if test `uname` = Darwin; then\n", fp);
  fputs("	ac_tar=\"tar -xpPf\"\n", fp);
  fputs("else if test \"`tar --help 2>&1 | grep GNU`\" = \"\"; then\n", fp);
  fputs("	ac_tar=\"tar -xpf\"\n", fp);
  fputs("else\n", fp);
  fputs("	ac_tar=\"tar -xpPf\"\n", fp);
  fputs("fi fi\n", fp);

  return (0);
}


/*
 * 'write_install()' - Write the installation script.
 */

static int				/* O - -1 on error, 0 on success */
write_install(dist_t     *dist,		/* I - Software distribution */
              const char *prodname,	/* I - Product name */
              int        rootsize,	/* I - Size of root files in kbytes */
	      int        usrsize,	/* I - Size of /usr files in kbytes */
	      const char *directory)	/* I - Directory */
{
  int		i;			/* Looping var */
  int		col;			/* Column in the output */
  FILE		*scriptfile;		/* Install script */
  char		filename[1024];		/* Name of temporary file */
  file_t	*file;			/* Software file */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */


  if (Verbosity)
    puts("Writing installation script...");

  snprintf(filename, sizeof(filename), "%s/%s.install", directory, prodname);

  if ((scriptfile = write_common(dist, "Installation", rootsize, usrsize,
                                 filename)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create installation script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo Software license silently accepted via command-line option.\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  qprintf(scriptfile, "	echo This installation script will install the \'%s\'\n",
          dist->product);
  qprintf(scriptfile, "	echo software version \'%s\' on your system.\n",
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
  fprintf(scriptfile, "if test -x %s/%s.remove; then\n", SoftwareDir, prodname);
  fprintf(scriptfile, "	echo Removing old versions of %s software...\n",
          prodname);
  fprintf(scriptfile, "	%s/%s.remove now\n", SoftwareDir, prodname);
  fputs("fi\n", scriptfile);

  write_space_checks(prodname, scriptfile, rootsize ? "sw" : NULL,
                     usrsize ? "ss" : NULL);
  write_depends(dist, scriptfile);
  write_commands(dist, scriptfile, COMMAND_PRE_INSTALL);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) != 0)
      break;

  if (i)
  {
    fputs("echo Backing up old versions of non-shared files to be installed...\n", scriptfile);

    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) != 0)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	if test -d \"$file\" -o -f \"$file\" -o -h \"$file\"; then\n", scriptfile);
    fputs("		mv -f \"$file\" \"$file.O\"\n", scriptfile);
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

    col = fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) == 0)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("		if test -d \"$file\" -o -f \"$file\" -o -h \"$file\"; then\n", scriptfile);
    fputs("			mv -f \"$file\" \"$file.O\"\n", scriptfile);
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
	qprintf(scriptfile, "if test ! -d %s -a ! -f %s -a ! -h %s; then\n",
        	file->dst, file->dst, file->dst);
	qprintf(scriptfile, "	mkdir -p %s\n", file->dst);
	fputs("else\n", scriptfile);
	qprintf(scriptfile, "	if test -f %s; then\n", file->dst);
	qprintf(scriptfile, "		echo Error: %s already exists as a regular file!\n",
	        file->dst);
	fputs("		exit 1\n", scriptfile);
	fputs("	fi\n", scriptfile);
	fputs("fi\n", scriptfile);
	qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	qprintf(scriptfile, "chmod %o %s\n", file->mode, file->dst);
      }
  }

  fputs("echo Installing software...\n", scriptfile);

  if (rootsize)
  {
    fprintf(scriptfile, "$ac_tar %s.sw\n", prodname);
  }

  if (usrsize)
  {
    fputs("if echo Write Test >/usr/.writetest 2>/dev/null; then\n", scriptfile);
    fprintf(scriptfile, "	$ac_tar %s.ss\n", prodname);
    fputs("fi\n", scriptfile);
  }

  fprintf(scriptfile, "if test -d %s; then\n", SoftwareDir);
  fprintf(scriptfile, "	rm -f %s/%s.remove\n", SoftwareDir, prodname);
  fputs("else\n", scriptfile);
  fprintf(scriptfile, "	mkdir -p %s\n", SoftwareDir);
  fputs("fi\n", scriptfile);
  fprintf(scriptfile, "cp %s.remove %s\n", prodname, SoftwareDir);
  fprintf(scriptfile, "chmod 544 %s/%s.remove\n", SoftwareDir, prodname);

  fputs("echo Updating file permissions...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) != 0 &&
        strcmp(file->user, "root") != 0)
      switch (tolower(file->type))
      {
	case 'c' :
	case 'f' :
	    qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	    break;
      }

  fputs("if test -f /usr/.writetest; then\n", scriptfile);
  fputs("	rm -f /usr/.writetest\n", scriptfile);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) == 0 &&
        strcmp(file->user, "root") != 0)
      switch (tolower(file->type))
      {
	case 'c' :
	case 'f' :
	    qprintf(scriptfile, "	chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "	chgrp %s %s\n", file->group, file->dst);
	    break;
      }
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
      break;

  if (i)
  {
    fputs("echo Checking configuration files...\n", scriptfile);

    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'c')
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	if test ! -f \"$file\"; then\n", scriptfile);
    fputs("		cp \"$file.N\" \"$file\"\n", scriptfile);
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
    fputs("	if test -d $dir/rc2.d -o -h $dir/rc2.d -o "
          "-d $dir/rc3.d -o -h $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	if test -d /usr/local/etc/rc.d; then\n", scriptfile);
    fputs("		for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        qprintf(scriptfile, " %s", file->dst);
    fputs("; do\n", scriptfile);
    fputs("			rm -f /usr/local/src/rc.d/$file.sh\n", scriptfile);
    qprintf(scriptfile, "			ln -s %s/init.d/$file "
                        "/usr/local/etc/rc.d/$file.sh\n",
            SoftwareDir);
    fputs("		done\n", scriptfile);
    fputs("	else\n", scriptfile);
    fputs("		echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("else\n", scriptfile);
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
	fputs("	if test -d $rcdir/init.d; then\n", scriptfile);
	qprintf(scriptfile, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	qprintf(scriptfile, "		/bin/ln -s %s/init.d/%s "
                    "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("	else\n", scriptfile);
	fputs("		if test -d /etc/init.d; then\n", scriptfile);
	qprintf(scriptfile, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	qprintf(scriptfile, "			/bin/ln -s %s/init.d/%s "
                    "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("		fi\n", scriptfile);
	fputs("	fi\n", scriptfile);

#ifndef __sun
	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels);
	     runlevels ++)
#else
	for (runlevels = get_runlevels(dist->files + i, "03");
             isdigit(*runlevels);
	     runlevels ++)
#endif /* !__sun */
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(scriptfile, "	/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  qprintf(scriptfile, "	/bin/ln -s %s/init.d/%s "
                      "$rcdir/rc%c.d/%c%02d%s\n", SoftwareDir, file->dst,
		  *runlevels, *runlevels == '0' ? 'K' : 'S', number, file->dst);
        }
      }

    fputs("	if test -x /etc/chkconfig; then\n", scriptfile);
    qprintf(scriptfile, "		/etc/chkconfig -f %s on\n", file->dst);
    fputs("	fi\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_INSTALL);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      qprintf(scriptfile, "%s/init.d/%s start\n", SoftwareDir, file->dst);

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
            int        rootsize,	/* I - Size of root files in kbytes */
	    int        usrsize,		/* I - Size of /usr files in kbytes */
	    const char *directory)	/* I - Directory */
{
  int		i;			/* Looping var */
  FILE		*scriptfile;		/* Patch script */
  char		filename[1024];		/* Name of temporary file */
  file_t	*file;			/* Software file */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */


  if (Verbosity)
    puts("Writing patch script...");

  snprintf(filename, sizeof(filename), "%s/%s.patch", directory, prodname);

  if ((scriptfile = write_common(dist, "Patch", rootsize, usrsize,
                                 filename)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create patch script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo Software license silently accepted via command-line option.\n", scriptfile);
  fputs("else\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  qprintf(scriptfile, "	echo This installation script will patch the \'%s\'\n",
          dist->product);
  qprintf(scriptfile, "	echo software to version \'%s\' on your system.\n", dist->version);
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

  write_space_checks(prodname, scriptfile, rootsize ? "psw" : NULL,
                     usrsize ? "pss" : NULL);
  write_depends(dist, scriptfile);

  fprintf(scriptfile, "if test ! -x %s/%s.remove; then\n",
          SoftwareDir, prodname);
  fputs("	echo You do not appear to have the base software installed!\n",
        scriptfile);
  fputs("	echo Please install the full distribution instead.\n", scriptfile);
  fputs("	exit 1\n", scriptfile);
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      qprintf(scriptfile, "%s/init.d/%s stop\n", SoftwareDir, file->dst);

  write_commands(dist, scriptfile, COMMAND_PRE_PATCH);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'D')
      break;

  if (i)
  {
    fputs("echo Creating new installation directories...\n", scriptfile);

    for (; i > 0; i --, file ++)
      if (file->type == 'D')
      {
	qprintf(scriptfile, "if test ! -d %s -a ! -f %s -a ! -h %s; then\n",
        	file->dst, file->dst, file->dst);
	qprintf(scriptfile, "	mkdir -p %s\n", file->dst);
	fputs("else\n", scriptfile);
	qprintf(scriptfile, "	if test -f %s; then\n", file->dst);
	qprintf(scriptfile, "		echo Error: %s already exists as a regular file!\n",
	        file->dst);
	fputs("		exit 1\n", scriptfile);
	fputs("	fi\n", scriptfile);
	fputs("fi\n", scriptfile);
	qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	qprintf(scriptfile, "chmod %o %s\n", file->mode, file->dst);
      }
  }

  fputs("echo Patching software...\n", scriptfile);

  if (rootsize)
  {
    fprintf(scriptfile, "$ac_tar %s.psw\n", prodname);
  }

  if (usrsize)
  {
    fputs("if echo Write Test >/usr/.writetest 2>/dev/null; then\n", scriptfile);
    fprintf(scriptfile, "	$ac_tar %s.pss\n", prodname);
    fputs("fi\n", scriptfile);
  }

  fprintf(scriptfile, "rm -f %s/%s.remove\n", SoftwareDir, prodname);
  fprintf(scriptfile, "cp %s.remove %s\n", prodname, SoftwareDir);
  fprintf(scriptfile, "chmod 544 %s/%s.remove\n", SoftwareDir, prodname);

  fputs("echo Updating file permissions...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) != 0 &&
        strcmp(file->user, "root") != 0)
      switch (file->type)
      {
	case 'C' :
	case 'F' :
	    qprintf(scriptfile, "chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "chgrp %s %s\n", file->group, file->dst);
	    break;
      }

  fputs("if test -f /usr/.writetest; then\n", scriptfile);
  fputs("	rm -f /usr/.writetest\n", scriptfile);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr", 4) == 0 &&
        strcmp(file->user, "root") != 0)
      switch (file->type)
      {
	case 'C' :
	case 'F' :
	    qprintf(scriptfile, "	chown %s %s\n", file->user, file->dst);
	    qprintf(scriptfile, "	chgrp %s %s\n", file->group, file->dst);
	    break;
      }
  fputs("fi\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->type == 'C')
      break;

  if (i)
  {
    fputs("echo Checking configuration files...\n", scriptfile);

    fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (file->type == 'C')
        qprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	if test ! -f \"$file\"; then\n", scriptfile);
    fputs("		cp \"$file.N\" \"$file\"\n", scriptfile);
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
        qprintf(scriptfile, " %s", file->dst);

    fputs("; do\n", scriptfile);
    fputs("	rm -f \"$file\"\n", scriptfile);
    fputs("	if test -d \"$file.O\" -o -f \"$file.O\" -o -h \"$file.O\"; then\n", scriptfile);
    fputs("		mv -f \"$file.O\" \"$file\"\n", scriptfile);
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
    fputs("	if test -d $dir/rc2.d -o -h $dir/rc2.d -o "
          "-d $dir/rc3.d -o -h $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	if test -d /usr/local/etc/rc.d; then\n", scriptfile);
    fputs("		for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'I')
        qprintf(scriptfile, " %s", file->dst);
    fputs("; do\n", scriptfile);
    fputs("			rm -f /usr/local/src/rc.d/$file.sh\n", scriptfile);
    qprintf(scriptfile, "			ln -s %s/init.d/$file "
                        "/usr/local/etc/rc.d/$file.sh\n",
            SoftwareDir);
    fputs("		done\n", scriptfile);
    fputs("	else\n", scriptfile);
    fputs("		echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("else\n", scriptfile);
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
	fputs("	if test -d $rcdir/init.d; then\n", scriptfile);
	qprintf(scriptfile, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	qprintf(scriptfile, "		/bin/ln -s %s/init.d/%s "
                    "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("	else\n", scriptfile);
	fputs("		if test -d /etc/init.d; then\n", scriptfile);
	qprintf(scriptfile, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	qprintf(scriptfile, "			/bin/ln -s %s/init.d/%s "
                    "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("		fi\n", scriptfile);
	fputs("	fi\n", scriptfile);

#ifndef __sun
	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels);
	     runlevels ++)
#else
	for (runlevels = get_runlevels(dist->files + i, "03");
             isdigit(*runlevels);
	     runlevels ++)
#endif /* !__sun */
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(scriptfile, "	/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  qprintf(scriptfile, "	/bin/ln -s %s/init.d/%s "
                      "$rcdir/rc%c.d/%c%02d%s\n", SoftwareDir, file->dst,
		  *runlevels, *runlevels == '0' ? 'K' : 'S', number, file->dst);
        }
      }

    fputs("	if test -x /etc/chkconfig; then\n", scriptfile);
    qprintf(scriptfile, "		/etc/chkconfig -f %s on\n", file->dst);
    fputs("	fi\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_PATCH);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      qprintf(scriptfile, "%s/init.d/%s start\n", SoftwareDir, file->dst);

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
             int        rootsize,	/* I - Size of root files in kbytes */
	     int        usrsize,	/* I - Size of /usr files in kbytes */
	     const char *directory)	/* I - Directory */
{
  int		i;			/* Looping var */
  int		col;			/* Current column */
  FILE		*scriptfile;		/* Remove script */
  char		filename[1024];		/* Name of temporary file */
  file_t	*file;			/* Software file */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */


  if (Verbosity)
    puts("Writing removal script...");

  snprintf(filename, sizeof(filename), "%s/%s.remove", directory, prodname);

  if ((scriptfile = write_common(dist, "Removal", rootsize, usrsize,
                                 filename)) == NULL)
  {
    fprintf(stderr, "epm: Unable to create removal script \"%s\" -\n"
                    "     %s\n", filename, strerror(errno));
    return (-1);
  }

  fputs("if test ! \"$*\" = \"now\"; then\n", scriptfile);
  fputs("	echo \"\"\n", scriptfile);
  qprintf(scriptfile, "	echo This removal script will remove the \'%s\'\n",
          dist->product);
  qprintf(scriptfile, "	echo software version \'%s\' from your system.\n",
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
      qprintf(scriptfile, "%s/init.d/%s stop\n", SoftwareDir, file->dst);

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
    fputs("	if test -d $dir/rc2.d -o -h $dir/rc2.d -o "
          "-d $dir/rc3.d -o -h $dir/rc3.d; then\n", scriptfile);
    fputs("		rcdir=\"$dir\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("done\n", scriptfile);
    fputs("if test \"$rcdir\" = \"\" ; then\n", scriptfile);
    fputs("	if test -d /usr/local/etc/rc.d; then\n", scriptfile);
    fputs("		for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        qprintf(scriptfile, " %s", file->dst);
    fputs("; do\n", scriptfile);
    fputs("			rm -f /usr/local/src/rc.d/$file.sh\n", scriptfile);
    fputs("		done\n", scriptfile);
    fputs("	else\n", scriptfile);
    fputs("		echo Unable to determine location of startup scripts!\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("else\n", scriptfile);
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
        qprintf(scriptfile, "	%s/init.d/%s stop\n", SoftwareDir, file->dst);

	fputs("	if test -d $rcdir/init.d; then\n", scriptfile);
	qprintf(scriptfile, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	fputs("	else\n", scriptfile);
	fputs("		if test -d /etc/init.d; then\n", scriptfile);
	qprintf(scriptfile, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	fputs("		fi\n", scriptfile);
	fputs("	fi\n", scriptfile);

#ifndef __sun
	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels);
	     runlevels ++)
#else
	for (runlevels = get_runlevels(dist->files + i, "03");
             isdigit(*runlevels);
	     runlevels ++)
#endif /* !__sun */
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(scriptfile, "	/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          *runlevels == '0' ? 'K' : 'S', number, file->dst);
        }
      }

    fputs("	if test -x /etc/chkconfig; then\n", scriptfile);
    qprintf(scriptfile, "		rm -f /etc/config/%s\n", file->dst);
    fputs("	fi\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  fputs("echo Removing/restoring installed files...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
        strncmp(file->dst, "/usr", 4) != 0)
      break;

  if (i)
  {
    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) != 0)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	rm -f \"$file\"\n", scriptfile);
    fputs("	if test -d \"$file.O\" -o -f \"$file.O\" -o -h \"$file.O\"; then\n", scriptfile);
    fputs("		mv -f \"$file.O\" \"$file\"\n", scriptfile);
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
    col = fputs("	for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if ((tolower(file->type) == 'f' || tolower(file->type) == 'l') &&
          strncmp(file->dst, "/usr", 4) == 0)
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("		rm -f \"$file\"\n", scriptfile);
    fputs("		if test -d \"$file.O\" -o -f \"$file.O\" -o -h \"$file.O\"; then\n", scriptfile);
    fputs("			mv -f \"$file.O\" \"$file\"\n", scriptfile);
    fputs("		fi\n", scriptfile);
    fputs("	done\n", scriptfile);
    fputs("fi\n", scriptfile);
  }

  fputs("echo Checking configuration files...\n", scriptfile);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
      break;

  if (i)
  {
    col = fputs("for file in", scriptfile);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'c')
      {
        if (col > 80)
	  col = qprintf(scriptfile, " \\\n%s", file->dst) - 2;
	else
          col += qprintf(scriptfile, " %s", file->dst);
      }

    fputs("; do\n", scriptfile);
    fputs("	if cmp -s \"$file\" \"$file.N\"; then\n", scriptfile);
    fputs("		# Config file not changed\n", scriptfile);
    fputs("		rm -f \"$file\"\n", scriptfile);
    fputs("	fi\n", scriptfile);
    fputs("	rm -f \"$file.N\"\n", scriptfile);
    fputs("done\n", scriptfile);
  }

  write_commands(dist, scriptfile, COMMAND_POST_REMOVE);

  fprintf(scriptfile, "rm -f %s/%s.remove\n", SoftwareDir, prodname);

  fputs("echo Removal is complete.\n", scriptfile);

  fclose(scriptfile);

  return (0);
}


/*
 * 'write_space_checks()' - Write disk space checks for the installer.
 */

static int				/* O - 0 on success, -1 on error */
write_space_checks(const char *prodname,/* I - Distribution name */
                   FILE       *fp,	/* I - File to write to */
                   const char *sw,	/* I - / archive */
		   const char *ss)	/* I - /usr archive */
{
  fputs("case `uname` in\n", fp);
  fputs("	AIX)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $15}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $11}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $15}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $11}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	HP-UX)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $1}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $9}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $1}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $9}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	IRIX*)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $15}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $13}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $15}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $13}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	SCO*)\n", fp);
  fputs("	dfroot=`df -k -B / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k -B /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $13}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $11}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $13}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $11}'`\n", fp);
  fputs("	;;\n\n", fp);
  fputs("	*)\n", fp);
  fputs("	dfroot=`df -k / | tr '\\n' ' '`\n", fp);
  fputs("	dfusr=`df -k /usr | tr '\\n' ' '`\n", fp);
  fputs("	fsroot=`echo $dfroot | awk '{print $13}'`\n", fp);
  fputs("	sproot=`echo $dfroot | awk '{print $11}'`\n", fp);
  fputs("	fsusr=`echo $dfusr | awk '{print $13}'`\n", fp);
  fputs("	spusr=`echo $dfusr | awk '{print $11}'`\n", fp);
  fputs("	;;\n", fp);
  fputs("esac\n", fp);
  fputs("\n", fp);

  if (sw)
  {
    fprintf(fp, "temp=`ls -ln %s.%s | awk '{print $5}'`\n", prodname, sw);
    fputs("spsw=`expr $temp / 1024`\n", fp);
  }
  else
    fputs("spsw=0\n", fp);
  fputs("\n", fp);

  if (ss)
  {
    fprintf(fp, "temp=`ls -ln %s.%s | awk '{print $5}'`\n", prodname, ss);
    fputs("spss=`expr $temp / 1024`\n", fp);
  }
  else
    fputs("spss=0\n", fp);
  fputs("\n", fp);

  fputs("spall=`expr $spsw + $spss`\n", fp);
  fputs("\n", fp);

  fputs("if test x$sproot = x -o x$spusr = x; then\n", fp);
  fputs("	echo WARNING: Unable to determine available disk space; installing blindly...\n", fp);
  fputs("else\n", fp);
  fputs("	if test x$fsroot = x$fsusr; then\n", fp);
  fputs("		if test $spall -gt $sproot; then\n", fp);
  fputs("			echo Not enough free disk space for software:\n", fp);
  fputs("			echo You need $spall kbytes but only have $sproot kbytes available.\n", fp);
  fputs("			exit 1\n", fp);
  fputs("		fi\n", fp);
  fputs("	else\n", fp);
  fputs("		if test $spsw -gt $sproot; then\n", fp);
  fputs("			echo Not enough free disk space for software:\n", fp);
  fputs("			echo You need $spsw kbytes in / but only have $sproot kbytes available.\n", fp);
  fputs("			exit 1\n", fp);
  fputs("		fi\n", fp);
  fputs("\n", fp);
  fputs("		if test $spss -gt $spusr; then\n", fp);
  fputs("			echo Not enough free disk space for software:\n", fp);
  fputs("			echo You need $spss kbytes in /usr but only have $spusr kbytes available.\n", fp);
  fputs("			exit 1\n", fp);
  fputs("		fi\n", fp);
  fputs("	fi\n", fp);
  fputs("fi\n", fp);

  return (0);
}


/*
 * End of "$Id: portable.c,v 1.79 2002/12/17 18:57:56 swdev Exp $".
 */
