/*
 * "$Id: aix.c,v 1.3 2001/06/26 16:22:21 mike Exp $"
 *
 *   AIX package gateway for the ESP Package Manager (EPM).
 *
 *   NOTE: AIX SUPPORT IS CURRENTLY NON-FUNCTIONAL!  The backup file
 *         is created, but installp and bffcreate will not read it
 *         for some reason.
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
 *   make_aix() - Make an AIX software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


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
  command_t		*c;		/* Current command */
  depend_t		*d;		/* Current dependency */
  file_t		*file;		/* Current distribution file */
  struct passwd		*pwd;		/* Pointer to user record */
  struct group		*grp;		/* Pointer to group record */
  static const char	*files[] =	/* Control files... */
			{
			  "al",
			  "cfgfiles",
			  "copyright",
			  "inventory",
			  "post_i",
			  "pre_i",
			  "pre_rm"
			};


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
  * Write the lpp_name file for bffcreate...
  */

  if (Verbosity)
    puts("Creating lpp_name file...");

  snprintf(filename, sizeof(filename), "%s/lpp_name", directory);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create lpp_name file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "4 R I %s {\n", prodname);
  fprintf(fp, "%s %s 1 N B x #%s\n", prodname, dist->version, dist->product);

 /*
  * Dependencies...
  */

  fputs("[\n", fp);
  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES)
      fprintf(fp, "*prereq %s %s", d->product, d->version[0]);

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
      fprintf(fp, "%s %s", d->product, d->version[0]);

 /*
  * Fix information is only used for updates (patches)...
  */

  fputs("%\n", fp);
  fputs("]\n", fp);
  fputs("}\n", fp);

  fclose(fp);

 /*
  * Write the product.al file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .al file...");

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
          fprintf(fp, "./etc/rc.d/rc2.d/S99%s\n", file->dst);
	  break;
      default :
          fprintf(fp, ".%s\n", file->dst);
	  break;
    }

  fclose(fp);

 /*
  * Write the product.cfgfiles file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .cfgfiles file...");

  snprintf(filename, sizeof(filename), "%s/%s.cfgfiles", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .cfgfiles file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'c')
    {
      fprintf(fp, ".%s hold_new\n", file->dst);
    }

  fclose(fp);

 /*
  * Write the product.copyright file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .copyright file...");

  snprintf(filename, sizeof(filename), "%s/%s.copyright", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .copyright file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "%s, %s\n%s\n%s\n", dist->product, dist->version,
          dist->vendor, dist->copyright);

  fclose(fp);

 /*
  * Write the product.pre_i file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .pre_i file...");

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
  * Write the product.post_i file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .post_i file...");

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
  * Write the product.pre_rm file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .pre_rm file...");

  snprintf(filename, sizeof(filename), "%s/%s.pre_rm", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .pre_rm file \"%s\" - %s\n", filename,
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

 /*
  * Write the product.inventory file for bffcreate...
  */

  if (Verbosity)
    puts("Creating .inventory file...");

  snprintf(filename, sizeof(filename), "%s/%s.inventory", directory, prodname);

  if ((fp = fopen(filename, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create .inventory file \"%s\" - %s\n", filename,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    switch (tolower(file->type))
    {
      case 'i' :
          fprintf(fp, "./etc/rc.d/rc2.d/S99%s:\n", file->dst);
	  break;
      default :
          fprintf(fp, ".%s:\n", file->dst);
	  break;
    }

    fputs("class=ClassName\n", fp);

    switch (tolower(file->type))
    {
      case 'd' :
          fputs("type=DIRECTORY\n", fp);
          fputs("size=1\n", fp);
          break;
      case 'l' :
          fputs("type=SYMLINK\n", fp);
	  fprintf(fp, "target=%s\n", file->src);
          fputs("size=1\n", fp);
          break;
      default :
          fputs("type=FILE\n", fp);
          if (!stat(file->src, &fileinfo))
	    fprintf(fp, "size=%d\n", (int)(fileinfo.st_size + 511) / 512);
	  else
            fputs("size=1\n", fp);
	  break;
    }
    fprintf(fp, "owner=%s\n", file->user);
    fprintf(fp, "group=%s\n", file->group);
    fprintf(fp, "mode=%04o\n", file->mode);
/*    fprintf(fp, "checksum=\n", );*/
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

  if (Verbosity)
    puts("Creating liblpp.a archive...");

  snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root", directory, prodname);
  make_directory(filename, 0755, 0, 0);

  if (run_command(directory, "ar rc usr/lpp/%s/inst_root/liblpp.a lpp.README",
                  prodname))
    return (1);

  for (i = 0; i < (sizeof(files) / sizeof(files[0])); i ++)
  {
    if (run_command(directory, "ar rc usr/lpp/%s/inst_root/liblpp.a %s.%s",
                    prodname, prodname, files[i]))
      return (1);
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
          if (strncmp(file->dst, "/usr/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s%s", directory, file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	            directory, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'i' :
          snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	          directory, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("%s -> %s...\n", file->src, filename);

	  if (copy_file(filename, file->src, file->mode, pwd ? pwd->pw_uid : 0,
			grp ? grp->gr_gid : 0))
	    return (1);
          break;
      case 'd' :
          if (strncmp(file->dst, "/usr/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s%s", directory, file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	            directory, prodname, file->dst);

	  if (Verbosity > 1)
	    printf("Directory %s...\n", filename);

          make_directory(filename, file->mode, pwd ? pwd->pw_uid : 0,
			 grp ? grp->gr_gid : 0);
          break;
      case 'l' :
          if (strncmp(file->dst, "/usr/", 5) == 0)
            snprintf(filename, sizeof(filename), "%s%s", directory, file->dst);
	  else
            snprintf(filename, sizeof(filename), "%s/usr/lpp/%s/inst_root/etc/rc.d/rc2.d/%s",
	            directory, prodname, file->dst);

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

  if (run_command(directory, "sh -c (echo lpp_name; find usr -print) | "
                             "backup -i -f %s.bff -q %s",
                  prodname, Verbosity ? "-v" : ""))
    return (1);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    run_command(NULL, "/bin/rm -rf %s/usr", directory);

    for (i = 0; i < (sizeof(files) / sizeof(files[0])); i ++)
    {
      snprintf(filename, sizeof(filename), "%s/%s.%s", directory, prodname, files[i]);
      unlink(filename);
    }

    snprintf(filename, sizeof(filename), "%s/lpp_name", directory);
    unlink(filename);
    snprintf(filename, sizeof(filename), "%s/lpp.README", directory);
    unlink(filename);
  }

  return (0);
}


/*
 * End of "$Id: aix.c,v 1.3 2001/06/26 16:22:21 mike Exp $".
 */
