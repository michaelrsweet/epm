/*
 * "$Id: file.c,v 1.19 2005/01/11 21:36:57 mike Exp $"
 *
 *   File functions for the ESP Package Manager (EPM).
 *
 *   Copyright 1999-2005 by Easy Software Products.
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
 *   copy_file()      - Copy a file...
 *   make_directory() - Make a directory.
 *   make_link()      - Make a symbolic link.
 *   strip_execs()    - Strip symbols from executable files in the
 *                      distribution.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'copy_file()' - Copy a file...
 */

int					/* O - 0 on success, -1 on failure */
copy_file(const char *dst,		/* I - Destination file */
          const char *src,		/* I - Source file */
          mode_t     mode,		/* I - Permissions */
	  uid_t      owner,		/* I - Owner ID */
	  gid_t      group)		/* I - Group ID */
{
  FILE		*dstfile,		/* Destination file */
		*srcfile;		/* Source file */
  char		buffer[8192];		/* Copy buffer */
  char		*slash;			/* Pointer to trailing slash */
  size_t	bytes;			/* Number of bytes read/written */


 /*
  * Check that the destination directory exists...
  */

  strcpy(buffer, dst);
  if ((slash = strrchr(buffer, '/')) != NULL)
    *slash = '\0';

  if (access(buffer, F_OK))
    make_directory(buffer, 0755, owner, group);

 /*
  * Open files...
  */

  unlink(dst);

  if ((dstfile = fopen(dst, "wb")) == NULL)
  {
    fprintf(stderr, "epm: Unable to create \"%s\" -\n     %s\n", dst,
            strerror(errno));
    return (-1);
  }

  if ((srcfile = fopen(src, "rb")) == NULL)
  {
    fclose(dstfile);
    unlink(dst);

    fprintf(stderr, "epm: Unable to open \"%s\" -\n     %s\n", src,
            strerror(errno));
    return (-1);
  }

 /*
  * Copy from src to dst...
  */

  while ((bytes = fread(buffer, 1, sizeof(buffer), srcfile)) > 0)
    if (fwrite(buffer, 1, bytes, dstfile) < bytes)
    {
      fprintf(stderr, "epm: Unable to write to \"%s\" -\n     %s\n", dst,
              strerror(errno));

      fclose(srcfile);
      fclose(dstfile);
      unlink(dst);

      return (-1);
    }

 /*
  * Close files, change permissions, and return...
  */

  fclose(srcfile);
  fclose(dstfile);

  chmod(dst, mode);
  chown(dst, owner, group);

  return (0);
}


/*
 * 'make_directory()' - Make a directory.
 */

int					/* O - 0 = success, -1 = error */
make_directory(const char *directory,	/* I - Directory */
               mode_t     mode,		/* I - Permissions */
	       uid_t      owner,	/* I - Owner ID */
	       gid_t      group)	/* I - Group ID */
{
  char	buffer[8192],			/* Filename buffer */
	*bufptr;			/* Pointer into buffer */


  for (bufptr = buffer; *directory;)
  {
    if (*directory == '/' && bufptr > buffer)
    {
      *bufptr = '\0';

      if (access(buffer, F_OK))
      {
	mkdir(buffer, 0777);
	chmod(buffer, mode | 0700);
	chown(buffer, owner, group);
      }
    }

    *bufptr++ = *directory++;
  }

  *bufptr = '\0';

  if (access(buffer, F_OK))
  {
    mkdir(buffer, 0777);
    chmod(buffer, mode | 0700);
    chown(buffer, owner, group);
  }

  return (0);
}


/*
 * 'make_link()' - Make a symbolic link.
 */

int					/* O - 0 = success, -1 = error */
make_link(const char *dst,		/* I - Destination file */
          const char *src)		/* I - Link */
{
  char	buffer[8192],			/* Copy buffer */
	*slash;				/* Pointer to trailing slash */


 /*
  * Check that the destination directory exists...
  */

  strcpy(buffer, dst);
  if ((slash = strrchr(buffer, '/')) != NULL)
    *slash = '\0';

  if (access(buffer, F_OK))
    make_directory(buffer, 0755, 0, 0);

 /* 
  * Make the symlink...
  */

  return (symlink(src, dst));
}


/*
 * 'strip_execs()' - Strip symbols from executable files in the distribution.
 */

void
strip_execs(dist_t *dist)		/* I - Distribution to strip... */
{
  int		i;			/* Looping var */
  file_t	*file;			/* Software file */
  FILE		*fp;			/* File pointer */
  char		header[4];		/* File header... */


 /*
  * Loop through the distribution files and strip any executable
  * files.
  */

  for (i = dist->num_files, file = dist->files; i > 0; i --, file ++)
    if (tolower(file->type) == 'f' && (file->mode & 0111) &&
        strstr(file->options, "nostrip()") == NULL)
    {
     /*
      * OK, this file has executable permissions; see if it is a
      * script...
      */

      if ((fp = fopen(file->src, "rb")) != NULL)
      {
       /*
        * Read the first 3 bytes of the file...
	*/

        if (fread(header, 1, sizeof(header) - 1, fp) == 0)
	  continue;

	header[sizeof(header) - 1] = '\0';

	fclose(fp);

       /*
        * Check for "#!/" or "#" + whitespace at the beginning of the file...
	*/

        if (!strncmp(header, "#!/", 3) ||
	    (header[0] == '#' && isspace(header[1] & 255)))
	  continue;
      }
      else
      {
       /*
        * File could not be opened; error out...
	*/

        fprintf(stderr, "epm: Unable to open file \"%s\" for destination "
	                "\"%s\" -\n     %s\n",
	        file->src, file->dst, strerror(errno));

        exit(1);
      }

     /*
      * Strip executables...
      */

      run_command(NULL, EPM_STRIP " %s", file->src);
    }
}


/*
 * End of "$Id: file.c,v 1.19 2005/01/11 21:36:57 mike Exp $".
 */
