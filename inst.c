/*
 * "$Id: inst.c,v 1.10 2001/01/03 20:41:33 mike Exp $"
 *
 *   IRIX package gateway for the ESP Package Manager (EPM).
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
 *   make_inst() - Make an IRIX software distribution package.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * Local functions...
 */

static int	compare_files(const file_t *f0, const file_t *f1);


/*
 * 'make_inst()' - Make an IRIX software distribution package.
 */

int					/* O - 0 = success, 1 = fail */
make_inst(const char     *prodname,	/* I - Product short name */
          const char     *directory,	/* I - Directory for distribution files */
          const char     *platname,	/* I - Platform name */
          dist_t         *dist,		/* I - Distribution information */
	  struct utsname *platform)	/* I - Platform information */
{
  int		i;			/* Looping var */
  FILE		*fp;			/* Spec/IDB/script file */
  tarf_t	*tarfile;		/* .tardist file */
  char		name[1024];		/* Full product name */
  char		specname[1024];		/* Spec filename */
  char		idbname[1024];		/* IDB filename */
  char		filename[1024],		/* Destination filename */
		srcname[1024],		/* Name of source file in distribution */
		dstname[1024];		/* Name of destination file in distribution */
  char		command[1024];		/* Command to run */
  char		subsys[255];		/* Subsystem name */
  file_t	*file;			/* Current distribution file */
  struct stat	fileinfo;		/* File information */
  static const char *extensions[] =	/* INST file extensions */
		{
		  "",
		  ".idb",
		  ".man",
		  ".sw"
		};


  if (Verbosity)
    puts("Creating inst distribution...");

  if (platname[0])
    sprintf(name, "%s-%s-%s", prodname, dist->version, platname);
  else
    sprintf(name, "%s-%s", prodname, dist->version);

 /*
  * Write the spec file for gendist...
  */

  if (Verbosity)
    puts("Creating spec file...");

  sprintf(specname, "%s/spec", directory);

  if ((fp = fopen(specname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create spec file \"%s\" - %s\n", specname,
            strerror(errno));
    return (1);
  }

  fprintf(fp, "product %s\n", prodname);
  fprintf(fp, "	id \"%s, %s\"\n", dist->product, dist->version);

  fputs("	image sw\n", fp);
  fprintf(fp, "		id \"%s, Software, %s\"\n", dist->product,
          dist->version);
  fprintf(fp, "		version %d\n", dist->vernumber);
  fputs("		subsys eoe default\n", fp);
  fprintf(fp, "			id \"%s, Software, %s\"\n", dist->product,
          dist->version);
  fprintf(fp, "			exp \"%s.sw.eoe\"\n", prodname);

  if (dist->num_requires)
  {
    fputs("			prereq\n", fp);
    fputs("			(\n", fp);
    for (i = 0; i < dist->num_requires; i ++)
      if (strchr(dist->requires[i], '.') != NULL)
	fprintf(fp, "				%s 0 maxint\n",
        	dist->requires[i]);
      else if (dist->requires[i][0] != '/')
	fprintf(fp, "				%s.sw.eoe 0 maxint\n",
        	dist->requires[i]);
    fputs("			)\n", fp);
  }

  for (i = 0; i < dist->num_replaces; i ++)
    if (strchr(dist->replaces[i], '.') != NULL)
      fprintf(fp, "			replaces %s 0 maxint\n",
              dist->replaces[i]);
    else if (dist->replaces[i][0] != '/')
      fprintf(fp, "			replaces %s.sw.eoe 0 maxint\n",
              dist->replaces[i]);

  for (i = 0; i < dist->num_incompats; i ++)
    if (strchr(dist->incompats[i], '.') != NULL)
      fprintf(fp, "			incompat %s 0 maxint\n",
              dist->incompats[i]);
    else if (dist->incompats[i][0] != '/')
      fprintf(fp, "			incompat %s.sw.eoe 0 maxint\n",
              dist->incompats[i]);

  fputs("		endsubsys\n", fp);
  fputs("	endimage\n", fp);

  fputs("	image man\n", fp);
  fprintf(fp, "		id \"%s, Man Pages, %s\"\n", dist->product,
          dist->version);
  fprintf(fp, "		version %d\n", dist->vernumber);
  fputs("		subsys eoe default\n", fp);
  fprintf(fp, "			id \"%s, Man Pages, %s\"\n", dist->product,
          dist->version);
  fprintf(fp, "			exp \"%s.man.eoe\"\n", prodname);
  fputs("		endsubsys\n", fp);
  fputs("	endimage\n", fp);

  fputs("endproduct\n", fp);

  fclose(fp);

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
      sprintf(file->src, "../init.d/%s", dist->files[i].dst);
      sprintf(file->dst, "/etc/rc0.d/K00%s", dist->files[i].dst);

      file = add_file(dist);
      file->type = 'l';
      file->mode = 0;
      strcpy(file->user, "root");
      strcpy(file->group, "sys");
      sprintf(file->src, "../init.d/%s", dist->files[i].dst);
      sprintf(file->dst, "/etc/rc2.d/S99%s", dist->files[i].dst);

      file = dist->files + i;

      sprintf(filename, "/etc/init.d/%s", file->dst);
      strcpy(file->dst, filename);
    }

 /*
  * Add remove and removal scripts as needed...
  */

  if (dist->num_installs)
  {
   /*
    * Add the install script file to the list...
    */

    file = add_file(dist);
    file->type = 'E';
    file->mode = 0555;
    strcpy(file->user, "root");
    strcpy(file->group, "sys");
    sprintf(file->src, "%s/%s.install", directory, prodname);
    sprintf(file->dst, "/etc/software/%s.install", prodname);

   /*
    * Then create the install script...
    */

    if (Verbosity)
      puts("Creating exitops script...");

    sprintf(filename, "%s/%s.install", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = 0; i < dist->num_installs; i ++)
      fprintf(fp, "%s\n", dist->installs[i]);

    fclose(fp);
  }

  if (dist->num_removes)
  {
   /*
    * Add the remove script file to the list...
    */

    file = add_file(dist);
    file->type = 'R';
    file->mode = 0555;
    strcpy(file->user, "root");
    strcpy(file->group, "sys");
    sprintf(file->src, "%s/%s.remove", directory, prodname);
    sprintf(file->dst, "/etc/software/%s.remove", prodname);

   /*
    * Then create the remove script...
    */

    if (Verbosity)
      puts("Creating removeop script...");

    sprintf(filename, "%s/%s.remove", directory, prodname);

    if ((fp = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "epm: Unable to create script file \"%s\" - %s\n", filename,
              strerror(errno));
      return (1);
    }

    fchmod(fileno(fp), 0755);

    fputs("#!/bin/sh\n", fp);
    fputs("# " EPM_VERSION "\n", fp);

    for (i = 0; i < dist->num_removes; i ++)
      fprintf(fp, "%s\n", dist->removes[i]);

    fclose(fp);
  }

 /*
  * Sort the file list by the destination name, since gendist needs a sorted
  * list...
  */

  if (dist->num_files > 1)
    qsort(dist->files, dist->num_files, sizeof(file_t),
          (int (*)(const void *, const void *))compare_files);

 /*
  * Write the idb file for gendist...
  */

  if (Verbosity)
    puts("Creating idb file...");

  sprintf(idbname, "%s/idb", directory);

  if ((fp = fopen(idbname, "w")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create idb file \"%s\" - %s\n", idbname,
            strerror(errno));
    return (1);
  }

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
  {
    if (strstr(file->dst, "/man/") != NULL ||
        strstr(file->dst, "/catman/") != NULL)
      sprintf(subsys, "%s.man.eoe", prodname);
    else
      sprintf(subsys, "%s.sw.eoe", prodname);

    switch (tolower(file->type))
    {
      case 'c' :
          fprintf(fp, "f %04o %s %s %s %s %s config(suggest)\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys);
          break;
      case 'd' :
          fprintf(fp, "d %04o %s %s %s - %s\n", file->mode,
	          file->user, file->group, file->dst + 1, subsys);
          break;
      case 'e' :
          fprintf(fp, "f %04o %s %s %s %s %s exitop($rbase/%s)\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys,
		  file->dst + 1);
          break;
      case 'f' :
          fprintf(fp, "f %04o %s %s %s %s %s\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys);
          break;
      case 'i' :
          fprintf(fp, "f %04o %s %s %s %s %s exitop($rbase/etc/init.d/%s start) removeop($rbase/etc/init.d/%s stop)\n",
	          file->mode, file->user, file->group, file->dst + 1,
		  file->src, subsys, file->dst + 12, file->dst + 12);
          break;
      case 'l' :
          fprintf(fp, "l %04o %s %s %s - %s symval(%s)\n", file->mode,
	          file->user, file->group, file->dst + 1, subsys, file->src);
          break;
      case 'r' :
          fprintf(fp, "f %04o %s %s %s %s %s removeop($rbase/%s)\n", file->mode,
	          file->user, file->group, file->dst + 1, file->src, subsys,
		  file->dst + 1);
          break;
    }
  }

  fclose(fp);

 /*
  * Build the distribution from the spec file...
  */

  if (Verbosity)
    puts("Building INST binary distribution...");

  sprintf(command, "gendist %s -dist %s -sbase . -idb %s -spec %s",
          Verbosity == 0 ? "" : "-v", directory, idbname, specname);

  if (system(command))
    return (1);

 /*
  * Build the tardist file...
  */

  if (Verbosity)
    printf("Writing tardist file:");

  sprintf(filename, "%s/%s.tardist", directory, name);
  if ((tarfile = tar_open(filename, 0)) == NULL)
  {
    if (Verbosity)
      puts("");

    fprintf(stderr, "epm: Unable to create file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (1);
  }

  for (i = 0; i < 4; i ++)
  {
    sprintf(srcname, "%s/%s%s", directory, prodname, extensions[i]);
    sprintf(dstname, "%s%s", prodname, extensions[i]);

    stat(srcname, &fileinfo);
    if (tar_header(tarfile, TAR_NORMAL, fileinfo.st_mode, fileinfo.st_size,
	           fileinfo.st_mtime, "root", "sys", dstname, NULL) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file header - %s\n",
	      strerror(errno));
      return (-1);
    }

    if (tar_file(tarfile, srcname) < 0)
    {
      if (Verbosity)
        puts("");

      fprintf(stderr, "epm: Error writing file data for %s -\n    %s\n",
	      dstname, strerror(errno));
      return (-1);
    }

    if (Verbosity)
    {
      printf(" %s%s", prodname, extensions[i]);
      fflush(stdout);
    }
  }

  tar_close(tarfile);

  if (Verbosity)
  {
    stat(filename, &fileinfo);
    if (fileinfo.st_size > (1024 * 1024))
      printf(" size=%.1fM\n", fileinfo.st_size / 1024.0 / 1024.0);
    else
      printf(" size=%.0fk\n", fileinfo.st_size / 1024.0);
  }

 /*
  * Remove temporary files...
  */

  if (Verbosity)
    puts("Removing temporary distribution files...");

  sprintf(filename, "%s/%s.install", directory, prodname);
  unlink(filename);

  sprintf(filename, "%s/%s.remove", directory, prodname);
  unlink(filename);

  unlink(idbname);
  unlink(specname);

  return (0);
}


/*
 * 'compare_files()' - Compare the destination filenames.
 */

static int			/* O - Result of comparison */
compare_files(const file_t *f0,	/* I - First file */
              const file_t *f1)	/* I - Second file */
{
  return (strcmp(f0->dst, f1->dst));
}


/*
 * End of "$Id: inst.c,v 1.10 2001/01/03 20:41:33 mike Exp $".
 */
