/*
 * "$Id: setld.c,v 1.1 2001/04/25 13:35:02 mike Exp $"
 *
 *   Tru64 package gateway for the ESP Package Manager (EPM)
 *
 *   Copyright 2001 by Easy Software Products and Aneesh Kumar.
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
 *   make_setld() - Make a Tru64 setld package.
 *   insert()     - insert a string into another at a specified index.
 *   copy()       - To copy file from one location to the other.
 *   mkdir_r()    - To create directory by making parent directory if needed.
 *   update_mi()  - To update the Master index file.
 */

/*
 * Include necessary headers...
 */

#include "epm.h"
#define BUFSIZE 1024
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TMP_FILE "/tmp/sort.mi"


/*
 * Local functions...
 */


static char	*insert(char *srcstr ,char *insstr, int index);
static int	copy(char *from, char *to, mode_t mode);
static int	mkdir_r(char *name, mode_t mode);
static char	*update_mi(char *name, const char *prodname,
		           int version_number);


/*
 * Local globals...
 */

static FILE	*mifp; /* Master index file is manipulated by different functions */
static char	insstr[230]; /* Global insert string /opt/OAT100 */


/*
 * 'make_setld()' - Make a Tru64 setld package.
 */

int                                     /* O - 0 = success, 1 = fail */
make_setld(const char     *prodname,    /* I - Product short name */
         const char     *directory,     /* I - Directory for distribution files */
         const char     *platname,      /* I - Platform name */
         dist_t         *dist,          /* I - Distribution information */
         struct utsname *platform)      /* I - Platform information */
{
  int		i = 0 ;
  double	version_number;
  char		*desstr;
  FILE		*kitfp;
  char		mifilename[230];
  char		kitfilename[230];
  int		status;
  int		tmp_mides;


  if (Verbosity)
    puts("Determining the version number");

  version_number = strtod(dist->version, NULL);

  if (version_number < 10)
  {
    version_number += 10;
    version_number *= 10;
  }
  else if (version_number < 100)
  {
    version_number *= 10;
  }
  else if(version_number > 999)
  {
    puts("Wrong version number (version number should be < 999)");
    return (1);
  }

  if (Verbosity)
    printf("Creating the packaging directory %s\n", directory);

  if (chdir(directory))
  {
    printf("Unable to chdir to the packaging directory %s (%s)\n",
           directory, strerror(errno));
    return (1);
  }

  if (Verbosity)
    printf("Creating the packaging directory  %s/src\n", directory);

  if (mkdir("src", 0777) < 0)
  {
    if (errno != EEXIST)
    {
      puts("Error in creating the Packaging area");
      return (1);
    }
  }

  if (Verbosity)
    printf("Creating the packaging directory  %s/data\n", directory);

  if (mkdir("data", 0777) < 0)
  {
    if (errno != EEXIST)
    {
      puts("Error in creating the Packaging area");
      return (1);
    }
  }

  if (Verbosity)
    printf("Creating the packaging directory  %s/output\n", directory);

  if (mkdir("output", 0777) < 0 )
  {
    if (errno != EEXIST)
    {
      puts("Error in creating the Packaging area");
      return (1);
    }
  }

 /* chdir to 'platform'/src directory */
  if (chdir("src"))
  {
    printf("Unable to chdir to the packaging directory src (%s)\n",
           strerror(errno));
    return (1);
  }

  if (Verbosity)
    puts("Creating the Master index file");

  sprintf(mifilename, "../data/%s%d.mi", prodname, (int)version_number);

  if ((mifp = fopen(mifilename,"w+")) == NULL)
  {
    printf("Unable to create the master index file %s\n", mifilename);
    return (1);
  }

  if (Verbosity)
    puts("Copying files to the temporary area");

  sprintf(insstr, "/opt/%s%d/", prodname, (int)version_number);
  for( i = 0 ; i < dist->num_files; i++)
  {
    switch (dist->files[i].type)
    {
      case 'd':
	  desstr = update_mi(dist->files[i].dst, prodname,
                             (int)version_number);

	 /* desstr+ to remove '/' from the begining */
	  if (mkdir_r(desstr + 1, dist->files[i].mode) < 0)
	  {
            perror("Error in creating the Packaging area ");
            return (1);
	  }

	 /* directory for creating symlink  */
	  if(mkdir_r(dist->files[i].dst + 1, dist->files[i].mode) < 0)
	  {
            perror("Error in creating the Packaging area");
            return (1);
	  }
	  break;

      case 'f' :
	 /* CWD 'platform'/src directory */
	 /* copy files from ../../ */
          desstr = update_mi(dist->files[i].dst, prodname,
                             (int)version_number);
         /* src should not be a relative path */

          if (copy(dist->files[i].src, desstr + 1, dist->files[i].mode))
	  {
            printf("Error in copying files to the packaging area\n");
            return (1);
          }

          if (symlink(desstr, dist->files[i].dst + 1) < 0)
	  {
            if (errno != EEXIST)
	    {
              printf("Error in creating links\n");
              return (1);
            }
          }
	  break;

      case 'l' :
          desstr = update_mi(dist->files[i].dst, prodname,
                             (int)version_number);
          if (symlink(desstr, dist->files[i].dst + 1) < 0)
	  {
            if (errno != EEXIST)
	    {
              printf("Error in creating links\n");
              return (1);
            }
          }
         /* Creating the actual symlink +1 to remove /  */
          if (symlink(dist->files[i].src, desstr + 1) < 0)
	  {
            if (errno != EEXIST)
	    {
              printf("Error in creating links\n");
              return (1);
            }
          }
	  break;

      default :
          printf("Not supported file type\n");
          return (1);
    }

    free(desstr);
  }

  if (Verbosity)
    puts("Creating the kit file");

  sprintf(kitfilename, "../data/%s%d.k", prodname, (int)version_number);

  if ((kitfp = fopen(kitfilename,"w+")) == NULL)
  {
    printf("Unable to create the kit file %s\n", kitfilename);
    return (1);
  }

  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"#  Product level attributes\n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"NAME='%s'\n",dist->product);
  fprintf(kitfp,"CODE=%s\n",prodname);
  fprintf(kitfp,"VERS=%d\n",(int)version_number);
  fprintf(kitfp,"MI=%s\n",mifilename);
  fprintf(kitfp,"COMPRESS=1\n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"#  Subset definitions                  \n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"#                   \n");
  fprintf(kitfp,"%%%%\n");
  if (dist->num_descriptions >= 1)
    fprintf(kitfp, "%s%d  .  0 '%s'\n", prodname, (int)version_number,
            dist->descriptions[0]);
  else
    fprintf(kitfp, "%s%d  .  0 'No product Description given'\n",
            prodname,(int)version_number);

  fclose(kitfp);
  fclose(mifp);

 /* I need to sort the Master Index file . What the hell ??@!@# */
  if(fork() == 0) {
/* I am trying to make  the tmp file as the stdout if it fails no
meaning in
continuing */
          if((tmp_mides = open(TMP_FILE,O_CREAT|O_RDWR,0777))  < 0    ){
                  printf("Error in opening temporary Master index file \n");
                   exit(1);
          }
          close(1);
          if(dup(tmp_mides) != 1 ) {
                  printf("Error in dup \n");
                   exit(1);
          }


          execl("/sbin/sort","sort",mifilename,(char *)0);
          printf("\nPackaging Failed while sorting mi file %s\n",strerror(errno));
          exit(1);

  }
  wait((int *)&status);

/* check whether sorting is successfull or not */
/* Checking for 256 Acutally I don't know why 256 but Tru64 returns that( even linux )  */
  if(WEXITSTATUS(status) == 1 )
          return (1);
  if( copy(TMP_FILE,mifilename,0644)){
          printf("Error in copying sorted Master index file  \n");

                  return (1);
  }
/* Unlink the temp file created */
  if( unlink(TMP_FILE)  < 0 ){
          if(Verbosity)
                  printf("Error in  removing file  temporary mi file    \n");
  }



  if( fork() == 0 ){
/* I am right now in the src directory */

execl("/usr/bin/kits","kits",kitfilename,".","../output",(char *)0);
          printf("\nPackaging Failed while building kits %s\n",strerror(errno));
          exit(1);

  }else
          wait(&status);
/* check whether kits is  successfull or not */
  if(WEXITSTATUS(status) == 1 )
          return (1);

  return 0 ;
}


/*
 * 'insert()' - insert a string into another at a specified index.
 */

static char * insert(char *srcstr ,char *insstr,int index )
/* returns the final string */
{
        char *desstr;
        if(srcstr == NULL)
                return NULL;
        desstr = (char *)malloc((strlen(srcstr))+(strlen(insstr)));
        strncpy(desstr,srcstr,index);
        desstr[index]='\0';
        strcat(desstr,insstr);
        strcat(desstr,(srcstr+(index+1)));

        return desstr;

}


/*
 * 'copy()' - To copy file from one location to the other.
 */

static int copy(char *from , char *to , mode_t mode)
/* Return 0 on success 1 on failure */
{
        int fromfd, tofd;
        char buf[BUFSIZE];
        int count;
/* Unlink the to file created */
        if( unlink(to)  < 0 ){
                if(Verbosity)
                        printf("Error in  removing file  file %s    \n",
to );
        }


        if((fromfd = open(from,O_RDONLY)) < 0 ) {
                printf("Error in opening file %s\n", from);
                return (1);
        }
        if((tofd = open(to,O_CREAT|O_RDWR)) < 0 ) {
                printf("Error in opening file %s\n", to);
                return (1);
        }
        while((count = read(fromfd,buf,BUFSIZE)) > 0 ) {
                write(tofd,buf,count);
                bzero(buf,BUFSIZE);
        }


        /* changing the mode to the required mode */
        if( fchmod(tofd,mode) < 0 ){
                printf("Error in changing mode of the file %s\n", to);
                return (1);
        }
        close(fromfd);
        close(tofd);




        return 0 ;
}


/*
 * 'mkdir_r()' - To create directory by making parent directory if needed.
 */

static int mkdir_r(char *name , mode_t mode)
{
        char *tok;
        char dirname[230];

         if((tok= strtok(name,"/"))  == NULL )
                        return -1;
        strcpy(dirname,tok);
        if( mkdir(dirname,mode) < 0){
                if( errno != EEXIST ){
                        perror("Error in creating the  Packaging area");
                        return -1;
                }
        }

         while((tok= strtok(NULL,"/")) != NULL){
                strcat(dirname,"/");
                strcat(dirname,tok);
                if( mkdir(dirname,mode) < 0){
                        if( errno != EEXIST ){
                                perror(" mkdir Error in creating the Packaging area ");
                                return -1;
                        }
                }

        }

    return 0;

}


/*
 * 'update_mi()' - To update the Master index file.
 */

static char *  update_mi(char *name, const char *prodname, int
version_number)
{
        char *desstr;
        if(!strncmp(name,"/usr",4)){
                desstr = insert(name,insstr,4);
                fprintf(mifp,"0\t.%s\t%s%d\n",name,
                        prodname,(int)version_number);

fprintf(mifp,"0\t.%s\t%s%d\n",desstr,prodname,(int)version_number);
        }
        else
        if(!strncmp(name,"/var",4)){
                desstr=  insert(name,insstr,4);
                fprintf(mifp,"2\t.%s\t%s%d\n",name,
                        prodname,(int)version_number);

fprintf(mifp,"2\t.%s\t%s%d\n",desstr,prodname,(int)version_number);
        }
        else{
                desstr = insert(name,insstr,1);
                fprintf(mifp,"0\t.%s\t%s%d\n",name,
                        prodname,(int)version_number);

fprintf(mifp,"0\t.%s\t%s%d\n",desstr,prodname,(int)version_number);
        }
        return desstr;

}


/*
 * End of "$Id: setld.c,v 1.1 2001/04/25 13:35:02 mike Exp $".
 */
