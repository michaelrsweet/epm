/*
 * "$Id: rpm.c,v 1.19 2000/08/22 12:56:17 mike Exp $"
 *
 *   Red Hat package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2000 by Easy Software Products.
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
  char		name[1024];		/* Full product name */
  char		specname[1024];		/* Spec filename */
  char		filename[1024],		/* Destination filename */
		linkname[1024];		/* Link filename */
  char		command[1024];		/* RPM command to run */
  const char	*rpmdir;		/* RPM directory */
  file_t	*file;			/* Current distribution file */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */


  if (Verbosity)
    puts("Creating RPM distribution...");

  if (platname[0])
    sprintf(name, "%s-%s-%s", prodname, dist->version, platname);
  else
    sprintf(name, "%s-%s", prodname, dist->version);

 /*
  * Write the spec file for RPM...
  */

  if (Verbosity)
    puts("Creating spec file...");

  sprintf(specname, "%s/%s.spec", directory, prodname);

  if ((fp = fopen(specname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create spec file \"%s\" - %s\n", specname,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "Summary: %s\n", dist->product);
  fprintf(fp, "Name: %s\n", prodname);
  fprintf(fp, "Version: %s\n", dist->version);
  fputs("Release: 1\n", fp);
  fprintf(fp, "Copyright: %s\n", dist->copyright);
  fprintf(fp, "Packager: %s\n", dist->vendor);
  fprintf(fp, "BuildRoot: %s/buildroot\n", directory);
  fputs("Group: Applications\n", fp);
  for (i = 0; i < dist->num_requires; i ++)
    if (dist->requires[i][0] != '/')
      fprintf(fp, "Requires: %s\n", dist->requires[i]);
  for (i = 0; i < dist->num_replaces; i ++)
    if (dist->replaces[i][0] != '/')
      fprintf(fp, "Conflicts: %s\n", dist->replaces[i]);
  for (i = 0; i < dist->num_incompats; i ++)
    if (dist->incompats[i][0] != '/')
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
      fputs("if test -x /sbin/chkconfig; then\n", fp);
      fprintf(fp, "	/sbin/chkconfig --add %s\n", file->dst);
      fprintf(fp, "	/sbin/chkconfig %s on\n", file->dst);
      fputs("fi\n", fp);
      fprintf(fp, "/etc/rc.d/init.d/%s start\n", file->dst);
    }
  fputs("%preun\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
    {
      fprintf(fp, "/etc/rc.d/init.d/%s stop\n", file->dst);
      fputs("if test -x /sbin/chkconfig; then\n", fp);
      fprintf(fp, "	/sbin/chkconfig --del %s\n", file->dst);
      fputs("fi\n", fp);
    }
  for (i = 0; i < dist->num_removes; i ++)
    fprintf(fp, "%s\n", dist->removes[i]);
  fputs("%files\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    switch (tolower(file->type))
    {
      case 'c' :
          fprintf(fp, "%%attr(%04o,%s,%s) %%config %s\n", file->mode,
	          file->user, file->group, file->dst);
          break;
      case 'd' :
          fprintf(fp, "%%attr(%04o,%s,%s) %%dir %s\n", file->mode, file->user,
	          file->group, file->dst);
          break;
      case 'f' :
      case 'l' :
          fprintf(fp, "%%attr(%04o,%s,%s) %s\n", file->mode, file->user,
	          file->group, file->dst);
          break;
      case 'i' :
          fprintf(fp, "%%attr(0555,root,root) /etc/rc.d/init.d/%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/rc.d/rc0.d/K00%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/rc.d/rc3.d/S99%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/rc.d/rc5.d/S99%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/init.d/%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/rc0.d/K00%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/rc3.d/S99%s\n", file->dst);
          fprintf(fp, "%%attr(0555,root,root) /etc/rc5.d/S99%s\n", file->dst);
          break;
    }

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
          sprintf(filename, "%s/buildroot%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          sprintf(filename, "%s/buildroot/etc/rc.d/init.d/%s", directory,
	          file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);

          sprintf(linkname, "/etc/rc.d/init.d/%s", file->dst);

          sprintf(filename, "%s/buildroot/etc/init.d/%s", directory, file->dst);
          make_link(filename, linkname);

          sprintf(filename, "%s/buildroot/etc/rc0.d/K00%s", directory, file->dst);
          make_link(filename, linkname);

          sprintf(filename, "%s/buildroot/etc/rc3.d/S99%s", directory, file->dst);
          make_link(filename, linkname);

          sprintf(filename, "%s/buildroot/etc/rc5.d/S99%s", directory, file->dst);
          make_link(filename, linkname);

          sprintf(filename, "%s/buildroot/etc/rc.d/rc0.d/K00%s", directory, file->dst);
          make_link(filename, linkname);

          sprintf(filename, "%s/buildroot/etc/rc.d/rc3.d/S99%s", directory, file->dst);
          make_link(filename, linkname);

          sprintf(filename, "%s/buildroot/etc/rc.d/rc5.d/S99%s", directory, file->dst);
          make_link(filename, linkname);
          break;
      case 'd' :
          sprintf(filename, "%s/buildroot%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          sprintf(filename, "%s/buildroot%s", directory, file->dst);

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

  if (strcmp(platform->machine, "intel") == 0)
    sprintf(command, "rpm -bb " EPM_RPMARCH " i386 %s %s",
            Verbosity == 0 ? "--quiet" : "", specname);
  else
    sprintf(command, "rpm -bb %s %s",
            Verbosity == 0 ? "--quiet" : "", specname);

  if (system(command))
    return (1);

 /*
  * Figure out where the RPMS are stored...
  */

  if ((rpmdir = getenv("RPMDIR")) == NULL)
  {
    if (!access("/usr/src/redhat/RPMS", 0))
      rpmdir = "/usr/src/redhat";
    else if (!access("/usr/src/RPM/RPMS", 0))
      rpmdir = "/usr/src/RPM";
    else
      rpmdir = "/usr/local/src/RPM";
  }

 /*
  * Move the RPM to the local directory and rename the RPM using the
  * product name specified by the user...
  */

  if (strcmp(platform->machine, "intel") == 0)
    sprintf(command, "cd %s; /bin/mv %s/RPMS/i386/%s-%s-1.i386.rpm %s.rpm",
            directory, rpmdir, prodname, dist->version, name);
  else
    sprintf(command, "cd %s; /bin/mv %s/RPMS/%s/%s-%s-1.%s.rpm %s.rpm",
            directory, rpmdir, platform->machine, prodname, dist->version,
	    platform->machine, name);

  system(command);

 /*
  * Remove temporary files...
  */

  if (Verbosity)
    puts("Removing temporary distribution files...");

  sprintf(command, "/bin/rm -rf %s/buildroot", directory);
  system(command);

  unlink(specname);

  return (0);
}


/*
 * End of "$Id: rpm.c,v 1.19 2000/08/22 12:56:17 mike Exp $".
 */
