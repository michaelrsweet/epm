/*
 * "$Id: dist.c,v 1.42 2001/10/26 16:14:31 mike Exp $"
 *
 *   Distribution functions for the ESP Package Manager (EPM).
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
 *   add_command()     - Add a command to the distribution...
 *   add_depend()      - Add a dependency to the distribution...
 *   add_file()        - Add a file to the distribution.
 *   free_dist()       - Free memory used by a distribution.
 *   get_platform()    - Get the operating system information...
 *   read_dist()       - Read a software distribution.
 *   sort_dist_files() - Sort the files in the distribution.
 *   add_string()      - Add a command to an array of commands...
 *   compare_files()   - Compare the destination filenames.
 *   free_strings()    - Free memory used by the array of strings.
 *   expand_name()     - Expand a filename with environment variables.
 *   get_file()        - Read a file into a string...
 *   get_inline()      - Read inline lines into a string...
 *   get_line()        - Get a line from a file, filtering for uname lines...
 *   get_vernumber()   - Convert a version string to a number...
 *   patmatch()        - Pattern matching...
 */

/*
 * Include necessary headers...
 */

#include "epm.h"
#include <pwd.h>


/*
 * Some versions of Solaris don't define gethostname()...
 */

#ifdef __sun
extern int	gethostname(char *, int);
#endif /* __sun */


/*
 * Local functions...
 */

static int	add_string(int num_strings, char ***strings, FILE *fp,
		           char *string);
static int	compare_files(const file_t *f0, const file_t *f1);
static void	expand_name(char *buffer, char *name);
static void	free_strings(int num_strings, char **strings);
static char	*get_file(const char *filename, char *buffer, int size);
static char	*get_inline(const char *term, FILE *fp, char *buffer, int size);
static char	*get_line(char *buffer, int size, FILE *fp,
		          struct utsname *platform, const char *format,
			  int *skip);
static int	get_vernumber(const char *version);
static int	patmatch(const char *, const char *);


/*
 * Conditional "skip" bits...
 */

#define SKIP_SYSTEM	1	/* Not the right system */
#define SKIP_FORMAT	2	/* Not the right format */
#define SKIP_IF		4	/* Set if the current #if was not satisfied */
#define SKIP_IFACTIVE	8	/* Set if we're in an #if */
#define SKIP_IFSAT	16	/* Set if an #if statement has been satisfied */
#define SKIP_MASK	7	/* Bits to look at */


/*
 * 'add_command()' - Add a command to the distribution...
 */

void
add_command(dist_t     *dist,		/* I - Distribution */
            FILE       *fp,		/* I - Distribution file */
            char       type,		/* I - Command type */
	    const char *command)	/* I - Command string */
{
  command_t	*temp;			/* New command */
  char		buf[16384];		/* File import buffer */


  if (strncmp(command, "<<", 2) == 0)
  {
    for (command += 2; isspace(*command); command ++);

    command = get_inline(command, fp, buf, sizeof(buf));
  }
  else if (command[0] == '<' && command[1])
  {
    for (command ++; isspace(*command); command ++);

    command = get_file(command, buf, sizeof(buf));
  }

  if (command == NULL)
    return;

  if (dist->num_commands == 0)
    temp = malloc(sizeof(command_t));
  else
    temp = realloc(dist->commands, (dist->num_commands + 1) * sizeof(command_t));

  if (temp == NULL)
  {
    perror("epm: Out of memory allocating a command");
    return;
  }

  dist->commands = temp;
  temp           += dist->num_commands;
  temp->type     = type;
  temp->command  = strdup(command);

  if (temp->command == NULL)
  {
    perror("epm: Out of memory duplicating a command string");
    return;
  }

  dist->num_commands ++;
}


/*
 * 'add_depend()' - Add a dependency to the distribution...
 */

void
add_depend(dist_t     *dist,		/* I - Distribution */
           char       type,		/* I - Type of dependency */
	   const char *line)		/* I - Line from file */
{
  int		i;			/* Looping var */
  depend_t	*temp;			/* New dependency */
  char		*ptr;			/* Pointer into string */
  const char	*lineptr;		/* Temporary pointer into line */


 /*
  * Allocate memory for the dependency...
  */

  if (dist->num_depends == 0)
    temp = malloc(sizeof(depend_t));
  else
    temp = realloc(dist->depends, (dist->num_depends + 1) * sizeof(depend_t));

  if (temp == NULL)
  {
    perror("epm: Out of memory allocating a dependency");
    return;
  }

  dist->depends = temp;
  temp          += dist->num_depends;
  dist->num_depends ++;

 /*
  * Initialize the dependency record...
  */

  memset(temp, 0, sizeof(depend_t));

  temp->type = type;

 /*
  * Get the product name string...
  */

  for (ptr = temp->product; *line && !isspace((int)*line); line ++)
    if (ptr < (temp->product + sizeof(temp->product) - 1))
      *ptr++ = *line;

  while (isspace((int)*line))
    line ++;

 /*
  * Get the version strings, if any...
  */

  for (i = 0; *line && i < 2; i ++)
  {
   /*
    * Handle <version, >version, etc.
    */

    if (!isdigit((int)*line))
    {
      if (*line == '<' && i == 0)
      {
        strcpy(temp->version[0], "0.0");
	i ++;
      }

      while (!isdigit((int)*line) && *line)
        line ++;
    }

    if (!*line)
      break;

   /*
    * Grab the version string...
    */

    for (ptr = temp->version[i]; *line && !isspace((int)*line); line ++)
      if (ptr < (temp->version[i] + sizeof(temp->version[i]) - 1))
	*ptr++ = *line;

    while (isspace((int)*line))
      line ++;

   /*
    * Get the version number, if any...
    */

    for (lineptr = line; isdigit((int)*lineptr); lineptr ++);

    if (!*line || (!isspace((int)*lineptr) && *lineptr))
    {
     /*
      * No version number specified, or the next number is a version
      * string...
      */

      temp->vernumber[i] = get_vernumber(temp->version[i]);
    }
    else
    {
     /*
      * Grab the version number directly from the line...
      */

      temp->vernumber[i] = atoi(line);

      for (line = lineptr; isspace((int)*line); line ++);
    }
  }

 /*
  * Handle assigning default values based on the number of version numbers...
  */

  switch (i)
  {
    case 0 :
        strcpy(temp->version[0], "0.0");
	/* fall through to set max version number */
    case 1 :
        strcpy(temp->version[1], "999.99.99p99");
	temp->vernumber[1] = INT_MAX;
	break;
  }
}


/*
 * 'add_file()' - Add a file to the distribution.
 */

file_t *		/* O - New file */
add_file(dist_t *dist)	/* I - Distribution */
{
  file_t	*file;	/* New file */


  if (dist->num_files == 0)
    dist->files = (file_t *)malloc(sizeof(file_t));
  else
    dist->files = (file_t *)realloc(dist->files, sizeof(file_t) *
					         (dist->num_files + 1));

  file = dist->files + dist->num_files;
  dist->num_files ++;

  return (file);
}


/*
 * 'free_dist()' - Free memory used by a distribution.
 */

void
free_dist(dist_t *dist)		/* I - Distribution to free */
{
  int	i;			/* Looping var */


  if (dist->num_files > 0)
    free(dist->files);

  free_strings(dist->num_descriptions, dist->descriptions);

  for (i = 0; i < dist->num_commands; i ++)
    free(dist->commands[i].command);

  if (dist->num_commands)
    free(dist->commands);

  if (dist->num_depends)
    free(dist->depends);

  free(dist);
}


/*
 * 'get_platform()' - Get the operating system information...
 */

void
get_platform(struct utsname *platform)	/* O - Platform info */
{
  char	*temp;				/* Temporary pointer */


 /*
  * Get the system identification information...
  */

  uname(platform);

 /*
  * Adjust the CPU type accordingly...
  */

#ifdef __sgi
  strcpy(platform->machine, "mips");
#elif defined(__hpux)
  strcpy(platform->machine, "hppa");
#elif defined(_AIX) || defined(__APPLE__)
  strcpy(platform->machine, "powerpc");
#else
  for (temp = platform->machine; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

  if (strstr(platform->machine, "86") != NULL)
    strcpy(platform->machine, "intel");
  else if (strncmp(platform->machine, "sun", 3) == 0)
    strcpy(platform->machine, "sparc");
#endif /* __sgi */

#ifdef _AIX
 /*
  * AIX stores the major and minor version numbers separately;
  * combine them...
  */

  sprintf(platform->release, "%d.%d", atoi(platform->version),
          atoi(platform->release));
#else
 /*
  * Remove any extra junk from the release number - we just want the
  * major and minor numbers...
  */

  while (!isdigit((int)platform->release[0]) && platform->release[0])
    strcpy(platform->release, platform->release + 1);

  if (platform->release[0] == '.')
    strcpy(platform->release, platform->release + 1);

  for (temp = platform->release; *temp && isdigit((int)*temp); temp ++);

  if (*temp == '.')
    for (temp ++; *temp && isdigit((int)*temp); temp ++);

  *temp = '\0';
#endif /* _AIX */

 /*
  * Convert the operating system name to lowercase, and strip out
  * hyphens and underscores...
  */

  for (temp = platform->sysname; *temp != '\0'; temp ++)
    if (*temp == '-' || *temp == '_')
    {
      strcpy(temp, temp + 1);
      temp --;
    }
    else
      *temp = tolower(*temp);

 /*
  * SunOS 5.x is really Solaris 2.x, and OSF1 is really Digital UNIX a.k.a.
  * Compaq Tru64 UNIX...
  */

  if (strcmp(platform->sysname, "sunos") == 0 &&
      platform->release[0] >= '5')
  {
    strcpy(platform->sysname, "solaris");
    platform->release[0] -= 3;
  }
  else if (strcmp(platform->sysname, "osf1") == 0)
    strcpy(platform->sysname, "tru64"); /* AKA Digital UNIX */
  else if (strcmp(platform->sysname, "irix64") == 0)
    strcpy(platform->sysname, "irix"); /* IRIX */

#ifdef DEBUG
  printf("sysname = %s\n", platform->sysname);
  printf("release = %s\n", platform->release);
  printf("machine = %s\n", platform->machine);
#endif /* DEBUG */
}


/*
 * 'read_dist()' - Read a software distribution.
 */

dist_t *				/* O - New distribution */
read_dist(const char     *filename,	/* I - Main distribution list file */
          struct utsname *platform,	/* I - Platform information */
          const char     *format)	/* I - Format of distribution */
{
  FILE		*listfiles[10];	/* File lists */
  int		listlevel;	/* Level in file list */
  char		line[2048],	/* Expanded line from list file */
		buf[1024],	/* Original line from list file */
		type,		/* File type */
		dst[256],	/* Destination path */
		src[256],	/* Source path */
		pattern[256],	/* Pattern for source files */
		user[32],	/* User */
		group[32],	/* Group */
		*temp;		/* Temporary pointer */
  int		mode,		/* File permissions */
		skip;		/* 1 = skip files, 0 = archive files */
  dist_t	*dist;		/* Distribution data */
  file_t	*file;		/* Distribution file */
  struct stat	fileinfo;	/* File information */
  DIR		*dir;		/* Directory */
  DIRENT	*dent;		/* Directory entry */
  struct passwd	*pwd;		/* Password entry */


 /*
  * Create a new, blank distribution...
  */

  dist = (dist_t *)calloc(sizeof(dist_t), 1);

 /*
  * Open the main list file...
  */

  if ((listfiles[0] = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "epm: Unable to open list file \"%s\" -\n     %s\n",
            filename, strerror(errno));
    return (NULL);
  }

 /*
  * Find any product descriptions, etc. in the list file...
  */

  if (Verbosity)
    puts("Searching for product information...");

  skip      = 0;
  listlevel = 0;

  do
  {
    while (get_line(buf, sizeof(buf), listfiles[listlevel], platform, format,
                    &skip) != NULL)
    {
     /*
      * Do variable substitution...
      */

      line[0] = buf[0]; /* Don't expand initial $ */
      expand_name(line + 1, buf + 1);

     /*
      * Check line for config stuff...
      */

      if (line[0] == '%')
      {
       /*
        * Find whitespace...
	*/

        for (temp = line; !isspace((int)*temp) && *temp; temp ++);
	for (; isspace((int)*temp); *temp++ = '\0');

       /*
        * Process directive...
        */

	if (strcmp(line, "%include") == 0)
	{
	  listlevel ++;

	  if ((listfiles[listlevel] = fopen(temp, "r")) == NULL)
	  {
	    fprintf(stderr, "epm: Unable to include \"%s\" -\n     %s\n", temp,
	            strerror(errno));
	    listlevel --;
	  }
	}
	else if (strcmp(line, "%description") == 0)
	  dist->num_descriptions =
	      add_string(dist->num_descriptions, &(dist->descriptions), 
	                 listfiles[listlevel], temp);
	else if (strcmp(line, "%preinstall") == 0)
          add_command(dist, listfiles[listlevel], COMMAND_PRE_INSTALL, temp);
	else if (strcmp(line, "%install") == 0 ||
	         strcmp(line, "%postinstall") == 0)
          add_command(dist, listfiles[listlevel], COMMAND_POST_INSTALL, temp);
	else if (strcmp(line, "%remove") == 0 ||
	         strcmp(line, "%preremove") == 0)
          add_command(dist, listfiles[listlevel], COMMAND_PRE_REMOVE, temp);
	else if (strcmp(line, "%postremove") == 0)
          add_command(dist, listfiles[listlevel], COMMAND_POST_REMOVE, temp);
	else if (strcmp(line, "%prepatch") == 0)
          add_command(dist, listfiles[listlevel], COMMAND_PRE_PATCH, temp);
	else if (strcmp(line, "%patch") == 0 ||
	         strcmp(line, "%postpatch") == 0)
          add_command(dist, listfiles[listlevel], COMMAND_POST_PATCH, temp);
        else if (strcmp(line, "%product") == 0)
	{
          if (!dist->product[0])
            strcpy(dist->product, temp);
	  else
	    fputs("epm: Ignoring %product line in list file.\n", stderr);
	}
	else if (strcmp(line, "%copyright") == 0)
	{
          if (!dist->copyright[0])
            strcpy(dist->copyright, temp);
	  else
	    fputs("epm: Ignoring %copyright line in list file.\n", stderr);
	}
	else if (strcmp(line, "%vendor") == 0)
	{
          if (!dist->vendor[0])
            strcpy(dist->vendor, temp);
	  else
	    fputs("epm: Ignoring %vendor line in list file.\n", stderr);
	}
	else if (strcmp(line, "%packager") == 0)
	{
          if (!dist->packager[0])
            strcpy(dist->packager, temp);
	  else
	    fputs("epm: Ignoring %packager line in list file.\n", stderr);
	}
	else if (strcmp(line, "%license") == 0)
	{
          if (!dist->license[0])
            strcpy(dist->license, temp);
	  else
	    fputs("epm: Ignoring %license line in list file.\n", stderr);
	}
	else if (strcmp(line, "%readme") == 0)
	{
          if (!dist->readme[0])
            strcpy(dist->readme, temp);
	  else
	    fputs("epm: Ignoring %readme line in list file.\n", stderr);
	}
	else if (strcmp(line, "%version") == 0)
	{
          if (!dist->version[0])
	  {
            strcpy(dist->version, temp);
	    if ((temp = strchr(dist->version, ' ')) != NULL)
	    {
	      *temp++ = '\0';
	      dist->vernumber = atoi(temp);
	    }
	    else
	      dist->vernumber = get_vernumber(dist->version);

            if ((temp = strrchr(dist->version, '-')) != NULL)
	    {
	      *temp++ = '\0';
	      dist->relnumber = atoi(temp);
	    }
	  }
	}
	else if (strcmp(line, "%release") == 0 && dist->relnumber == 0)
	{
	  dist->relnumber = atoi(temp);
	  dist->vernumber += dist->relnumber;
	}
	else if (strcmp(line, "%incompat") == 0)
	  add_depend(dist, DEPEND_INCOMPAT, temp);
	else if (strcmp(line, "%provides") == 0)
	  add_depend(dist, DEPEND_PROVIDES, temp);
	else if (strcmp(line, "%replaces") == 0)
	  add_depend(dist, DEPEND_REPLACES, temp);
	else if (strcmp(line, "%requires") == 0)
	  add_depend(dist, DEPEND_REQUIRES, temp);
	else
	{
	  fprintf(stderr, "epm: Unknown directive \"%s\" ignored!\n", line);
	  fprintf(stderr, "     %s %s\n", line, temp);
	}
      }
      else if (line[0] == '$')
      {
       /*
        * Define a variable...
	*/

        if (line[1] == '{' && (temp = strchr(line + 2, '}')) != NULL)
	{
	 /*
	  * Remove {} from name...
	  */

	  strcpy(temp, temp + 1);
          strcpy(line + 1, line + 2);
        }
	else if (line[1] == '(' && (temp = strchr(line + 2, ')')) != NULL)
	{
	 /*
	  * Remove () from name...
	  */

	  strcpy(temp, temp + 1);
          strcpy(line + 1, line + 2);
        }

        if ((temp = strchr(line + 1, '=')) != NULL)
	{
	 /*
	  * Only define the variable if it is not in the environment
	  * or on the command-line.
	  */

	  *temp = '\0';

	  if (getenv(line + 1) == NULL)
	  {
	    *temp = '=';
            if ((temp = strdup(line + 1)) != NULL)
	      putenv(temp);
	  }
	}
      }
      else if (sscanf(line, "%c%o%15s%15s%254s%254s", &type, &mode, user, group,
        	      dst, src) < 5)
	fprintf(stderr, "epm: Bad line - %s\n", line);
      else
      {
	if (tolower(type) == 'd' || type == 'R')
	  src[0] = '\0';

#ifdef __osf__ /* Remap group "sys" to "system" */
        if (strcmp(group, "sys") == 0)
	  strcpy(group, "system");
#elif defined(__linux) /* Remap group "sys" to "root" */
        if (strcmp(group, "sys") == 0)
	  strcpy(group, "root");
#endif /* __osf__ */

        if ((temp = strrchr(src, '/')) == NULL)
	  temp = src;
	else
	  temp ++;

        for (; *temp; temp ++)
	  if (strchr("*?[", *temp))
	    break;

        if (*temp)
	{
	 /*
	  * Add using wildcards...
	  */

          if ((temp = strrchr(src, '/')) == NULL)
	    temp = src;
	  else
	    *temp++ = '\0';

	  strncpy(pattern, temp, sizeof(pattern) - 1);
	  pattern[sizeof(pattern) - 1] = '\0';

          if (dst[strlen(dst) - 1] != '/')
	    strncat(dst, "/", sizeof(dst) - 1);

          if (temp == src)
	    dir = opendir(".");
	  else
	    dir = opendir(src);

          if (dir == NULL)
	    fprintf(stderr, "epm: Unable to open directory \"%s\" - %s\n",
	            src, strerror(errno));
          else
	  {
	   /*
	    * Make sure we have a directory separator...
	    */

	    if (temp > src)
	      temp[-1] = '/';

	    while ((dent = readdir(dir)) != NULL)
	    {
	      strcpy(temp, dent->d_name);
	      if (stat(src, &fileinfo))
	        continue; /* Skip files we can't read */

              if (S_ISDIR(fileinfo.st_mode))
	        continue; /* Skip directories */

              if (!patmatch(dent->d_name, pattern))
	        continue;

              file = add_file(dist);

              file->type = type;
	      file->mode = mode;
              strcpy(file->src, src);
	      strcpy(file->dst, dst);
	      strncat(file->dst, dent->d_name, sizeof(file->dst) - 1);
	      strcpy(file->user, user);
	      strcpy(file->group, group);
	    }

            closedir(dir);
	  }
	}
	else
	{
         /*
	  * Add single file...
	  */

          file = add_file(dist);

          file->type = type;
	  file->mode = mode;
          strcpy(file->src, src);
	  strcpy(file->dst, dst);
	  strcpy(file->user, user);
	  strcpy(file->group, group);
	}
      }
    }

    fclose(listfiles[listlevel]);
    listlevel --;
  }
  while (listlevel >= 0);

  if (!dist->packager[0])
  {
   /*
    * Assign a default packager name...
    */

    gethostname(buf, sizeof(buf));

    setpwent();
    if ((pwd = getpwuid(getuid())) != NULL)
      snprintf(dist->packager, sizeof(dist->packager), "%s@%s", pwd->pw_name,
               buf);
    else
      snprintf(dist->packager, sizeof(dist->packager), "unknown@%s", buf);
  }

  sort_dist_files(dist);

  return (dist);
}


/*
 * 'sort_dist_files()' - Sort the files in the distribution.
 */

void
sort_dist_files(dist_t *dist)	/* I - Distribution to sort */
{
  int		i;		/* Looping var */
  file_t	*file;		/* File in distribution */


 /*
  * Sort the files...
  */

  if (dist->num_files > 1)
    qsort(dist->files, dist->num_files, sizeof(file_t),
          (int (*)(const void *, const void *))compare_files);

 /*
  * Remove duplicates...
  */

  for (i = dist->num_files - 1, file = dist->files; i > 0; i --, file ++)
    if (strcmp(file[0].dst, file[1].dst) == 0)
    {
      memcpy(file, file + 1, i * sizeof(file_t));
      dist->num_files --;
      file --;
    }
}


/*
 * 'add_string()' - Add a command to an array of commands...
 */

static int			/* O - Number of strings */
add_string(int  num_strings,	/* I - Number of strings */
           char ***strings,	/* O - Pointer to string pointer array */
           FILE *fp,		/* I - File to read from for inline text */
           char *string)	/* I - String to add */
{
  char		**temp,		/* Temporary pointer to string array */
		buf[16384];	/* File import buffer */


  if (strncmp(string, "<<", 2) == 0)
  {
    for (string += 2; isspace(*string); string ++);

    string = get_inline(string, fp, buf, sizeof(buf));
  }
  else if (string[0] == '<' && string[1])
  {
    for (string ++; isspace(*string); string ++);

    string = get_file(string, buf, sizeof(buf));
  }

  if (string == NULL)
    return (num_strings);

  if (num_strings == 0)
    temp = (char **)malloc(sizeof(char *));
  else
    temp = (char **)realloc(*strings, (num_strings + 1) * sizeof(char *));

  if (temp == NULL)
  {
    perror("epm: Out of memory adding string");
    return (num_strings);
  }

  *strings = temp;
  temp     += num_strings;

  if ((*temp = strdup(string)) == NULL)
  {
    perror("epm: Out of memory duplicating string");
    return (num_strings);
  }

  return (num_strings + 1);
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
 * 'expand_name()' - Expand a filename with environment variables.
 */

static void
expand_name(char *buffer,	/* O - Output string */
            char *name)		/* I - Input string */
{
  char	var[255],		/* Environment variable name */
	*varptr;		/* Current position in name */


  while (*name != '\0')
  {
    if (*name == '$')
    {
      name ++;
      if (*name == '$')
      {
       /*
        * Insert a lone $...
	*/

	*buffer++ = *name++;
	continue;
      }
      else if (*name == '{')
      {
       /*
        * Bracketed variable name...
	*/

	for (varptr = var, name ++; *name != '}' && *name != '\0';)
          *varptr++ = *name++;

        if (*name == '}')
	  name ++;
      }
      else
      {
       /*
        * Unbracketed variable name...
	*/

	for (varptr = var; strchr("/ \t-", *name) == NULL && *name != '\0';)
          *varptr++ = *name++;
      }

      *varptr = '\0';

      if ((varptr = getenv(var)) != NULL)
      {
        strcpy(buffer, varptr);
	buffer += strlen(buffer);
      }
    }
    else
      *buffer++ = *name++;
  }

  *buffer = '\0';
}


/*
 * 'free_strings()' - Free memory used by the array of strings.
 */

static void
free_strings(int  num_strings,	/* I - Number of strings */
             char **strings)	/* I - Pointer to string pointers */
{
  int	i;			/* Looping var */


  for (i = 0; i < num_strings; i ++)
    free(strings[i]);

  if (num_strings)
    free(strings);
}


/*
 * 'get_file()' - Read a file into a string...
 */

static char *			/* O  - Pointer to string or NULL on EOF */
get_file(const char *filename,	/* I  - File to read from */
         char       *buffer,	/* IO - String buffer */
	 int        size)	/* I  - Size of string buffer */
{
  FILE		*fp;		/* File buffer */
  struct stat	info;		/* File information */


  if (stat(filename, &info))
  {
    fprintf(stderr, "epm: Unable to stat \"%s\" - %s\n", filename,
            strerror(errno));
    return (NULL);
  }

  if (info.st_size > (size - 1))
  {
    fprintf(stderr, "epm: File \"%s\" is too large (%d bytes) for buffer (%d bytes)\n",
            filename, (int)info.st_size, size - 1);
    return (NULL);
  }

  if ((fp = fopen(filename, "r")) == NULL)
  {
    fprintf(stderr, "epm: Unable to open \"%s\" - %s\n", filename,
            strerror(errno));
    return (NULL);
  }

  if ((fread(buffer, 1, info.st_size, fp)) < info.st_size)
  {
    fprintf(stderr, "epm: Unable to read \"%s\" - %s\n", filename,
            strerror(errno));
    fclose(fp);
    return (NULL);
  }

  fclose(fp);

  if (buffer[info.st_size - 1] == '\n')
    buffer[info.st_size - 1] = '\0';
  else
    buffer[info.st_size] = '\0';

  return (buffer);
}


/*
 * 'get_inline()' - Read inline lines into a string...
 */

static char *			/* O  - Pointer to string or NULL on EOF */
get_inline(const char *term,	/* I  - Termination string */
           FILE       *fp,	/* I  - File to read from */
           char       *buffer,	/* IO - String buffer */
	   int        size)	/* I  - Size of string buffer */
{
  char	*bufptr;		/* Pointer into buffer */
  int	termlen;		/* Length of termination string */
  int	linelen;		/* Length of line */


  bufptr  = buffer;
  termlen = strlen(term);

  if (termlen == 0)
    return (NULL);

  while (fgets(bufptr, size, fp) != NULL)
  {
    if (strncmp(bufptr, term, termlen) == 0 && bufptr[termlen] == '\n')
    {
      *bufptr = '\0';
      break;
    }

    linelen = strlen(bufptr);
    size    -= linelen;
    bufptr  += linelen;

    if (size < 2)
    {
      fputs("epm: Inline script too long!\n", stderr);
      break;
    }
  }

  if (bufptr > buffer)
  {
    bufptr --;
    if (*bufptr == '\n')
      *bufptr = '\0';

    return (buffer);
  }
  else
    return (NULL);
}


/*
 * 'get_line()' - Get a line from a file, filtering for uname lines...
 */

static char *				/* O - String read or NULL at EOF */
get_line(char           *buffer,	/* I - Buffer to read into */
         int            size,		/* I - Size of buffer */
	 FILE           *fp,		/* I - File to read from */
	 struct utsname *platform,	/* I - Platform information */
         const char     *format,	/* I - Distribution format */
	 int            *skip)		/* IO - Skip lines? */
{
  int		op,			/* Operation (0 = OR, 1 = AND) */
		namelen,		/* Length of system name + version */
		len,			/* Length of string */
		match;			/* 1 = match, 0 = not */
  char		*ptr,			/* Pointer into value */
		*bufptr,		/* Pointer into buffer */
		namever[255],		/* Name + version */
		value[255];		/* Value string */
  const char	*var;			/* Variable value */


  while (fgets(buffer, size, fp) != NULL)
  {
   /*
    * Skip comment and blank lines...
    */

    if (buffer[0] == '#' || buffer[0] == '\n')
      continue;

   /*
    * See if this is a %system, %format, or conditional line...
    */

    if (strncmp(buffer, "%system ", 8) == 0)
    {
     /*
      * Yes, do filtering based on the OS (+version)...
      */

      *skip &= ~SKIP_SYSTEM;

      if (strcmp(buffer + 8, "all\n") != 0)
      {
	namelen = strlen(platform->sysname);
        bufptr  = buffer + 8;
	snprintf(namever, sizeof(namever), "%s-%s", platform->sysname,
	         platform->release);

	while (isspace((int)*bufptr))
	  bufptr ++;

        if (*bufptr != '!')
	  *skip |= SKIP_SYSTEM;

        while (*bufptr)
	{
	 /*
          * Skip leading whitespace...
	  */

	  while (isspace((int)*bufptr))
	    bufptr ++;

          if (!*bufptr)
	    break;

          if (*bufptr == '!')
	  {
	    op = 1;
	    bufptr ++;
	  }
	  else
	    op = 0;

	  for (ptr = value;
	       *bufptr && !isspace((int)*bufptr) &&
	           ptr < (value + sizeof(value) - 1);
	       *ptr++ = *bufptr++);

	  *ptr = '\0';

          if (strncmp(value, "dunix", 5) == 0)
	    memcpy(value, "tru64", 5); /* Keep existing nul/version */

          if ((ptr = strchr(value, '-')) != NULL)
	    len = ptr - value;
	  else
	    len = strlen(value);

          if (len < namelen)
	    match = 0;
	  else
	    match = strncasecmp(value, namever, strlen(value)) == 0 ? SKIP_SYSTEM : 0;

          if (op)
	    *skip |= match;
	  else
	    *skip &= ~match;
        }
      }
    }
    else if (strncmp(buffer, "%format ", 8) == 0)
    {
     /*
      * Yes, do filtering based on the distribution format...
      */

      *skip &= ~SKIP_FORMAT;

      if (strcmp(buffer + 8, "all\n") != 0)
      {
        bufptr = buffer + 8;

	while (isspace((int)*bufptr))
	  bufptr ++;

        if (*bufptr != '!')
	  *skip  |= SKIP_FORMAT;

        while (*bufptr)
	{
	 /*
          * Skip leading whitespace...
	  */

	  while (isspace((int)*bufptr))
	    bufptr ++;

          if (!*bufptr)
	    break;

          if (*bufptr == '!')
	  {
	    op = 1;
	    bufptr ++;
	  }
	  else
	    op = 0;

	  for (ptr = value;
	       *bufptr && !isspace((int)*bufptr) &&
	           ptr < (value + sizeof(value) - 1);
	       *ptr++ = *bufptr++);

	  *ptr = '\0';

	  match = (strcasecmp(value, format) == 0) ? SKIP_FORMAT : 0;

          if (op)
	    *skip |= match;
	  else
	    *skip &= ~match;
        }
      }
    }
    else if (strncmp(buffer, "%ifdef ", 7) == 0 ||
             strncmp(buffer, "%elseifdef ", 11) == 0)
    {
     /*
      * Yes, do filtering based on the presence of variables...
      */

      if ((*skip & SKIP_IFACTIVE) && buffer[1] != 'e')
      {
       /*
        * Nested %if...
	*/

        fputs("epm: Warning, nested %ifdef's are not supported!\n", stderr);
	continue;
      }

      if (buffer[1] == 'e')
	bufptr = buffer + 11;
      else
	bufptr = buffer + 7;

      *skip |= SKIP_IF | SKIP_IFACTIVE;

      if (*skip & SKIP_IFSAT)
        continue;

      while (isspace((int)*bufptr))
	bufptr ++;

      if (*bufptr == '!')
        *skip &= ~SKIP_IF;

      while (*bufptr)
      {
       /*
        * Skip leading whitespace...
	*/

	while (isspace((int)*bufptr))
	  bufptr ++;

        if (!*bufptr)
	  break;

       /*
        * Get the variable name...
	*/

        if (*bufptr == '!')
	{
	  op = 1;
	  bufptr ++;
	}
	else
	  op = 0;

	for (ptr = value;
	     *bufptr && !isspace((int)*bufptr) &&
	         ptr < (value + sizeof(value) - 1);
	     *ptr++ = *bufptr++);

	*ptr = '\0';

	match = (getenv(value) != NULL) ? SKIP_IF : 0;

        if (op)
	  *skip |= match;
	else
	  *skip &= ~match;

        if (match)
	  *skip |= SKIP_IFSAT;
      }
    }
    else if (strncmp(buffer, "%if ", 4) == 0 ||
             strncmp(buffer, "%elseif ", 8) == 0)
    {
     /*
      * Yes, do filtering based on the value of variables...
      */

      if ((*skip & SKIP_IFACTIVE) && buffer[1] != 'e')
      {
       /*
        * Nested %if...
	*/

        fputs("epm: Warning, nested %if's are not supported!\n", stderr);
	continue;
      }

      if (buffer[1] == 'e')
        bufptr = buffer + 8;
      else
	bufptr = buffer + 4;

      *skip |= SKIP_IF | SKIP_IFACTIVE;

      if (*skip & SKIP_IFSAT)
        continue;

      while (isspace((int)*bufptr))
	bufptr ++;

      if (*bufptr == '!')
        *skip &= ~SKIP_IF;

      while (*bufptr)
      {
       /*
        * Skip leading whitespace...
	*/

	while (isspace((int)*bufptr))
	  bufptr ++;

        if (!*bufptr)
	  break;

       /*
        * Get the variable name...
	*/

        if (*bufptr == '!')
	{
	  op = 1;
	  bufptr ++;
	}
	else
	  op = 0;

	for (ptr = value;
	     *bufptr && !isspace((int)*bufptr) &&
	         ptr < (value + sizeof(value) - 1);
	     *ptr++ = *bufptr++);

	*ptr = '\0';

        match = ((var = getenv(value)) != NULL && *var) ? SKIP_IF : 0;

        if (op)
	  *skip |= match;
	else
	  *skip &= ~match;

        if (match)
	  *skip |= SKIP_IFSAT;
      }
    }
    else if (strcmp(buffer, "%else\n") == 0)
    {
     /*
      * Handle "else" condition of %ifdef statement...
      */

      if (!(*skip & SKIP_IFACTIVE))
      {
       /*
        * No matching %if/%ifdef statement...
	*/

        fputs("epm: Warning, no matching %if or %ifdef for %else!\n", stderr);
	break;
      }

      if (*skip & SKIP_IFSAT)
        *skip |= SKIP_IF;
      else
      {
	*skip &= ~SKIP_IF;
	*skip |= SKIP_IFSAT;
      }
    }
    else if (strcmp(buffer, "%endif\n") == 0)
    {
     /*
      * Cancel any filtering based on environment variables.
      */

      if (!(*skip & SKIP_IFACTIVE))
      {
       /*
        * No matching %if/%ifdef statement...
	*/

        fputs("epm: Warning, no matching %if or %ifdef for %endif!\n", stderr);
	break;
      }

      *skip &= ~(SKIP_IF | SKIP_IFACTIVE | SKIP_IFSAT);
    }
    else if (!(*skip & SKIP_MASK))
    {
     /*
      * Otherwise strip any trailing newlines and return the string!
      */

      if (buffer[strlen(buffer) - 1] == '\n')
        buffer[strlen(buffer) - 1] = '\0';

      return (buffer);
    }
  }

  return (NULL);
}


/*
 * 'get_vernumber()' - Convert a version string to a number...
 */

static int				/* O - Version number */
get_vernumber(const char *version)	/* I - Version string */
{
  int		numbers[4],		/* Raw version numbers */
		nnumbers,		/* Number of numbers in version */
		temp,			/* Temporary version number */
		offset;			/* Offset for last version number */
  const char	*ptr;			/* Pointer into string */


 /*
  * Loop through the version number string and construct a version number.
  */

  memset(numbers, 0, sizeof(numbers));

  for (ptr = version, offset = 0, temp = 0, nnumbers = 0;
       *ptr && !isspace((int)*ptr);
       ptr ++)
    if (isdigit((int)*ptr))
      temp = temp * 10 + *ptr - '0';
    else
    {
     /*
      * Add each mini version number (m.n.p) and patch/pre stuff...
      */

      if (nnumbers < 4)
      {
        numbers[nnumbers] = temp;
	nnumbers ++;
      }

      temp = 0;

      if (*ptr == '.')
	offset = 0;
      else if (*ptr == 'p' || *ptr == '-')
      {
	if (strncmp(ptr, "pre", 3) == 0)
	{
	  ptr += 2;
	  offset = -20;
	}
	else
	  offset = 0;

        nnumbers = 3;
      }
      else if (*ptr == 'b')
      {
	offset   = -50;
        nnumbers = 3;
      }
      else /* if (*ptr == 'a') */
      {
	offset   = -100;
        nnumbers = 3;
      }
    }

  if (nnumbers < 4)
    numbers[nnumbers] = temp;

 /*
  * Compute the version number as MMmmPPpp + offset
  */

  return (((numbers[0] * 100 + numbers[1]) * 100 + numbers[2]) * 100 +
          numbers[3] + offset);
}


/*
 * 'patmatch()' - Pattern matching...
 */

static int			/* O - 1 if match, 0 if no match */
patmatch(const char *s,		/* I - String to match against */
         const char *pat)	/* I - Pattern to match against */
{
 /*
  * Range check the input...
  */

  if (s == NULL || pat == NULL)
    return (0);

 /*
  * Loop through the pattern and match strings, and stop if we come to a
  * point where the strings don't match or we find a complete match.
  */

  while (*s != '\0' && *pat != '\0')
  {
    if (*pat == '*')
    {
     /*
      * Wildcard - 0 or more characters...
      */

      pat ++;
      if (*pat == '\0')
        return (1);	/* Last pattern char is *, so everything matches now... */

     /*
      * Test all remaining combinations until we get to the end of the string.
      */

      while (*s != '\0')
      {
        if (patmatch(s, pat))
	  return (1);

	s ++;
      }
    }
    else if (*pat == '?')
    {
     /*
      * Wildcard - 1 character...
      */

      pat ++;
      s ++;
      continue;
    }
    else if (*pat == '[')
    {
     /*
      * Match a character from the input set [chars]...
      */

      pat ++;
      while (*pat != ']' && *pat != '\0')
        if (*s == *pat)
	  break;
	else if (pat[1] == '-' && *s >= pat[0] && *s <= pat[2])
	  break;
	else
	  pat ++;

      if (*pat == ']' || *pat == '\0')
        return (0);

      while (*pat != ']' && *pat != '\0')
        pat ++;

      if (*pat == ']')
        pat ++;

      s ++;

      continue;
    }
    else if (*pat == '\\')
    {
     /*
      * Handle quoted characters...
      */

      pat ++;
    }

   /*
    * Stop if the pattern and string don't match...
    */

    if (*pat++ != *s++)
      return (0);
  }

 /*
  * Done parsing the pattern and string; return 1 if the last character matches
  * and 0 otherwise...
  */

  return (*s == *pat);
}


/*
 * End of "$Id: dist.c,v 1.42 2001/10/26 16:14:31 mike Exp $".
 */
