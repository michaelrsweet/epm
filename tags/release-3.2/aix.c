/*
 * "$Id: aix.c,v 1.7 2001/10/26 17:32:04 mike Exp $"
 *
 *   AIX package gateway for the ESP Package Manager (EPM).
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
 *   make_aix()     - Make an AIX software distribution package.
 *   aix_version()  - Generate an AIX version number.
 *   write_liblpp() - Create the liblpp.a file for the root or /usr parts.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static char	*aix_version(const char *version);
static int	write_liblpp(const char *prodname,
		             const char *directory,
		             dist_t *dist, int root);


/*
 * Local globals...
 */

static const char	*files[] =	/* Control files... */
			{
			  "al",
			  "cfgfiles",
			  "copyright",
			  "inventory",
			  "post_i",
			  "pre_i",
			  "unpost_i"
			};


/*
 * 'make_aix()' - Make an AIX software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_aix(const char     *prodname,	/* I - Product short name */
         const char     *directory,	/* I - Directory for distribution files */
         const char     *platname,	/* I - Platform name */
         dist_t         *dist,		/* I - Distribution information */
	 struct utsname *platform)	/* I - Platform information */
{
  int			i;		/* Looping var */
  FILE			*fp;		/* Control file */
  char			name[1024];	/* Full product name */
  char			filename[1024];	/* Destination filename */
  char			current[1024];	/* Current directory */
  int			blocks;		/* Number of blocks needed */
  struct stat		fileinfo;	/* File information */
  depend_t		*d;		/* Current dependency */
  file_t		*file;		/* Current distribution file */
  struct passwd		*pwd;		/* Pointer to user record */
  struct group		*grp;		/* Pointer to group record */


  REF(platform);

  if (Verbosity)
    puts("Creating AIX distribution...");

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

  getcwd(current, sizeof(current));

 /*
  * Write the lpp_name file for installp...
  */

  if (Verbosity)
    puts("Creating lpp_name file...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);
  make_directory(filename, 0755, 0, 0);

  snprintf(filename, sizeof(filename), "%s/%s/lpp_name", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create lpp_name file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "4 R I %s {\n", prodname);
  fprintf(fp, "%s %s 01 N B x %s\n", prodname,
          aix_version(dist->version), dist->product);

 /*
  * Dependencies...
  */

  fputs("[\n", fp);
  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES)
      fprintf(fp, "*prereq %s %s\n", d->product, aix_version(d->version[0]));

 /*
  * Installation sizes...
  */

  fputs("%\n", fp);
  for (i = dist->num_files, file = dist->files, blocks = 0; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr/", 5) != 0)
      switch (tolower(file->type))
      {
        case 'd' :
	case 'l' :
	    blocks ++;
	    break;

	default :
	    if (!stat(file->src, &fileinfo))
	      blocks += (fileinfo.st_size + 511) / 512;
	    break;
      }
  fprintf(fp, "/ %d\n", blocks);

  for (i = dist->num_files, file = dist->files, blocks = 0; i > 0; i --, file ++)
    if (strncmp(file->dst, "/usr/", 5) == 0)
      switch (tolower(file->type))
      {
        case 'd' :
	case 'l' :
	    blocks ++;
	    break;

	default :
	    if (!stat(file->src, &fileinfo))
	      blocks += (fileinfo.st_size + 511) / 512;
	    break;
      }
  fprintf(fp, "/usr %d\n", blocks);

 /*
  * This package supercedes which others?
  */

  fputs("%\n", fp);
  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REPLACES)
      fprintf(fp, "%s %s", d->product, aix_version(d->version[0]));

 /*
  * Fix information is only used for updates (patches)...
  */

  fputs("%\n", fp);
  fputs("]\n", fp);
  fputs("}\n", fp);

  fclose(fp);

 /*
  * Write the root partition liblpp.a file...
  */

  write_liblpp(prodname, directory, dist, 1);

 /*
  * Write the usr partition liblpp.a file...
  */

  write_liblpp(prodname, directory, dist, 0);

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
          if (strncmp(file->dst, "/usr/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodname,
	             file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	             directory, prodname, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	           directory, prodname, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          if (strncmp(file->dst, "/usr/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodname,
	             file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	             directory, prodname, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          if (strncmp(file->dst, "/usr/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s/%s%s", directory, prodname,
	             file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	             directory, prodname, prodname, file->dst);

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
    puts("Building AIX binary distribution...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);

  if (run_command(filename, "sh -c \'find . -print | backup -i -f ../%s.bff -q %s\'",
                  prodname, Verbosity ? "-v" : "", prodname))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    run_command(NULL, "/bin/rm -rf %s/%s", directory, prodname);

    for (i = 0; i < (sizeof(files) / sizeof(files[0])); i ++)
    {
      snprintf(filename, sizeof(filename), "%s/%s.%s", directory, prodname, files[i]);
      unlink(filename);
    }

    snprintf(filename, sizeof(filename), "%s/lpp.README", directory);
    unlink(filename);
  }

  return (0);
}


/*
 * 'aix_version()' - Generate an AIX version number.
 */

static char *				/* O - AIX version number */
aix_version(const char *version)	/* I - EPM version number */
{
  int		verparts[4];		/* Version number parts */
  static char	aix[255];		/* AIX version number string */


 /*
  * AIX requires a four-part version number (M.m.p.r)...
  */

  memset(verparts, 0, sizeof(verparts));
  sscanf(version, "%d.%d.%d.%d", verparts + 0, verparts + 1,
         verparts + 2, verparts + 3);
  sprintf(aix, "%d.%d.%d.%d", verparts[0], verparts[1],
          verparts[2], verparts[3]);

  return (aix);
}


/*
 * 'write_liblpp()' - Create the liblpp.a file for the root or /usr parts.
 */

static int				/* O - 0 = success, 1 = fail */
write_liblpp(const char     *prodname,	/* I - Product short name */
             const char     *directory,	/* I - Directory for distribution files */
             dist_t         *dist,	/* I - Distribution information */
             int            root)	/* I - Root partition? */
{
  int			i;		/* Looping var */
  FILE			*fp;		/* Control file */
  char			filename[1024];	/* Destination filename */
  struct stat		fileinfo;	/* File information */
  command_t		*c;		/* Current command */
  file_t		*file;		/* Current distribution file */


 /*
  * Progress info...
  */

  if (Verbosity)
    puts(root ? "Creating root partition liblpp.a file..." :
                "Creating /usr partition liblpp.a file...");

 /*
  * Write the product.al file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .al file...");

  snprintf(filename, sizeof(filename), "%s/%s.al", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .al file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    switch (tolower(file->type))
    {
      case 'i' :
          if (root)
            fprintf(fp, "./etc/rc.d/rc2.d/S99%s\n", file->dst);
	  break;
      default :
          if ((strncmp(file->dst, "/usr/", 5) != 0) == root)
            fprintf(fp, ".%s\n", file->dst);
	  break;
    }

  fclose(fp);

 /*
  * Write the product.cfgfiles file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .cfgfiles file...");

  snprintf(filename, sizeof(filename), "%s/%s.cfgfiles", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .cfgfiles file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c' &&
        (strncmp(file->dst, "/usr/", 5) != 0) == root)
    {
      fprintf(fp, ".%s hold_new\n", file->dst);
    }

  fclose(fp);

 /*
  * Write the product.copyright file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .copyright file...");

  snprintf(filename, sizeof(filename), "%s/%s.copyright", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .copyright file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "%s, %s\n%s\n%s\n", dist->product, aix_version(dist->version),
          dist->vendor, dist->copyright);

  fclose(fp);

  if (root)
  {
   /*
    * Write the product.pre_i file for installp...
    */

    if (Verbosity > 1)
      puts("    Creating .pre_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.pre_i", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .pre_i file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);

   /*
    * Write the product.post_i file for installp...
    */

    if (Verbosity > 1)
      puts("    Creating .post_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.post_i", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .post_i file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);

   /*
    * Write the product.unpost_i file for installp...
    */

    if (Verbosity > 1)
      puts("    Creating .unpost_i file...");

    snprintf(filename, sizeof(filename), "%s/%s.unpost_i", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create .unpost_i file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (c = dist->commands, i = dist->num_commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE)
	fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }

 /*
  * Write the product.inventory file for installp...
  */

  if (Verbosity > 1)
    puts("    Creating .inventory file...");

  snprintf(filename, sizeof(filename), "%s/%s.inventory", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .inventory file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    if (root)
    {
      if (strncmp(file->dst, "/usr/", 5) == 0)
        continue;
    }
    else
    {
      if (tolower(file->type) == 'c' ||
          strncmp(file->dst, "/usr/", 5) != 0)
	continue;
    }

    switch (tolower(file->type))
    {
      case 'i' :
          fprintf(fp, "/etc/rc.d/rc2.d/S99%s:\n", file->dst);
	  break;
      default :
          fprintf(fp, "%s:\n", file->dst);
	  break;
    }

    fprintf(fp, "    class=apply,inventory,%s\n", prodname);

    switch (tolower(file->type))
    {
      case 'd' :
          fputs("    type=DIRECTORY\n", fp);
          break;
      case 'l' :
          fputs("    type=SYMLINK\n", fp);
	  fprintf(fp, "    target=%s\n", file->src);
          break;
      default :
          fputs("    type=FILE\n", fp);
          if (!stat(file->src, &fileinfo))
	    fprintf(fp, "    size=%d\n", (int)fileinfo.st_size);
	  break;
    }
    fprintf(fp, "    owner=%s\n", file->user);
    fprintf(fp, "    group=%s\n", file->group);
    fprintf(fp, "    mode=%04o\n", file->mode);
    fputs("\n", fp);
  }

  fclose(fp);

 /*
  * Write the lpp.README file...
  */

  snprintf(filename, sizeof(filename), "%s/lpp.README", directory);
  copy_file(filename, dist->license, 0644, 0, 0);

 /*
  * Create the liblpp.a file...
  */

  if (Verbosity > 1)
    puts("    Creating liblpp.a archive...");

  if (root)
  {
    snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s/inst_root",
             directory, prodname, prodname);
    make_directory(filename, 0755, 0, 0);

    snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root/liblpp.a",
             prodname, prodname);
  }
  else
  {
    snprintf(filename, sizeof(filename), "%s/%s/usr/lpp/%s",
             directory, prodname, prodname);
    make_directory(filename, 0755, 0, 0);

    snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/liblpp.a",
             prodname, prodname);
  }

  if (run_command(directory, "ar rc %s lpp.README", filename))
    return (1);

  for (i = 0; i < (sizeof(files) / sizeof(files[0])); i ++)
  {
    if (i >= 4 && !root)
      break;

    if (run_command(directory, "ar rc %s %s.%s",
                    filename, prodname, files[i]))
      return (1);
  }

  return (0);
}

/*
 * End of "$Id: aix.c,v 1.7 2001/10/26 17:32:04 mike Exp $".
 */
