/*
 * "$Id: rpm.c,v 1.25 2001/03/06 17:13:39 mike Exp $"
 *
 *   Red Hat package gateway for the ESP Package Manager (EPM).
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
  char		command[1024];		/* RPM command to run */
  const char	*rpmdir;		/* RPM directory */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  depend_t	*d;			/* Current dependency */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  char		current[1024];		/* Current directory */


  if (Verbosity)
    puts("Creating RPM distribution...");

  getcwd(current, sizeof(current));

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
  fprintf(fp, "Packager: %s\n", dist->packager);
  fprintf(fp, "Vendor: %s\n", dist->vendor);
  fprintf(fp, "BuildRoot: %s/%s/buildroot\n", current, directory);
  fputs("Group: Applications\n", fp);

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES)
      fprintf(fp, "Requires: %s\n", d->product);
    else
      fprintf(fp, "Conflicts: %s\n", d->product);

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
    fputs("	if test -d $dir/rc3.d -o " SYMLINK " $dir/rc3.d; then\n", fp);
    fputs("		rcdir=\"$dir\"\n", fp);
    fputs("	fi\n", fp);
    fputs("done\n", fp);
    fputs("if test \"$rcdir\" = \"\" ; then\n", fp);
    fputs("	echo Unable to determine location of startup scripts!\n", fp);
    fputs("else\n", fp);
    fputs("	for file in", fp);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        fprintf(fp, " %s", file->dst);

    fputs("; do\n", fp);
    fputs("		if test -d $rcdir/init.d; then\n", fp);
    fputs("			/bin/rm -f $rcdir/init.d/$file\n", fp);
    fputs("			/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/init.d/$file\n", fp);
    fputs("		else\n", fp);
    fputs("			if test -d /etc/init.d; then\n", fp);
    fputs("				/bin/rm -f /etc/init.d/$file\n", fp);
    fputs("				/bin/ln -s " EPM_SOFTWARE "/init.d/$file /etc/init.d/$file\n", fp);
    fputs("			fi\n", fp);
    fputs("		fi\n", fp);
    fputs("		/bin/rm -f $rcdir/rc0.d/K00$file\n", fp);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc0.d/K00$file\n", fp);
    fputs("		/bin/rm -f $rcdir/rc2.d/S99$file\n", fp);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc2.d/S99$file\n", fp);
    fputs("		/bin/rm -f $rcdir/rc3.d/S99$file\n", fp);
    fputs("		/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc3.d/S99$file\n", fp);
    fputs("		if test -d $rcdir/rc5.d; then\n", fp);
    fputs("			/bin/rm -f $rcdir/rc5.d/S99$file\n", fp);
    fputs("			/bin/ln -s " EPM_SOFTWARE "/init.d/$file $rcdir/rc5.d/S99$file\n", fp);
    fputs("		fi\n", fp);
    fputs("		" EPM_SOFTWARE "/init.d/$file start\n", fp);
    fputs("	done\n", fp);
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
    fputs("	if test -d $dir/rc3.d -o " SYMLINK " $dir/rc3.d; then\n", fp);
    fputs("		rcdir=\"$dir\"\n", fp);
    fputs("	fi\n", fp);
    fputs("done\n", fp);
    fputs("if test \"$rcdir\" = \"\" ; then\n", fp);
    fputs("	echo Unable to determine location of startup scripts!\n", fp);
    fputs("else\n", fp);
    fputs("	for file in", fp);
    for (; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        fprintf(fp, " %s", file->dst);

    fputs("; do\n", fp);
    fputs("		/bin/rm -f $rcdir/init.d/$file\n", fp);
    fputs("		/bin/rm -f $rcdir/rc0.d/K00$file\n", fp);
    fputs("		/bin/rm -f $rcdir/rc2.d/S99$file\n", fp);
    fputs("		/bin/rm -f $rcdir/rc3.d/S99$file\n", fp);
    fputs("		if test -d $rcdir/rc5.d; then\n", fp);
    fputs("			/bin/rm -f $rcdir/rc5.d/S99$file\n", fp);
    fputs("		fi\n", fp);
    fputs("		" EPM_SOFTWARE "/init.d/$file stop\n", fp);
    fputs("	done\n", fp);
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
          fprintf(fp, "%%attr(%04o,%s,%s) %%config(noreplace) %s\n", file->mode,
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
          fprintf(fp, "%%attr(0555,root,root) /etc/software/init.d/%s\n", file->dst);
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
          sprintf(filename, "%s/buildroot/etc/software/init.d/%s", directory,
	          file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
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
    sprintf(command, "rpm -bb " EPM_RPMARCH "i386 %s %s",
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
    if (!access("/usr/src/redhat/RPMS", 0))		/* Red Hat */
      rpmdir = "/usr/src/redhat";
    else if (!access("/usr/src/RPM/RPMS", 0))		/* Mandrake */
      rpmdir = "/usr/src/RPM";
    else if (!access("/usr/src/packages/RPMS", 0))	/* SuSE */
      rpmdir = "/usr/src/packages";
    else
      rpmdir = "/usr/local/src/RPM";			/* Others? */
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
 * End of "$Id: rpm.c,v 1.25 2001/03/06 17:13:39 mike Exp $".
 */
