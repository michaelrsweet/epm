/*
 * "$Id: deb.c,v 1.12 2001/07/05 15:34:48 mike Exp $"
 *
 *   Debian package gateway for the ESP Package Manager (EPM).
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
 *   make_deb() - Make a Debian software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_deb()' - Make a Debian software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_deb(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int			i;		/* Looping var */
  FILE			*fp;		/* Control file */
  char			name[1024];	/* Full product name */
  char			filename[1024];	/* Destination filename */
  command_t		*c;		/* Current command */
  depend_t		*d;		/* Current dependency */
  file_t		*file;		/* Current distribution file */
  struct passwd		*pwd;		/* Pointer to user record */
  struct group		*grp;		/* Pointer to group record */
  static const char	*depends[] =	/* Dependency names */
			{
			  "Depends",
			  "Conflicts",
			  "Replaces",
			  "Provides"
			};


  if (Verbosity)
    puts("Creating Debian distribution...");

  if (dist->relnumber)
  {
    if (platname[0])
      snprintf(name, sizeof(name), "%s-%s-%d-%s", prodname, dist->version, dist->relnumber,
              platname);
    else
      snprintf(name, sizeof(name), "%s-%s-%d", prodname, dist->version, dist->relnumber);
  }
  else if (platname[0])
    snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version, platname);
  else
    snprintf(name, sizeof(name), "%s-%s", prodname, dist->version);

 /*
  * Write the control file for DPKG...
  */

  if (Verbosity)
    puts("Creating control file...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, name);
  mkdir(filename, 0777);
  strcat(filename, "/DEBIAN");
  mkdir(filename, 0777);

  strcat(filename, "/control");

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create control file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "Package: %s\n", prodname);
  fprintf(fp, "Version: %s\n", dist->version);
  fprintf(fp, "Maintainer: %s\n", dist->vendor);

 /*
  * The Architecture attribute needs to match the uname info
  * (which we change in get_platform to a common name)
  */

  if (strcmp(platform->machine, "intel") == 0)
    fputs("Architecture: i386\n", fp);
  else
    fprintf(fp, "Architecture: %s\n", platform->machine);

  fprintf(fp, "Description: %s\n", dist->product);
  fprintf(fp, " Copyright: %s\n", dist->copyright);
  for (i = 0; i < dist->num_descriptions; i ++)
    fprintf(fp, " %s\n", dist->descriptions[i]);

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
  {
    fprintf(fp, "%s: %s", depends[(int)d->type], d->product);

    if (d->vernumber[0] == 0)
    {
      if (d->vernumber[1] < INT_MAX)
        fprintf(fp, " (<= %s)\n", d->version[1]);
      else
        putc('\n', fp);
    }
    else
      fprintf(fp, " (>= %s, <= %s)\n", d->version[0], d->version[1]);
  }

  fclose(fp);

 /*
  * Write the preinst file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      break;

  if (i)
  {
    if (Verbosity)
      puts("Creating preinst script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/preinst", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }

 /*
  * Write the postinst file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;

  if (i)
  {
    if (Verbosity)
      puts("Creating postinst script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/postinst", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
        fprintf(fp, "update-rc.d %s defaults >/dev/null\n", file->dst);

        fprintf(fp, "/etc/init.d/%s start\n", file->dst);
      }

    fclose(fp);
  }

 /*
  * Write the prerm file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;

  if (i)
  {
    if (Verbosity)
      puts("Creating prerm script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/prerm", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
        fputs("if [ purge = \"$1\" ]; then\n", fp);
        fprintf(fp, "	update-rc.d %s remove >/dev/null\n", file->dst);
        fputs("fi\n", fp);

        fprintf(fp, "/etc/init.d/%s stop\n", file->dst);
      }

    fclose(fp);
  }

 /*
  * Write the postrm file for DPKG...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      break;

  if (i)
  {
    if (Verbosity)
      puts("Creating postrm script...");

    snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/postrm", directory, name);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_REMOVE)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }

 /*
  * Write the conffiles file for DPKG...
  */

  if (Verbosity)
    puts("Creating conffiles...");

  snprintf(filename, sizeof(filename), "%s/%s/DEBIAN/conffiles", directory, name);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
      fprintf(fp, "%s\n", file->dst);
    else if (tolower(file->type) == 'i')
      fprintf(fp, "/etc/init.d/%s\n", file->dst);

  fclose(fp);

 /*
  * Copy the files over...
  */

  if (Verbosity)
    puts("Copying temporary distribution files...");

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
   /*
    * Find the username and groupname IDs...
    */

    pwd = getpwnam(file->user);
    grp = getgrnam(file->group);

    endpwent();
    endgrent();

   /*
    * Copy the file or make the directory or make the symlink as needed...
    */

    switch (tolower(file->type))
    {
      case 'c' :
      case 'f' :
          snprintf(filename, sizeof(filename), "%s/%s%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename), "%s/%s/etc/init.d/%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          snprintf(filename, sizeof(filename), "%s/%s%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          snprintf(filename, sizeof(filename), "%s/%s%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

          make_link(filename, file->src);
          break;
    }
  }

 /*
  * Build the distribution from the spec file...
  */

  if (Verbosity)
    puts("Building Debian binary distribution...");

  if (run_command(directory, "dpkg --build %s", name))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    run_command(NULL, "/bin/rm -rf %s/%s", directory, name);
  }

  return (0);
}


/*
 * End of "$Id: deb.c,v 1.12 2001/07/05 15:34:48 mike Exp $".
 */
