/*
 * "$Id: rpm.c,v 1.46 2003/05/30 04:38:44 mike Exp $"
 *
 *   Red Hat package gateway for the ESP Package Manager (EPM).
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
  char		filename[1024];		/* Destination filename */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  depend_t	*d;			/* Current dependency */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  char		current[1024];		/* Current directory */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */
  char		rpmdir[1024];		/* RPMDIR env var */
  const char	*dname;			/* Dependency name */


  if (Verbosity)
    puts("Creating RPM distribution...");

  getcwd(current, sizeof(current));

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
  * Write the spec file for RPM...
  */

  if (Verbosity)
    puts("Creating spec file...");

  snprintf(specname, sizeof(specname), "%s/%s.spec", directory, prodname);

  if ((fp = fopen(specname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create spec file \"%s\" - %s\n", specname,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "Summary: %s\n", dist->product);
  fprintf(fp, "Name: %s\n", prodname);
  fprintf(fp, "Version: %s\n", dist->version);
  fprintf(fp, "Release: %d\n", dist->relnumber);
  fprintf(fp, "Copyright: %s\n", dist->copyright);
  fprintf(fp, "Packager: %s\n", dist->packager);
  fprintf(fp, "Vendor: %s\n", dist->vendor);
  if (directory[0] == '/')
    fprintf(fp, "BuildRoot: %s/buildroot\n", directory);
  else
    fprintf(fp, "BuildRoot: %s/%s/buildroot\n", current, directory);
  fputs("Group: Applications\n", fp);

 /*
  * Tell RPM to put the distributions in the output directory...
  */

#ifdef EPM_RPMTOPDIR
  if (directory[0] == '/')
  {
    fprintf(fp, "%%define _topdir %s\n", directory);
    strncpy(rpmdir, directory, sizeof(rpmdir) - 1);
    rpmdir[sizeof(rpmdir) - 1] = '\0';
  }
  else
  {
    fprintf(fp, "%%define _topdir %s/%s\n", current, directory);
    snprintf(rpmdir, sizeof(rpmdir), "%s/%s", current, directory);
  }
#else
  if (getenv("RPMDIR"))
  {
    strncpy(rpmdir, getenv("RPMDIR"), sizeof(rpmdir) - 1);
    rpmdir[sizeof(rpmdir) - 1] = '\0';
  }
  else if (!access("/usr/src/redhat", 0))
    strcpy(rpmdir, "/usr/src/redhat");
  else if (!access("/usr/src/Mandrake", 0))
    strcpy(rpmdir, "/usr/src/Mandrake");
  else
    strcpy(rpmdir, "/usr/src/RPM");
#endif /* EPM_RPMTOPDIR */

  snprintf(filename, sizeof(filename), "%s/RPMS", directory);

  make_directory(filename, 0777, getuid(), getgid());

  if (strcmp(platform->machine, "intel") == 0)
    snprintf(filename, sizeof(filename), "%s/RPMS/i386", directory);
  else
    snprintf(filename, sizeof(filename), "%s/RPMS/%s", directory,
             platform->machine);

  make_directory(filename, 0777, getuid(), getgid());

 /*
  * Now list all of the dependencies...
  */

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
  {
    if ((dname = strrchr(d->product, '/')) != NULL)
      dname ++;
    else
      dname = d->product;

    if (d->type == DEPEND_REQUIRES)
      fprintf(fp, "Requires: %s", dname);
    else if (d->type == DEPEND_PROVIDES)
      fprintf(fp, "Provides: %s", dname);
    else
      fprintf(fp, "Conflicts: %s", dname);

    if (d->vernumber[0] == 0)
    {
      if (d->vernumber[1] < INT_MAX)
        fprintf(fp, " <= %s\n", d->version[1]);
      else
        putc('\n', fp);
    }
    else if (d->vernumber[0] && d->vernumber[1] < INT_MAX)
    {
      if (d->vernumber[0] < INT_MAX && d->vernumber[1] < INT_MAX)
        fprintf(fp, " >= %s, %s <= %s\n", d->version[0], d->product,
	        d->version[1]);
    }
    else
      fprintf(fp, " = %s\n", d->version[0]);
  }

  fputs("%description\n", fp);
  for (i = 0; i < dist->num_descriptions; i ++)
    fprintf(fp, "%s\n", dist->descriptions[i]);

  fputs("%pre\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      fprintf(fp, "%s\n", c->command);

  fputs("%post\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      fprintf(fp, "%s\n", c->command);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      break;

  if (i)
  {
    fputs("echo Setting up init scripts...\n", fp);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", fp);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", fp);
    fputs("	if test -d $dir/rc3.d -o -h $dir/rc3.d; then\n", fp);
    fputs("		rcdir=\"$dir\"\n", fp);
    fputs("	fi\n", fp);
    fputs("done\n", fp);
    fputs("if test \"$rcdir\" = \"\" ; then\n", fp);
    fputs("	echo Unable to determine location of startup scripts!\n", fp);
    fputs("else\n", fp);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
	fputs("	if test -d $rcdir/init.d; then\n", fp);
	qprintf(fp, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	qprintf(fp, "		/bin/ln -s %s/init.d/%s "
                    "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("	else\n", fp);
	fputs("		if test -d /etc/init.d; then\n", fp);
	qprintf(fp, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	qprintf(fp, "			/bin/ln -s %s/init.d/%s "
                    "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("		fi\n", fp);
	fputs("	fi\n", fp);

	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(fp, "	/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          *runlevels == '0' ? 'K' : 'S', number, file->dst);
	  qprintf(fp, "	/bin/ln -s %s/init.d/%s "
                      "$rcdir/rc%c.d/%c%02d%s\n", SoftwareDir, file->dst,
		  *runlevels, *runlevels == '0' ? 'K' : 'S', number, file->dst);
        }

        qprintf(fp, "	%s/init.d/%s start\n", SoftwareDir, file->dst);
      }

    fputs("fi\n", fp);
  }

  fputs("%preun\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i')
      break;

  if (i)
  {
    fputs("echo Cleaning up init scripts...\n", fp);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("rcdir=\"\"\n", fp);
    fputs("for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", fp);
    fputs("	if test -d $dir/rc3.d -o -h $dir/rc3.d; then\n", fp);
    fputs("		rcdir=\"$dir\"\n", fp);
    fputs("	fi\n", fp);
    fputs("done\n", fp);
    fputs("if test \"$rcdir\" = \"\" ; then\n", fp);
    fputs("	echo Unable to determine location of startup scripts!\n", fp);
    fputs("else\n", fp);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
      {
        qprintf(fp, "	%s/init.d/%s stop\n", SoftwareDir, file->dst);

	fputs("	if test -d $rcdir/init.d; then\n", fp);
	qprintf(fp, "		/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	fputs("	else\n", fp);
	fputs("		if test -d /etc/init.d; then\n", fp);
	qprintf(fp, "			/bin/rm -f /etc/init.d/%s\n", file->dst);
	fputs("		fi\n", fp);
	fputs("	fi\n", fp);

	for (runlevels = get_runlevels(dist->files + i, "0235");
             isdigit(*runlevels);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(fp, "	/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          *runlevels == '0' ? 'K' : 'S', number, file->dst);
        }
      }

    fputs("fi\n", fp);
  }

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE)
      fprintf(fp, "%s\n", c->command);

  fputs("%postun\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      fprintf(fp, "%s\n", c->command);

  fputs("%files\n", fp);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    switch (tolower(file->type))
    {
      case 'c' :
          qprintf(fp, "%%attr(%04o,%s,%s) %%config(noreplace) %s\n", file->mode,
	          file->user, file->group, file->dst);
          break;
      case 'd' :
          qprintf(fp, "%%attr(%04o,%s,%s) %%dir %s\n", file->mode, file->user,
	          file->group, file->dst);
          break;
      case 'f' :
      case 'l' :
          qprintf(fp, "%%attr(%04o,%s,%s) %s\n", file->mode, file->user,
	          file->group, file->dst);
          break;
      case 'i' :
          qprintf(fp, "%%attr(0555,root,root) %s/init.d/%s\n", SoftwareDir,
	          file->dst);
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
          snprintf(filename, sizeof(filename), "%s/buildroot%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename), "%s/buildroot%s/init.d/%s",
	           directory, SoftwareDir, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          snprintf(filename, sizeof(filename), "%s/buildroot%s", directory, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          snprintf(filename, sizeof(filename), "%s/buildroot%s", directory, file->dst);

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
  {
    if (run_command(NULL, EPM_RPMBUILD " %s -bb " EPM_RPMARCH "i386 %s",
                    Verbosity == 0 ? "--quiet" : "", specname))
      return (1);
  }
  else if (run_command(NULL, EPM_RPMBUILD " %s -bb " EPM_RPMARCH "%s %s",
                       Verbosity == 0 ? "--quiet" : "", platform->machine,
		       specname))
    return (1);

 /*
  * Move the RPM to the local directory and rename the RPM using the
  * product name specified by the user...
  */

  if (strcmp(platform->machine, "intel") == 0)
    run_command(NULL, "/bin/mv %s/RPMS/i386/%s-%s-%d.i386.rpm %s/%s.rpm",
        	rpmdir, prodname, dist->version, dist->relnumber,
		directory, name);
  else
    run_command(NULL, "/bin/mv %s/RPMS/%s/%s-%s-%d.%s.rpm %s/%s.rpm",
        	rpmdir, platform->machine, prodname, dist->version,
		dist->relnumber, platform->machine, directory, name);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    run_command(NULL, "/bin/rm -rf %s/RPMS", directory);
    run_command(NULL, "/bin/rm -rf %s/buildroot", directory);

    unlink(specname);
  }

  return (0);
}


/*
 * End of "$Id: rpm.c,v 1.46 2003/05/30 04:38:44 mike Exp $".
 */
