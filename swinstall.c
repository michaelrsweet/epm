/*
 * "$Id: swinstall.c,v 1.14 2001/06/26 16:22:22 mike Exp $"
 *
 *   HP-UX package gateway for the ESP Package Manager (EPM).
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
 *   make_swinstall() - Make an HP-UX software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'make_swinstall()' - Make an HP-UX software distribution package.
 */

int						/* O - 0 = success, 1 = fail */
make_swinstall(const char     *prodname,	/* I - Product short name */
               const char     *directory,	/* I - Directory for distribution files */
               const char     *platname,	/* I - Platform name */
               dist_t         *dist,		/* I - Distribution information */
	       struct utsname *platform)	/* I - Platform information */
{
  int		i, j;			/* Looping vars */
  FILE		*fp;			/* Spec/script file */
  tarf_t	*tarfile;		/* .tardist file */
  int		linknum;		/* Symlink number */
  char		name[1024];		/* Full product name */
  char		infoname[1024],		/* Info filename */
		preinstall[1024],	/* preinstall script */
		postinstall[1024],	/* postinstall script */
		preremove[1024],	/* preremove script */
		postremove[1024];	/* postremove script */
  char		filename[1024];		/* Destination filename */
  file_t	*file;			/* Current distribution file */
  command_t	*c;			/* Current command */
  depend_t	*d;			/* Current dependency */


  (void)platform; /* Eliminates compiler warning about unused variable */

  if (Verbosity)
    puts("Creating swinstall distribution...");

  if (dist->relnumber)
  {
    if (platname[0])
      snprintf(name, sizeof(name), "%s-%s-%d-%s", prodname,
               dist->version, dist->relnumber, platname);
    else
      snprintf(name, sizeof(name), "%s-%s-%d", prodname,
               dist->version, dist->relnumber);
  }
  else if (platname[0])
    snprintf(name, sizeof(name), "%s-%s-%s", prodname, dist->version, platname);
  else
    snprintf(name, sizeof(name), "%s-%s", prodname, dist->version);

 /*
  * Add symlinks for init scripts...
  */

  for (i = 0; i < dist->num_files; i ++)
    if (tolower(dist->files[i].type) == 'i')
    {
      file = add_file(dist);
      file->type = 'l';
      file->mode = 0;
      strcpy(file->user, "root");
      strcpy(file->group, "sys");
      snprintf(file->src, sizeof(file->src), "../init.d/%s",
               dist->files[i].dst);
      snprintf(file->dst, sizeof(file->dst), "/sbin/rc0.d/K000%s",
               dist->files[i].dst);

      file = add_file(dist);
      file->type = 'l';
      file->mode = 0;
      strcpy(file->user, "root");
      strcpy(file->group, "sys");
      snprintf(file->src, sizeof(file->src), "../init.d/%s",
               dist->files[i].dst);
      snprintf(file->dst, sizeof(file->dst), "/sbin/rc2d.d/S999%s",
               dist->files[i].dst);

      file = dist->files + i;

      snprintf(filename, sizeof(filename), "/sbin/init.d/%s", file->dst);
      strcpy(file->dst, filename);
    }

 /*
  * Write the preinstall script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_INSTALL)
      break;

  if (i)
  {
   /*
    * Create the preinstall script...
    */

    snprintf(preinstall, sizeof(preinstall), "%s/%s.preinst", directory,
             prodname);

    if (Verbosity)
      puts("Creating preinstall script...");

    if ((fp = fopen(preinstall, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", preinstall,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_INSTALL)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
	fprintf(fp, "/sbin/init.d/%s start\n", file->dst);

    fclose(fp);
  }
  else
    preinstall[0] = '\0';

 /*
  * Write the postinstall script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_INSTALL)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;

  if (i)
  {
   /*
    * Create the postinstall script...
    */

    snprintf(postinstall, sizeof(postinstall), "%s/%s.postinst", directory,
             prodname);

    if (Verbosity)
      puts("Creating postinstall script...");

    if ((fp = fopen(postinstall, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", postinstall,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_INSTALL)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
	fprintf(fp, "/sbin/init.d/%s start\n", file->dst);

    fclose(fp);
  }
  else
    postinstall[0] = '\0';

 /*
  * Write the preremove script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_PRE_REMOVE)
      break;

  if (!i)
    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
        break;

  if (i)
  {
   /*
    * Then create the remove script...
    */

    if (Verbosity)
      puts("Creating preremove script...");

    snprintf(preremove, sizeof(preremove), "%s/%s.prerm", directory, prodname);

    if ((fp = fopen(preremove, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", preremove,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (j = dist->num_files, file = dist->files; j > 0; j --, file ++)
      if (tolower(file->type) == 'i')
	fprintf(fp, "/sbin/init.d/%s stop\n", file->dst);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_PRE_REMOVE)
        fprintf(fp, "%s\n", c->command);

    fclose(fp);
  }
  else
    preremove[0] = '\0';

 /*
  * Write the postremove script if needed...
  */

  for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
    if (c->type == COMMAND_POST_REMOVE)
      break;

  if (i)
  {
   /*
    * Create the postremove script...
    */

    snprintf(postremove, sizeof(postremove), "%s/%s.postrm", directory,
             prodname);

    if (Verbosity)
      puts("Creating postremove script...");

    if ((fp = fopen(postremove, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", postremove,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = dist->num_commands, c = dist->commands; i > 0; i --, c ++)
      if (c->type == COMMAND_POST_REMOVE)
        fprintf(fp, "%s\n", c->command);

    for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
      if (tolower(file->type) == 'i')
	fprintf(fp, "/sbin/init.d/%s start\n", file->dst);

    fclose(fp);
  }
  else
    postremove[0] = '\0';

 /*
  * Create all symlinks...
  */

  if (Verbosity)
    puts("Creating symlinks...");

  for (i = dist->num_files, file = dist->files, linknum = 0;
       i > 0;
       i --, file ++)
    if (tolower(file->type) == 'l')
    {
      snprintf(filename, sizeof(filename), "%s/%s.link%04d", directory,
               prodname, linknum);
      symlink(file->src, filename);
      linknum ++;
    }

 /*
  * Write the info file for swpackage...
  */

  if (Verbosity)
    puts("Creating info file...");

  snprintf(infoname, sizeof(infoname), "%s/%s.info", directory, prodname);

  if ((fp = fopen(infoname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create info file \"%s\" - %s\n", infoname,
            strerror(errno));
    return (1);
  }

  fputs("product\n", fp);
  fprintf(fp, "  tag %s\n", prodname);
  fprintf(fp, "  revision %s\n", dist->version);
  fprintf(fp, "  title %s, %s\n", dist->product, dist->version);
  if (dist->num_descriptions)
    fprintf(fp, "  description %s\n", dist->descriptions[0]);
  fprintf(fp, "  copyright Copyright %s\n", dist->copyright);
  fprintf(fp, "  readme < %s\n", dist->license);
  fputs("  is_locatable false\n", fp);

  fputs("  fileset\n", fp);
  fputs("    tag all\n", fp);
  fprintf(fp, "    title %s, %s\n", dist->product, dist->version);

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REQUIRES && d->product[0] != '/')
      break;

  if (i)
  {
    fputs("    corequisites", fp);

    for (; i > 0; i --, d ++)
      if (d->type == DEPEND_REQUIRES && d->product[0] != '/')
      {
        fprintf(fp, " %s", d->product);
	if (d->vernumber[0] == 0)
	{
	  if (d->vernumber[1] < INT_MAX)
            fprintf(fp, ",r<=%s", d->version[1]);
	}
	else
	  fprintf(fp, ",r>=%s,r<=%s", d->version[0], d->version[1]);
      }

    fputs("\n", fp);
  }

  for (i = dist->num_depends, d = dist->depends; i > 0; i --, d ++)
    if (d->type == DEPEND_REPLACES && d->product[0] != '/')
      break;

  if (i)
  {
    fputs("    ancestor", fp);

    for (; i > 0; i --, d ++)
      if (d->type == DEPEND_REPLACES && d->product[0] != '/')
      {
        fprintf(fp, " %s", d->product);
	if (d->vernumber[0] == 0)
	{
	  if (d->vernumber[1] < INT_MAX)
            fprintf(fp, ",r<=%s", d->version[1]);
	}
	else
	  fprintf(fp, ",r>=%s,r<=%s", d->version[0], d->version[1]);
      }

    fputs("\n", fp);
  }

  if (preinstall[0])
    fprintf(fp, "    preinstall %s\n", preinstall);
  if (postinstall[0])
    fprintf(fp, "    postinstall %s\n", postinstall);
  if (preremove[0])
    fprintf(fp, "    preremove %s\n", preremove);
  if (postremove[0])
    fprintf(fp, "    postremove %s\n", postremove);

  for (i = dist->num_files, file = dist->files, linknum = 0;
       i > 0;
       i --, file ++)
  {
    switch (tolower(file->type))
    {
      case 'd' :
          fprintf(fp, "    file -m %o -o %s -g %s . %s\n", file->mode | S_IFDIR,
	          file->user, file->group, file->dst);
          break;
      case 'c' :
          fprintf(fp, "    file -m %04o -o %s -g %s -v %s %s\n", file->mode,
	          file->user, file->group, file->src, file->dst);
          break;
      case 'f' :
      case 'i' :
          fprintf(fp, "    file -m %04o -o %s -g %s %s %s\n", file->mode,
	          file->user, file->group, file->src, file->dst);
          break;
      case 'l' :
          snprintf(filename, sizeof(filename), "%s/%s.link%04d", directory,
	           prodname, linknum);
          linknum ++;
          fprintf(fp, "    file -o %s -g %s %s %s\n", file->user, file->group,
	          filename, file->dst);
          break;
    }
  }

  fputs("  end\n", fp);
  fputs("end\n", fp);

  fclose(fp);

 /*
  * Build the distribution from the spec file...
  */

  if (Verbosity)
    puts("Building swinstall binary distribution...");

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);
  mkdir(filename, 0777);

  if (run_command(NULL, "/usr/sbin/swpackage %s-s %s -d %s/%s "
                        "-x write_remote_files=true %s",
        	  Verbosity == 0 ? "" : "-v ", infoname, directory,
		  prodname, prodname))
    return (1);

 /*
  * Tar and compress the distribution...
  */

  if (Verbosity)
    puts("Creating tar.gz file for distribution...");

  snprintf(filename, sizeof(filename), "%s/%s.tar.gz", directory, name);

  if ((tarfile = tar_open(filename, 1)) == NULL)
    return (1);

  snprintf(filename, sizeof(filename), "%s/%s", directory, prodname);

  if (tar_directory(tarfile, filename, prodname))
  {
    tar_close(tarfile);
    return (1);
  }

  tar_close(tarfile);

 /*
  * Remove temporary files...
  */

  if (!KeepFiles)
  {
    if (Verbosity)
      puts("Removing temporary distribution files...");

    unlink(infoname);

    if (preinstall[0])
      unlink(preinstall);
    if (postinstall[0])
      unlink(postinstall);
    if (preremove[0])
      unlink(preremove);
    if (postremove[0])
      unlink(postremove);

    while (linknum > 0)
    {
      linknum --;
      snprintf(filename, sizeof(filename), "%s/%s.link%04d", directory,
               prodname, linknum);
      unlink(filename);
    }
  }

  return (0);
}


/*
 * End of "$Id: swinstall.c,v 1.14 2001/06/26 16:22:22 mike Exp $".
 */
