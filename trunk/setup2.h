//
// "$Id: setup2.h,v 1.5 2001/06/28 22:32:29 mike Exp $"
//
//   ESP Software Wizard header file for the ESP Package Manager (EPM).
//
//   Copyright 1999-2001 by Easy Software Products.
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
};

struct dtype_t			//// Installation types
{
  char		label[64];	// Type name;
  int		num_products;	// Number of products to install (0 = select)
  int		products[50];	// Products to install
};


//
// Globals...
//

#ifdef _SETUP2_CXX_
#  define VAR
#else
#  define VAR	extern
#endif // _SETUP2_CXX_

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
void	get_dists(const char *d);
void	get_installed(void);
int	install_dist(const dist_t *dist);
void	load_image(void);
void	load_types(void);
int	sort_dists(const dist_t *d0, const dist_t *d1);


//
// End of "$Id: setup2.h,v 1.5 2001/06/28 22:32:29 mike Exp $".
//
