/*
 * "$Id: rpm.c,v 1.54 2005/01/11 21:20:17 mike Exp $"
 *
 *   Red Hat package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2004 by Easy Software Products.
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
 *   make_rpm()   - Make a Red Hat software distribution package.
 *   move_rpms()  - Move RPM packages to the build directory...
 *   write_spec() - Write the subpackage-specific parts of the RPM spec file.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	write_spec(const char *prodname, dist_t *dist, FILE *fp,
		           const char *subpackage);
static int	move_rpms(const char *prodname, const char *directory,
		          const char *platname, dist_t *dist,
			  struct utsname *platform,
			  const char *rpmdir, const char *subpackage);


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
  file_t	*file;			/* Current distribution file */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  char		absdir[1024];		/* Absolute directory */
  char		rpmdir[1024];		/* RPMDIR env var */


  if (Verbosity)
    puts("Creating RPM distribution...");

  if (directory[0] != '/')
  {
    char	current[1024];		/* Current directory */


    getcwd(current, sizeof(current));

    snprintf(absdir, sizeof(absdir), "%s/%s", current, directory);
  }
  else
    strlcpy(absdir, directory, sizeof(absdir));

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

  fprintf(fp, "Name: %s\n", prodname);
  fprintf(fp, "Version: %s\n", dist->version);
  fprintf(fp, "Release: %d\n", dist->relnumber);
  fprintf(fp, "Copyright: %s\n", dist->copyright);
  fprintf(fp, "Packager: %s\n", dist->packager);
  fprintf(fp, "Vendor: %s\n", dist->vendor);
  fprintf(fp, "BuildRoot: %s/buildroot\n", absdir);

 /*
  * Tell RPM to put the distributions in the output directory...
  */

#ifdef EPM_RPMTOPDIR
  fprintf(fp, "%%define _topdir %s\n", absdir);
  strcpy(rpmdir, absdir);
#else
  if (getenv("RPMDIR"))
    strlcpy(rpmdir, getenv("RPMDIR"), sizeof(rpmdir));
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
  * Now list all of the subpackages...
  */

  write_spec(prodname, dist, fp, NULL);
  for (i = 0; i < dist->num_subpackages; i ++)
    write_spec(prodname, dist, fp, dist->subpackages[i]);

 /*
  * Close the spec file...
  */

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
    if (run_command(NULL, EPM_RPMBUILD " -bb " EPM_RPMARCH "i386 %s",
                    specname))
      return (1);
  }
  else if (run_command(NULL, EPM_RPMBUILD " -bb " EPM_RPMARCH "%s %s",
                       platform->machine, specname))
    return (1);

 /*
  * Move the RPMs to the local directory and rename the RPMs using the
  * product name specified by the user...
  */

  move_rpms(prodname, directory, platname, dist, platform, rpmdir, NULL);

  for (i = 0; i < dist->num_subpackages; i ++)
    move_rpms(prodname, directory, platname, dist, platform, rpmdir,
              dist->subpackages[i]);

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
 * 'move_rpms()' - Move RPM packages to the build directory...
 */

int					/* O - 0 = success, 1 = fail */
move_rpms(const char     *prodname,	/* I - Product short name */
          const char     *directory,	/* I - Directory for distribution files */
          const char     *platname,	/* I - Platform name */
          dist_t         *dist,		/* I - Distribution information */
	  struct utsname *platform,	/* I - Platform information */
	  const char     *rpmdir,	/* I - RPM directory */
          const char     *subpackage)	/* I - Subpackage name */
{
  char		rpmname[1024];		/* RPM name */
  char		prodfull[1024];		/* Full product name */
  struct stat	rpminfo;		/* RPM file info */


 /*
  * Move the RPMs to the local directory and rename the RPMs using the
  * product name specified by the user...
  */

  if (subpackage)
    snprintf(prodfull, sizeof(prodfull), "%s-%s", prodname, subpackage);
  else
    strlcpy(prodfull, prodname, sizeof(prodfull));

  if (dist->relnumber)
    snprintf(rpmname, sizeof(rpmname), "%s/%s-%s-%d", directory, prodfull,
             dist->version, dist->relnumber);
  else
    snprintf(rpmname, sizeof(rpmname), "%s/%s-%s", directory, prodfull,
             dist->version);

  if (platname[0])
  {
    strlcat(rpmname, "-", sizeof(rpmname));
    strlcat(rpmname, platname, sizeof(rpmname));
  }

  strlcat(rpmname, ".rpm", sizeof(rpmname));

  if (strcmp(platform->machine, "intel") == 0)
    run_command(NULL, "/bin/mv %s/RPMS/i386/%s-%s-%d.i386.rpm %s",
		rpmdir, prodfull, dist->version, dist->relnumber,
		rpmname);
  else
    run_command(NULL, "/bin/mv %s/RPMS/%s/%s-%s-%d.%s.rpm %s",
		rpmdir, platform->machine, prodfull, dist->version,
		dist->relnumber, platform->machine, rpmname);

  if (Verbosity)
  {
    stat(rpmname, &rpminfo);

    printf("    %7.0fk  %s\n", rpminfo.st_size / 1024.0, rpmname);
  }

  return (0);
}


/*
 * 'write_spec()' - Write the subpackage-specific parts of the RPM spec file.
 */

static int				/* O - 0 on success, -1 on error */
write_spec(const char *prodname,	/* I - Product name */
	   dist_t     *dist,		/* I - Distribution */
           FILE       *fp,		/* I - Spec file */
           const char *subpackage)	/* I - Subpackage name */
{
  int		i;			/* Looping var */
  char		name[1024];		/* Full product name */
  const char	*product;		/* Product to depend on */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  depend_t	*d;			/* Current dependency */
  const char	*runlevels;		/* Run levels */
  int		number;			/* Start/stop number */
  const char	*dname;			/* Dependency name */


 /*
  * Get the name we'll use for the subpackage...
  */

  if (subpackage)
    snprintf(name, sizeof(name), " %s", subpackage);
  else
    name[0] = '\0';

 /*
  * Common stuff...
  */

  if (subpackage)
    fprintf(fp, "%%package%s\n", name);

  fprintf(fp, "Summary: %s\n", dist->product);
  fputs("Group: Applications\n", fp);

 /*
  * List all of the dependencies...
  */

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
  {
    if (d->subpackage != subpackage)
      continue;

    if (!strcmp(d->product, "_self"))
      product = prodname;
    else
      product = d->product;

    if ((dname = strrchr(product, '/')) != NULL)
      dname ++;
    else
      dname = product;

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
        fprintf(fp, " >= %s, %s <= %s\n", d->version[0], product,
	        d->version[1]);
    }
    else if (d->vernumber[0] != d->vernumber[1])
      fprintf(fp, " >= %s\n", d->version[0]);
    else
      fprintf(fp, " = %s\n", d->version[0]);
  }

 /*
  * Pre/post install commands...
  */

  fprintf(fp, "%%pre%s\n", name);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL && c->subpackage == subpackage)
      fprintf(fp, "%s\n", c->command);

  fprintf(fp, "%%post%s\n", name);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL && c->subpackage == subpackage)
      fprintf(fp, "%s\n", c->command);

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("if test \"x$1\" = x1; then\n", fp);
    fputs("	echo Setting up init scripts...\n", fp);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("	rcdir=\"\"\n", fp);
    fputs("	for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", fp);
    fputs("		if test -d $dir/rc3.d -o -h $dir/rc3.d; then\n", fp);
    fputs("			rcdir=\"$dir\"\n", fp);
    fputs("		fi\n", fp);
    fputs("	done\n", fp);
    fputs("	if test \"$rcdir\" = \"\" ; then\n", fp);
    fputs("		echo Unable to determine location of startup scripts!\n", fp);
    fputs("	else\n", fp);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
	fputs("		if test -d $rcdir/init.d; then\n", fp);
	qprintf(fp, "			/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	qprintf(fp, "			/bin/ln -s %s/init.d/%s "
                    "$rcdir/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("		else\n", fp);
	fputs("			if test -d /etc/init.d; then\n", fp);
	qprintf(fp, "				/bin/rm -f /etc/init.d/%s\n", file->dst);
	qprintf(fp, "				/bin/ln -s %s/init.d/%s "
                    "/etc/init.d/%s\n", SoftwareDir, file->dst, file->dst);
	fputs("			fi\n", fp);
	fputs("		fi\n", fp);

	for (runlevels = get_runlevels(dist->files + i, "0123456");
             isdigit(*runlevels & 255);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(fp, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          (*runlevels == '0' || *runlevels == '1' ||
		   *runlevels == '6') ? 'K' : 'S', number, file->dst);
	  qprintf(fp, "		/bin/ln -s %s/init.d/%s "
                      "$rcdir/rc%c.d/%c%02d%s\n", SoftwareDir, file->dst,
		  *runlevels,
		  (*runlevels == '0' || *runlevels == '1' ||
		   *runlevels == '6') ? 'K' : 'S', number, file->dst);
        }

        qprintf(fp, "		%s/init.d/%s start\n", SoftwareDir, file->dst);
      }

    fputs("	fi\n", fp);
    fputs("fi\n", fp);
  }

  fprintf(fp, "%%preun%s\n", name);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      break;

  if (i)
  {
    fputs("if test \"x$1\" = x0; then\n", fp);
    fputs("	echo Cleaning up init scripts...\n", fp);

   /*
    * Find where the frigging init scripts go...
    */

    fputs("	rcdir=\"\"\n", fp);
    fputs("	for dir in /sbin/rc.d /sbin /etc/rc.d /etc ; do\n", fp);
    fputs("		if test -d $dir/rc3.d -o -h $dir/rc3.d; then\n", fp);
    fputs("			rcdir=\"$dir\"\n", fp);
    fputs("		fi\n", fp);
    fputs("	done\n", fp);
    fputs("	if test \"$rcdir\" = \"\" ; then\n", fp);
    fputs("		echo Unable to determine location of startup scripts!\n", fp);
    fputs("	else\n", fp);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i' && file->subpackage == subpackage)
      {
        qprintf(fp, "		%s/init.d/%s stop\n", SoftwareDir, file->dst);

	fputs("		if test -d $rcdir/init.d; then\n", fp);
	qprintf(fp, "			/bin/rm -f $rcdir/init.d/%s\n", file->dst);
	fputs("		else\n", fp);
	fputs("			if test -d /etc/init.d; then\n", fp);
	qprintf(fp, "				/bin/rm -f /etc/init.d/%s\n", file->dst);
	fputs("			fi\n", fp);
	fputs("		fi\n", fp);

	for (runlevels = get_runlevels(dist->files + i, "0123456");
             isdigit(*runlevels & 255);
	     runlevels ++)
	{
	  if (*runlevels == '0')
            number = get_stop(file, 0);
	  else
	    number = get_start(file, 99);

	  qprintf(fp, "		/bin/rm -f $rcdir/rc%c.d/%c%02d%s\n", *runlevels,
	          (*runlevels == '0' || *runlevels == '1' ||
		   *runlevels == '6') ? 'K' : 'S', number, file->dst);
        }
      }

    fputs("	fi\n", fp);
    fputs("fi\n", fp);
  }

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE && c->subpackage == subpackage)
      fprintf(fp, "%s\n", c->command);

  fprintf(fp, "%%postun%s\n", name);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE && c->subpackage == subpackage)
      fprintf(fp, "%s\n", c->command);

 /*
  * Description...
  */

  for (i = 0; i < dist->num_descriptions; i ++)
    if (dist->descriptions[i].subpackage == subpackage)
      break;

  if (i < dist->num_descriptions)
  {
    fprintf(fp, "%%description %s\n", name);

    for (; i < dist->num_descriptions; i ++)
      if (dist->descriptions[i].subpackage == subpackage)
	fprintf(fp, "%s\n", dist->descriptions[i].description);
  }

 /*
  * Files...
  */

  fprintf(fp, "%%files%s\n", name);
  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (file->subpackage == subpackage)
      switch (tolower(file->type))
      {
	case 'c' :
            fprintf(fp, "%%attr(%04o,%s,%s) %%config(noreplace) \"%s\"\n",
	            file->mode, file->user, file->group, file->dst);
            break;
	case 'd' :
            fprintf(fp, "%%attr(%04o,%s,%s) %%dir \"%s\"\n", file->mode,
	            file->user, file->group, file->dst);
            break;
	case 'f' :
	case 'l' :
            fprintf(fp, "%%attr(%04o,%s,%s) \"%s\"\n", file->mode, file->user,
	            file->group, file->dst);
            break;
	case 'i' :
            fprintf(fp, "%%attr(0555,root,root) \"%s/init.d/%s\"\n", SoftwareDir,
	            file->dst);
            break;
      }

  return (0);
}


/*
 * End of "$Id: rpm.c,v 1.54 2005/01/11 21:20:17 mike Exp $".
 */
