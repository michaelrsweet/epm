/*
 * "$Id: pkg.c,v 1.11 2001/02/15 13:34:16 mike Exp $"
 *
 *   AT&T package gateway for the ESP Package Manager (EPM).
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
 *   make_pkg() - Make an AT&T software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_pkg()' - Make an AT&T software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_pkg(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int		i, j;			/* Looping vars */
  FILE		*fp;			/* Control file */
  char		name[1024];		/* Full product name */
  char		filename[1024];		/* Destination filename */
  char		command[1024];		/* Command to run */
  char		current[1024];		/* Current directory */
  file_t	*file;			/* Current distribution file */
  tarf_t	*tarfile;		/* Distribution file */
  time_t	curtime;		/* Current time info */
  struct tm	*curdate;		/* Current date info */


  if (Verbosity)
    puts("Creating PKG distribution...");

  if (platname[0])
    sprintf(name, "%s-%s-%s", prodname, dist->version, platname);
  else
    sprintf(name, "%s-%s", prodname, dist->version);

  getcwd(current, sizeof(current));

 /*
  * Write the pkginfo file for pkgmk...
  */

  if (Verbosity)
    puts("Creating package information file...");

  sprintf(filename, "%s/%s.pkginfo", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create package information file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  curtime = time(NULL);
  curdate = gmtime(&curtime);

  fprintf(fp, "PKG=%s\n", prodname);
  fprintf(fp, "NAME=%s\n", dist->product);
  fprintf(fp, "VERSION=%s\n", dist->version);
  fprintf(fp, "VENDOR=%s\n", dist->vendor);
  fprintf(fp, "PSTAMP=epm%04d%02d%02d%02d%02d%02d\n",
          curdate->tm_year + 1900, curdate->tm_mon + 1, curdate->tm_mday,
	  curdate->tm_hour, curdate->tm_min, curdate->tm_sec);

  if (dist->num_descriptions > 0)
    fprintf(fp, "DESC=%s\n", dist->descriptions[0]);

  fputs("CATEGORY=application\n", fp);
  fputs("CLASSES=none\n", fp);

  if (strcmp(platform->machine, "intel") == 0)
    fputs("ARCH=i86pc\n", fp);
  else
    fputs("ARCH=sparc\n", fp);

  fclose(fp);

 /*
  * Write the depend file for pkgmk...
  */

  if (Verbosity)
    puts("Creating package dependency file...");

  sprintf(filename, "%s/%s.depend", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create package dependency file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = 0; i < dist->num_requires; i ++)
    if (dist->requires[i][0] != '/')
      fprintf(fp, "P %s\n", dist->requires[i]);

  for (i = 0; i < dist->num_incompats; i ++)
    if (dist->incompats[i][0] != '/')
      fprintf(fp, "I %s\n", dist->incompats[i]);

  for (i = 0; i < dist->num_replaces; i ++)
    if (dist->replaces[i][0] != '/')
      fprintf(fp, "I %s\n", dist->replaces[i]);

  fclose(fp);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      break;

  if (dist->num_installs || i)
  {
   /*
    * Write the postinstall file for pkgmk...
    */

    if (Verbosity)
      puts("Creating postinstall script...");

    sprintf(filename, "%s/%s.postinstall", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (j = 0; j < dist->num_installs; j ++)
      fprintf(fp, "%s\n", dist->installs[j]);

    for (j = dist->num_files, file = dist->files; j > 0; j --, file ++)
      if (tolower(file->type) == 'i')
	fprintf(fp, "/etc/init.d/%s start\n", file->dst);

    fclose(fp);
  }

  if (dist->num_removes || i)
  {
   /*
    * Write the preremove file for pkgmk...
    */

    if (Verbosity)
      puts("Creating preremove script...");

    sprintf(filename, "%s/%s.preremove", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (j = dist->num_files, file = dist->files; j > 0; j --, file ++)
      if (tolower(file->type) == 'i')
	fprintf(fp, "/etc/init.d/%s stop\n", file->dst);

    for (j = 0; j < dist->num_removes; j ++)
      fprintf(fp, "%s\n", dist->removes[j]);

    fclose(fp);
  }

 /*
  * Add symlinks for init scripts...
  */

  for (i = 0; i < dist->num_files; i ++)
    if (tolower(dist->files[i].type) == 'i')
    {
      file = add_file(dist);
      file->type = 'l';
      file->mode = 0;
      strcpy(file->user, "root");
      strcpy(file->group, "sys");
      sprintf(file->src, "../init.d/%s", dist->files[i].dst);
      sprintf(file->dst, "/etc/rc0.d/K00%s", dist->files[i].dst);

      file = add_file(dist);
      file->type = 'l';
      file->mode = 0;
      strcpy(file->user, "root");
      strcpy(file->group, "sys");
      sprintf(file->src, "../init.d/%s", dist->files[i].dst);
      sprintf(file->dst, "/etc/rc3.d/S99%s", dist->files[i].dst);

      file = dist->files + i;

      sprintf(filename, "/etc/init.d/%s", file->dst);
      strcpy(file->dst, filename);
    }

 /*
  * Write the prototype file for pkgmk...
  */

  if (Verbosity)
    puts("Creating prototype file...");

  sprintf(filename, "%s/%s.prototype", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create prototype file \"%s\" - %s\n",
            filename, strerror(errno));
    return (1);
  }

  fprintf(fp, "i copyright=%s/%s\n", current, dist->license);
  fprintf(fp, "i depend=%s/%s/%s.depend\n", current, directory, prodname);
  fprintf(fp, "i pkginfo=%s/%s/%s.pkginfo\n", current, directory, prodname);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      break;

  if (dist->num_installs || i)
    fprintf(fp, "i postinstall=%s/%s/%s.postinstall\n", current, directory, prodname);
  if (dist->num_removes || i)
    fprintf(fp, "i preremove=%s/%s/%s.preremove\n", current, directory, prodname);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    switch (tolower(file->type))
    {
      case 'c' :
          if (file->src[0] == '/')
            fprintf(fp, "e none %s=%s %04o %s %s\n", file->dst,
	            file->src, file->mode, file->user, file->group);
          else
            fprintf(fp, "e none %s=%s/%s %04o %s %s\n", file->dst,
	            current, file->src, file->mode, file->user, file->group);
          break;
      case 'd' :
	  fprintf(fp, "d none %s %04o %s %s\n", file->dst, file->mode,
		  file->user, file->group);
          break;
      case 'f' :
      case 'i' :
          if (file->src[0] == '/')
            fprintf(fp, "f none %s=%s %04o %s %s\n", file->dst,
	            file->src, file->mode, file->user, file->group);
          else
            fprintf(fp, "f none %s=%s/%s %04o %s %s\n", file->dst,
	            current, file->src, file->mode, file->user, file->group);
          break;
      case 'l' :
          fprintf(fp, "s none %s=%s\n", file->dst, file->src);
          break;
    }

  fclose(fp);

 /*
  * Build the distribution from the prototype file...
  */

  if (Verbosity)
    puts("Building PKG binary distribution...");

  sprintf(command, "pkgmk -o -f %s/%s.prototype -d %s/%s",
          directory, prodname, current, directory);

  if (system(command))
    return (1);

 /*
  * Tar and compress the distribution...
  */

  if (Verbosity)
    puts("Creating tar.gz file for distribution...");

  sprintf(filename, "%s/%s.tar.gz", directory, name);

  if ((tarfile = tar_open(filename, 1)) == NULL)
    return (1);

  sprintf(filename, "%s/%s", directory, prodname);

  if (tar_directory(tarfile, filename, prodname))
  {
    tar_close(tarfile);
    return (1);
  }

  tar_close(tarfile);

 /*
  * Make a package stream file...
  */

  if (Verbosity)
    puts("Copying into package stream file...");

  sprintf(command, "cd %s; pkgtrans -s . %s.pkg %s",
          directory, name, prodname);

  if (system(command))
    return (1);

 /*
  * Remove temporary files...
  */

  if (Verbosity)
    puts("Removing temporary distribution files...");

  sprintf(filename, "%s/%s.pkginfo", directory, prodname);
  unlink(filename);
  sprintf(filename, "%s/%s.depend", directory, prodname);
  unlink(filename);
  sprintf(filename, "%s/%s.prototype", directory, prodname);
  unlink(filename);
  sprintf(filename, "%s/%s.postinstall", directory, prodname);
  unlink(filename);
  sprintf(filename, "%s/%s.preremove", directory, prodname);
  unlink(filename);

  return (0);
}


/*
 * End of "$Id: pkg.c,v 1.11 2001/02/15 13:34:16 mike Exp $".
 */
