/*
 * "$Id: deb.c,v 1.2 1999/11/05 14:44:40 mike Exp $"
 *
 *   Debian package gateway for the ESP Package Manager (EPM).
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
  int		i;			/* Looping var */
  FILE		*fp;			/* Control file */
  char		name[1024];		/* Full product name */
  char		filename[1024];		/* Destination filename */
  char		command[1024];		/* Command to run */
  file_t	*file;			/* Current distribution file */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */


  if (Verbosity)
    puts("Creating DEB distribution...");

  if (platname[0])
    sprintf(name, "%s-%s-%s", prodname, dist->version, platname);
  else
    sprintf(name, "%s-%s", prodname, dist->version);

 /*
  * Write the control file for DPKG...
  */

  if (Verbosity)
    puts("Creating control file...");

  sprintf(filename, "%s/%s", directory, name);
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

  for (i = 0; i < dist->num_requires; i ++)
    fprintf(fp, "Depends: %s\n", dist->requires[i]);

  for (i = 0; i < dist->num_incompats; i ++)
    fprintf(fp, "Conflicts: %s\n", dist->incompats[i]);

  fclose(fp);

 /*
  * Write the postinst file for DPKG...
  */

  if (Verbosity)
    puts("Creating postinst script...");

  sprintf(filename, "%s/%s/DEBIAN/postinst", directory, name);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fchmod(fileno(fp), 0755);

  fputs("#!/bin/sh\n", fp);
  fputs("# " EPM_VERSION "\n", fp);

  for (i = 0; i < dist->num_installs; i ++)
    fprintf(fp, "%s\n", dist->installs[i]);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
    {
      fprintf(fp, "update-rc.d %s defaults >/dev/null\n", file->dst);

      fprintf(fp, "/etc/init.d/%s start\n", file->dst);
    }

  fclose(fp);

 /*
  * Write the preun file for DPKG...
  */

  if (Verbosity)
    puts("Creating preun script...");

  sprintf(filename, "%s/%s/DEBIAN/preun", directory, name);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fchmod(fileno(fp), 0755);

  fputs("#!/bin/sh\n", fp);
  fputs("# " EPM_VERSION "\n", fp);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
    {
      fputs("if [ purge = \"$1\" ]; then\n", fp);
      fprintf(fp, "	update-rc.d %s remove >/dev/null\n", file->dst);
      fputs("fi\n", fp);

      fprintf(fp, "/etc/init.d/%s stop\n", file->dst);
    }

  for (i = 0; i < dist->num_removes; i ++)
    fprintf(fp, "%s\n", dist->removes[i]);

 /*
  * Write the conffiles file for DPKG...
  */

  if (Verbosity)
    puts("Creating conffiles...");

  sprintf(filename, "%s/%s/DEBIAN/conffiles", directory, name);

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
          sprintf(filename, "%s/%s%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          sprintf(filename, "%s/%s/etc/init.d/%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          sprintf(filename, "%s/%s%s", directory, name, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", file->src, filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          sprintf(filename, "%s/%s%s", directory, name, file->dst);

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
    puts("Building DEB binary distribution...");

  sprintf(command, "cd %s; dpkg --build %s", directory, name);

  if (system(command))
    return (1);

 /*
  * Remove temporary files...
  */

  if (Verbosity)
    puts("Removing temporary distribution files...");

  sprintf(command, "/bin/rm -rf %s/%s", directory, name);
  system(command);

  return (0);
}


/*
 * End of "$Id: deb.c,v 1.2 1999/11/05 14:44:40 mike Exp $".
 */
