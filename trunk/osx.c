/*
 * "$Id: osx.c,v 1.3 2002/10/17 17:21:22 swdev Exp $"
 *
 *   MacOS X package gateway for the ESP Package Manager (EPM).
 *
 *   Copyright 2002 by Easy Software Products.
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
 *   make_osx() - Make a MacOS X software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_osx()' - Make a Red Hat software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_osx(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec file */
  char		name[1024];		/* Full product name */
  char		filename[1024];		/* Destination filename */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  char		current[1024];		/* Current directory */


  if (Verbosity)
    puts("Creating MacOS X distribution...");

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

#if 0
  fputs("%pre\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      qprintf(fp, "%s\n", c->command);

  fputs("%post\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      qprintf(fp, "%s\n", c->command);

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
      qprintf(fp, "%s\n", c->command);

  fputs("%postun\n", fp);
  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      qprintf(fp, "%s\n", c->command);

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
#endif /* 0 */

 /*
  * Copy the resources for the license, readme, and welcome (description)
  * stuff...
  */

  if (Verbosity)
    puts("Copying temporary resource files...");

  snprintf(filename, sizeof(filename), "%s/Resources", directory);
  make_directory(filename, 0777, 0, 0);

  snprintf(filename, sizeof(filename), "%s/Resources/License.txt", directory);
  copy_file(filename, dist->license, 0644, 0, 0);

  snprintf(filename, sizeof(filename), "%s/Resources/ReadMe.txt", directory);
  copy_file(filename, dist->readme, 0644, 0, 0);

  snprintf(filename, sizeof(filename), "%s/Resources/Welcome.txt", directory);
  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create welcome file \"%s\" - %s\n",
            filename, strerror(errno));
    return (1);
  }

  fprintf(fp, "%s version %s\n", dist->product, dist->version);
  fprintf(fp, "Copyright %s\n", dist->copyright);

  for (i = 0; i < dist->num_descriptions; i ++)
    fprintf(fp, "%s\n", dist->descriptions[i]);

  fclose(fp);

 /*
  * Do pre/post install commands...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      break;

  if (i)
  {
    snprintf(filename, sizeof(filename), "%s/Resources/preflight", directory);
    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create preinstall script \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fputs("#!/bin/sh\n", fp);

    for (; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);
    chmod(filename, 0755);
  }

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      break;

  if (!i)
  {
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;
  }

  if (i)
  {
    snprintf(filename, sizeof(filename), "%s/Resources/postflight", directory);
    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create postinstall script \"%s\" - %s\n",
              filename, strerror(errno));
      return (1);
    }

    fputs("#!/bin/sh\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
	fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        qprintf(fp, "/System/Library/StartupItems/%s/%s start\n",
	        file->dst, file->dst);

    fclose(fp);
    chmod(filename, 0755);
  }

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
          if (strncmp(file->dst, "/etc/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s/Package/private%s",
	             directory, file->dst);
          else
            snprintf(filename, sizeof(filename), "%s/Package%s",
	             directory, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename),
	           "%s/Package/System/Library/StartupItems/%s/%s",
	           directory, file->dst, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);

          snprintf(filename, sizeof(filename),
	           "%s/Package/System/Library/StartupItems/%s/StartupParameters.plist",
	           directory, file->dst);
	  if ((fp = fopen(filename, "w")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to create init data file \"%s\" - %s\n",
        	    filename, strerror(errno));
	    return (1);
	  }

          fputs("{\n", fp);
          fprintf(fp, "  Description = \"%s\";\n", dist->product);
	  fprintf(fp, "  Provides = \"%s\";\n", file->dst);
	  fputs("}\n", fp);

	  fclose(fp);

          snprintf(filename, sizeof(filename),
	           "%s/Package/System/Library/StartupItems/%s/Resources/English.lproj",
	           directory, file->dst);
          make_directory(filename, 0777, 0, 0);

          snprintf(filename, sizeof(filename),
	           "%s/Package/System/Library/StartupItems/%s/Resources/English.lproj/Localizable.strings",
	           directory, file->dst);
	  if ((fp = fopen(filename, "w")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to create init strings file \"%s\" - %s\n",
        	    filename, strerror(errno));
	    return (1);
	  }

	  fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", fp);
	  fputs("<!DOCTYPE plist SYSTEM \"file://localhost/System/Library/DTDs/PropertyList.dtd\">\n", fp);
	  fputs("<plist version=\"0.9\">\n", fp);
	  fputs("<dict>\n", fp);
	  fprintf(fp, "        <key>Starting %s</key>\n", dist->product);
	  fprintf(fp, "        <string>Starting %s</string>\n", dist->product);
	  fputs("</dict>\n", fp);
	  fputs("</plist>\n", fp);

	  fclose(fp);
          break;
      case 'd' :
          if (strncmp(file->dst, "/etc/", 5) == 0 ||
	      strcmp(file->dst, "/etc") == 0)
            snprintf(filename, sizeof(filename), "%s/Package/private%s",
	             directory, file->dst);
          else
            snprintf(filename, sizeof(filename), "%s/Package%s",
	             directory, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          if (strncmp(file->dst, "/etc/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s/Package/private%s",
	             directory, file->dst);
          else
            snprintf(filename, sizeof(filename), "%s/Package%s",
	             directory, file->dst);

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
    puts("Building OSX package...");

  if (directory[0] == '/')
  {
    strncpy(filename, directory, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';
  }
  else
    snprintf(filename, sizeof(filename), "%s/%s", current, directory);

  if (run_command(NULL, "/Developer/Applications/PackageMaker.app/"
                        "Contents/MacOS/PackageMaker -build "
			"-p %s/%s.pkg -f %s/Package -r %s/Resources",
		  filename, prodname, filename, filename))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    run_command(NULL, "/bin/rm -rf %s/Resources", directory);
    run_command(NULL, "/bin/rm -rf %s/Package", directory);
  }

  return (0);
}


/*
 * End of "$Id: osx.c,v 1.3 2002/10/17 17:21:22 swdev Exp $".
 */
