/*
 * "$Id: osx.c,v 1.5 2002/10/17 17:31:17 mike Exp $"
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

  snprintf(filename, sizeof(filename), "%s/Resources/Description.plist", directory);
  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create description file \"%s\" - %s\n",
            filename, strerror(errno));
    return (1);
  }

  fputs("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n", fp);
  fputs("<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n", fp);
  fputs("<plist version=\"1.0\">\n", fp);
  fputs("<dict>\n", fp);
  fputs("        <key>IFPkgDescriptionDeleteWarning</key>\n", fp);
  fputs("        <string></string>\n", fp);
  fputs("        <key>IFPkgDescriptionDescription</key>\n", fp);
  fputs("        <string>", fp);
  for (i = 0; i < dist->num_descriptions; i ++)
    fprintf(fp, "%s\n", dist->descriptions[i]);
  fputs("</string>\n", fp);
  fputs("        <key>IFPkgDescriptionTitle</key>\n", fp);
  fprintf(fp, "        <string>%s</string>\n", dist->product);
  fputs("        <key>IFPkgDescriptionVersion</key>\n", fp);
  fprintf(fp, "        <string>%s</string>\n", dist->version);
  fputs("</dict>\n", fp);
  fputs("</plist>\n", fp);

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

  run_command(NULL, "/Developer/Applications/PackageMaker.app/"
                    "Contents/MacOS/PackageMaker -build "
		    "-p %s/%s.pkg -f %s/Package -r %s/Resources",
	      filename, prodname, filename, filename);

  snprintf(filename, sizeof(filename), "%s/%s.pkg", directory, prodname);
  if (access(filename, 0))
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
 * End of "$Id: osx.c,v 1.5 2002/10/17 17:31:17 mike Exp $".
 */
