//
// "$Id: setup2.h,v 1.3 2000/01/04 13:45:41 mike Exp $"
//
//   ESP Software Wizard header file for the ESP Package Manager (EPM).
//
//   Copyright 1999-2000 by Easy Software Products.
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>


//
// Distribution structure...
//

struct dist_t
{
  char	product[64];		// Product name
  char	name[256];		// Product long name
  char	version[32];		// Product version
  int	num_requires;		// Number of required files/products
  char	**requires;		// Requires files/products
  int	num_incompats;		// Number of incompatible files/products
  char	**incompats;		// Incompatible files/products
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


//
// Prototypes...
//

void	get_dists(const char *d);
int	install_dist(const dist_t *dist);
void	load_image(void);
int	sort_dists(const dist_t *d0, const dist_t *d1);


//
// End of "$Id: setup2.h,v 1.3 2000/01/04 13:45:41 mike Exp $".
//
