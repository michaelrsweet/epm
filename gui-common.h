//
// "$Id: gui-common.h,v 1.1 2003/01/30 04:29:34 mike Exp $"
//
//   ESP Software Wizard common header file for the ESP Package Manager (EPM).
//
//   Copyright 1999-2003 by Easy Software Products.
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2, or (at your option)
//   any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//

//
// Include necessary headers...
//

#include "epmstring.h"
#include <stdio.h>
#include <stdlib.h>


//
// Dependency types...
//

enum
{
  DEPEND_REQUIRES,		// This product requires
  DEPEND_INCOMPAT,		// This product is incompatible with
  DEPEND_REPLACES,		// This product replaces
  DEPEND_PROVIDES		// This product provides
};


//
// Distribution structures...
//

struct depend_t			//// Dependencies
{
  int	type;			// Type of dependency
  char	product[64];		// Name of product or file
  int	vernumber[2];		// Version number(s)
};

struct dist_t			//// Distributions
{
  char		product[64];	// Product name
  char		name[256];	// Product long name
  char		version[32];	// Product version
  int		vernumber;	// Version number
  int		num_depends;	// Number of dependencies
  depend_t	*depends;	// Dependencies
  int		rootsize,	// Size of root partition files in kbytes
		usrsize;	// Size of /usr partition files in kbytes
};

struct dtype_t			//// Installation types
{
  char		label[80];	// Type name;
  int		num_products;	// Number of products to install (0 = select)
  int		products[50];	// Products to install
  int		size;		// Size of products in kbytes
};


//
// Define a C API function type for comparisons...
//

extern "C" {
typedef int (*compare_func_t)(const void *, const void *);
}


//
// Globals...
//

#ifdef _DEFINE_GLOBALS_
#  define VAR
#else
#  define VAR	extern
#endif // _DEFINE_GLOBALS_

VAR int		NumDists;	// Number of distributions in directory
VAR dist_t	*Dists;		// Distributions in directory
VAR int		NumInstalled;	// Number of distributions installed
VAR dist_t	*Installed;	// Distributions installed
VAR int		NumInstTypes;	// Number of installation types
VAR dtype_t	InstTypes[8];	// Installation types


//
// Prototypes...
//

void	add_depend(dist_t *d, int type, const char *name, int lowver, int hiver);
dist_t	*add_dist(int *num_d, dist_t **d);
dist_t	*find_dist(const char *name, int num_d, dist_t *d);
void	get_installed(void);
int	sort_dists(const dist_t *d0, const dist_t *d1);


//
// End of "$Id: gui-common.h,v 1.1 2003/01/30 04:29:34 mike Exp $".
//
