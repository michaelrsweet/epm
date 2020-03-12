/*
 * macOS package gateway for the ESP Package Manager (EPM).
 *
 * Copyright © 2002-2020 by Michael R Sweet
 * Copyright © 2002-2010 by Easy Software Products.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	make_package(int format, const char *prodname, const char *directory, dist_t *dist, const char *setup);


/*
 * 'make_osx()' - Make a Red Hat software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_osx(int            format,		/* I - Format */
	 const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform,	/* I - Platform information */
	 const char	*setup)		/* I - Setup GUI image */
{
  char		filename[1024];		/* Destination filename */


  REF(platname);
  REF(platform);

 /*
  * Create the main package and subpackages (if any)...
  */

  if (make_package(format, prodname, directory, dist, setup))
    return (1);

 /*
  * TODO: Copy uninstall application to disk image...
  */

 /*
  * Create a disk image of the package...
  */

  if (Verbosity)
    puts("Creating disk image...");

  if (dist->release[0])
    snprintf(filename, sizeof(filename), "%s-%s-%s", prodname, dist->version,
             dist->release);
  else
    snprintf(filename, sizeof(filename), "%s-%s", prodname, dist->version);

  if (platname[0])
  {
    strlcat(filename, "-", sizeof(filename));
    strlcat(filename, platname, sizeof(filename));
  }

  if (run_command(NULL, "hdiutil create -ov -srcfolder %s/%s.pkg %s/%s.dmg", directory, prodname, directory, filename))
  {
    fputs("epm: Unable to create disk image.\n", stderr);
    return (1);
  }

  return (0);
}


/*
 * 'make_package()' - Make a macOS package.
 */

static int
make_package(int        format,		/* I - Format */
	     const char *prodname,	/* I - Product short name */
	     const char *directory,	/* I - Directory for distribution files */
	     dist_t     *dist,		/* I - Distribution  information */
	     const char *setup)		/* I - Setup GUI image */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec file */
  char		prodfull[1024],		/* Full product name */
		title[1024],		/* Software title */
		filename[1024],		/* Destination filename */
		pkgname[1024];		/* Package name */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  struct passwd	*pwd;			/* Pointer to user record */
  struct group	*grp;			/* Pointer to group record */
  char		current[1024];		/* Current directory */
  const char	*option;		/* Init script option */


  strlcpy(prodfull, prodname, sizeof(prodfull));
  strlcpy(title, dist->product, sizeof(title));

  if (Verbosity)
    printf("Creating %s macOS package...\n", prodfull);

  getcwd(current, sizeof(current));

 /*
  * Copy the resources for the license, readme, and welcome (description)
  * stuff...
  */

  if (Verbosity)
    puts("Copying temporary resource files...");

  snprintf(filename, sizeof(filename), "%s/%s/Resources", directory, prodfull);
  make_directory(filename, 0755, 0, 0);

 /*
  * Do pre/post install commands...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      break;

  if (i)
  {
    snprintf(filename, sizeof(filename), "%s/%s/Resources/preinstall", directory, prodfull);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create preinstall script \"%s\": %s\n", filename, strerror(errno));
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
    snprintf(filename, sizeof(filename), "%s/%s/Resources/postinstall", directory, prodfull);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create postinstall script \"%s\": %s\n", filename, strerror(errno));
      return (1);
    }

    fputs("#!/bin/sh\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
	fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        qprintf(fp, "/Library/StartupItems/%s/%s start\n", file->dst, file->dst);

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
          if (!strncmp(file->dst, "/etc/", 5) || !strncmp(file->dst, "/var/", 5))
            snprintf(filename, sizeof(filename), "%s/%s/Package/private%s",
	             directory, prodfull, file->dst);
          else
            snprintf(filename, sizeof(filename), "%s/%s/Package%s",
	             directory, prodfull, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename),
	           "%s/%s/Package/Library/StartupItems/%s/%s",
	           directory, prodfull, file->dst, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);

          snprintf(filename, sizeof(filename),
	           "%s/%s/Package/Library/StartupItems/%s/StartupParameters.plist",
	           directory, prodfull, file->dst);
	  if ((fp = fopen(filename, "w")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to create init data file \"%s\": %s\n", filename, strerror(errno));
	    return (1);
	  }

          fputs("{\n", fp);
          fprintf(fp, "  Description = \"%s\";\n", dist->product);
	  qprintf(fp, "  Provides = (%s);\n",
	          get_option(file, "provides", file->dst));
          if ((option = get_option(file, "requires", NULL)) != NULL)
	    qprintf(fp, "  Requires = (%s);\n", option);
          if ((option = get_option(file, "uses", NULL)) != NULL)
	    qprintf(fp, "  Uses = (%s);\n", option);
          if ((option = get_option(file, "order", NULL)) != NULL)
	    qprintf(fp, "  OrderPreference = \"%s\";\n", option);
	  fputs("}\n", fp);

	  fclose(fp);

          snprintf(filename, sizeof(filename),
	           "%s/%s/Package/Library/StartupItems/%s/Resources/English.lproj",
	           directory, prodfull, file->dst);
          make_directory(filename, 0755, 0, 0);

          snprintf(filename, sizeof(filename),
	           "%s/%s/Package/Library/StartupItems/%s/Resources/English.lproj/Localizable.strings",
	           directory, prodfull, file->dst);
	  if ((fp = fopen(filename, "w")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to create init strings file \"%s\": %s\n", filename, strerror(errno));
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
          if (!strncmp(file->dst, "/etc/", 5) || !strncmp(file->dst, "/var/", 5) ||
	      !strcmp(file->dst, "/etc") || !strcmp(file->dst, "/var"))
            snprintf(filename, sizeof(filename), "%s/%s/Package/private%s",
	             directory, prodfull, file->dst);
          else
            snprintf(filename, sizeof(filename), "%s/%s/Package%s",
	             directory, prodfull, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0, grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          if (!strncmp(file->dst, "/etc/", 5) || !strncmp(file->dst, "/var/", 5))
            snprintf(filename, sizeof(filename), "%s/%s/Package/private%s",
	             directory, prodfull, file->dst);
          else
            snprintf(filename, sizeof(filename), "%s/%s/Package%s",
	             directory, prodfull, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

          make_link(filename, file->src);
          break;
    }
  }

 /*
  * Build the distribution...
  */

  if (Verbosity)
    puts("Building macOS package...");

  if (directory[0] == '/')
    strlcpy(filename, directory, sizeof(filename));
  else
    snprintf(filename, sizeof(filename), "%s/%s", current, directory);

 /*
  * The package stands alone - just put it in the output directory...
  */

  snprintf(pkgname, sizeof(pkgname), "%s/%s.pkg", filename, prodfull);

  if (format == PACKAGE_MACOS_SIGNED)
  {
    const char *identity = getenv("EPM_SIGNING_IDENTITY");
    if (!identity)
    {
      fputs("epm: Using default 'Developer ID Installer' signing identity.\n"
            "     Set the EPM_SIGNING_IDENTITY environment variable to override.\n", stderr);
      identity = "Developer ID Installer";
    }

    run_command(NULL, "/usr/bin/pkgbuild --identifier %s --version %s --ownership preserve --scripts %s/%s/Resources --root %s/%s/Package --sign '%s' %s", prodfull, dist->version, directory, prodfull, directory, prodfull, identity, pkgname);
  }
  else
  {
    run_command(NULL, "/usr/bin/pkgbuild --identifier %s --version %s --ownership preserve --scripts %s/%s/Resources --root %s/%s/Package %s", prodfull, dist->version, directory, prodfull, directory, prodfull, pkgname);
  }

 /*
  * Verify that the package was created...
  */

  if (access(pkgname, 0))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    snprintf(filename, sizeof(filename), "%s/%s", directory, prodfull);
    unlink_directory(filename);
  }

  return (0);
}
