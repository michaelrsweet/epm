/*
 * "$Id: rpm.c,v 1.3 1999/11/05 02:50:32 mike Exp $"
 *
 *   Red Hat package gateway for the ESP Package Manager (EPM).
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
 *   make_rpm() - Make a Red Hat software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_rpm()' - Make a Red Hat software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_rpm(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec file */
  char		specname[1024];		/* Spec filename */
  char		filename[1024];		/* Destination filename */
  char		command[1024];		/* RPM command to run */
  file_t	*file;			/* Current distribution file */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */


  if (Verbosity)
    puts("Creating RPM distribution...");

 /*
  * Write the spec file for RPM...
  */

  if (Verbosity)
    puts("Creating spec file...");

  sprintf(specname, "%s/%s.spec", directory, prodname);

  if ((fp = fopen(specname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create spec file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "Summary: %s\n", dist->product);
  fprintf(fp, "Name: %s\n", prodname);
  fprintf(fp, "Version: %s\n", dist->version);
  fputs("Release: 1\n", fp);
  fprintf(fp, "Copyright: %s\n", dist->copyright);
  fprintf(fp, "Packager: %s\n", dist->vendor);
  fprintf(fp, "BuildRoot: %s\n", directory);
  fputs("Group: Applications\n", fp);
  for (i = 0; i < dist->num_requires; i ++)
    fprintf(fp, "Requires: %s\n", dist->requires[i]);
  for (i = 0; i < dist->num_incompats; i ++)
    fprintf(fp, "Conflicts: %s\n", dist->incompats[i]);
  fputs("%description\n", fp);
  for (i = 0; i < dist->num_descriptions; i ++)
    fprintf(fp, "%s\n", dist->descriptions[i]);
  fputs("%post\n", fp);
  for (i = 0; i < dist->num_installs; i ++)
    fprintf(fp, "%s\n", dist->installs[i]);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
    {
      fprintf(fp, "/bin/chkconfig --add %s\n", file->dst);
      fprintf(fp, "/etc/rc.d/init.d/%s start\n", file->dst);
    }
  fputs("%preun\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
    {
      fprintf(fp, "/bin/chkconfig --del %s\n", file->dst);
      fprintf(fp, "/etc/rc.d/init.d/%s stop\n", file->dst);
    }
  for (i = 0; i < dist->num_removes; i ++)
    fprintf(fp, "%s\n", dist->removes[i]);
  fputs("%files\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    switch (tolower(file->type))
    {
      case 'c' :
          fprintf(fp, "%%config %s\n", file->dst);
          break;
      case 'd' :
          fprintf(fp, "%%dir %s\n", file->dst);
          break;
      case 'f' :
      case 'l' :
          fprintf(fp, "%s\n", file->dst);
          break;
      case 'i' :
          fprintf(fp, "/etc/rc.d/init.d/%s\n", file->dst);
          break;
    }

  fclose(fp);

 /*
  * Copy the files over...
  */

  if (Verbosity)
    puts("Copying distribution files...");

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
          sprintf(filename, "%s%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          sprintf(filename, "%s/etc/rc.d/init.d/%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          sprintf(filename, "%s%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", file->src, filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          sprintf(filename, "%s%s", directory, file->dst);

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
    puts("Building RPM binary distribution...");

  sprintf(command, "rpm -bb %s", specname);

  return (system(command));
}


/*
 * End of "$Id: rpm.c,v 1.3 1999/11/05 02:50:32 mike Exp $".
 */
