/*
 * "$Id: tar.c,v 1.3 1999/12/07 13:57:05 mike Exp $"
 *
 *   TAR file functions for the ESP Package Manager (EPM).
 *
 *   Copyright 1999 by Easy Software Products.
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
 *   tar_close()  - Close a tar file, padding as needed.
 *   tar_file()   - Write the contents of a file...
 *   tar_header() - Write a TAR header for the specified file...
 *   tar_open()   - Open a TAR file for writing.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"


/*
 * 'tar_close()' - Close a tar file, padding as needed.
 */

int			/* O - -1 on error, 0 on success */
tar_close(tarf_t *fp)	/* I - File to write to */
{
  int	blocks;		/* Number of blocks to write */
  int	status;		/* Return status */
  char	padding[TAR_BLOCKS * TAR_BLOCK];
			/* Padding for tar blocks */


 /*
  * Zero the padding record...
  */

  memset(padding, 0, sizeof(padding));

 /*
  * Write a single 0 block to signal the end of the archive...
  */

  if (fwrite(padding, TAR_BLOCK, 1, fp->file) < 1)
    return (-1);

  fp->blocks ++;

 /*
  * Pad the tar files to TAR_BLOCKS blocks...  This is bullshit of course,
  * but old versions of tar need it...
  */

  status = 0;

  if ((blocks = fp->blocks % TAR_BLOCKS) > 0)
  {
    blocks = TAR_BLOCKS - blocks;

    if (fwrite(padding, TAR_BLOCK, blocks, fp->file) < blocks)
      status = -1;
  }

 /*
  * Close the file and free memory...
  */

  if (fp->compressed)
  {
    if (pclose(fp->file))
      status = -1;
  }
  else if (fclose(fp->file))
    status = -1;

  free(fp);

  return (status);
}


/*
 * 'tar_file()' - Write the contents of a file...
 */

int				/* O - 0 on success, -1 on error */
tar_file(tarf_t     *fp,	/* I - Tar file to write to */
         const char *filename)	/* I - File to write */
{
  FILE	*file;			/* File to write */
  int	nbytes,			/* Number of bytes read */
	tbytes,			/* Total bytes read/written */
	fill;			/* Number of fill bytes needed */
  char	buffer[8192];		/* Copy buffer */


 /*
  * Try opening the file...
  */

  if ((file = fopen(filename, "rb")) == NULL)
    return (-1);

 /*
  * Copy the file to the tar file...
  */

  tbytes = 0;

  while ((nbytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
  {
   /*
    * Zero fill the file to a 512 byte record as needed.
    */

    if (nbytes < sizeof(buffer))
    {
      fill = TAR_BLOCK - (nbytes & (TAR_BLOCK - 1));

      if (fill < TAR_BLOCK)
      {
        memset(buffer + nbytes, 0, fill);
        nbytes += fill;
      }
    }

   /*
    * Write the buffer to the file...
    */

    if (fwrite(buffer, 1, nbytes, fp->file) < nbytes)
    {
      fclose(file);
      return (-1);
    }

    tbytes     += nbytes;
    fp->blocks += nbytes / TAR_BLOCK;
  }

 /*
  * Close the file and return...
  */

  fclose(file);
  return (0);
}


/*
 * 'tar_header()' - Write a TAR header for the specified file...
 */

int				/* O - 0 on success, -1 on error */
tar_header(tarf_t     *fp,	/* I - Tar file to write to */
           char       type,	/* I - File type */
	   int        mode,	/* I - File permissions */
	   int        size,	/* I - File size */
           time_t     mtime,	/* I - File modification time */
	   const char *user,	/* I - File owner */
	   const char *group,	/* I - File group */
	   const char *pathname,/* I - File name */
	   const char *linkname)/* I - File link name (for links only) */
{
  tar_t		record;		/* TAR header record */
  int		i,		/* Looping var... */
		sum;		/* Checksum */
  unsigned char	*sumptr;	/* Pointer into header record */
  struct passwd	*pwd;		/* Pointer to user record */
  struct group	*grp;		/* Pointer to group record */


 /*
  * Find the username and groupname IDs...
  */

  pwd = getpwnam(user);
  grp = getgrnam(group);

  endpwent();
  endgrent();

 /*
  * Format the header...
  */

  memset(&record, 0, sizeof(record));

  strcpy(record.header.pathname, pathname);
  sprintf(record.header.mode, "%-6o ", mode);
  sprintf(record.header.uid, "%o ", pwd == NULL ? 0 : pwd->pw_uid);
  sprintf(record.header.gid, "%o ", grp == NULL ? 0 : grp->gr_gid);
  sprintf(record.header.size, "%011o", size);
  sprintf(record.header.mtime, "%011o", mtime);
  memset(&(record.header.chksum), ' ', sizeof(record.header.chksum));
  record.header.linkflag = type;
  if (type == TAR_SYMLINK)
    strcpy(record.header.linkname, linkname);
  strcpy(record.header.magic, TAR_MAGIC);
  strcpy(record.header.uname, user);
  strcpy(record.header.gname, group);

 /*
  * Compute the checksum of the header...
  */

  for (i = sizeof(record), sumptr = record.all, sum = 0; i > 0; i --)
    sum += *sumptr++;

 /*
  * Put the correct checksum in place and write the header...
  */

  sprintf(record.header.chksum, "%6o", sum);

  if (fwrite(&record, 1, sizeof(record), fp->file) < sizeof(record))
    return (-1);

  fp->blocks ++;
  return (0);
}


/*
 * 'tar_open()' - Open a TAR file for writing.
 */

tarf_t *			/* O - New tar file */
tar_open(const char *filename,	/* I - File to create */
         int        compress)	/* I - Compress with gzip? */
{
  tarf_t	*fp;		/* New tar file */
  char		command[1024];	/* Compression command */


 /*
  * Allocate memory for the tar file state...
  */

  if ((fp = calloc(sizeof(tarf_t), 1)) == NULL)
    return (NULL);

 /*
  * Open the output file or pipe into gzip for compressed output...
  */

  if (compress)
  {
    sprintf(command, EPM_GZIP " > %s", filename);
    fp->file = popen(command, "w");
  }
  else
    fp->file = fopen(filename, "wb");

 /*
  * If we couldn't open the output file, abort...
  */

  if (fp->file == NULL)
  {
    free(fp);
    return (NULL);
  }

 /*
  * Save the compression state and return...
  */

  fp->compressed = compress;

  return (fp);
}


/*
 * End of "$Id: tar.c,v 1.3 1999/12/07 13:57:05 mike Exp $".
 */
