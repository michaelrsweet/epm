//
// "$Id: epmeditor.cxx,v 1.3 2005/01/11 21:36:57 mike Exp $"
//
//   ESP List Editor main entry for the ESP Package Manager (EPM).
//
//   Copyright 1999-2005 by Easy Software Products.
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
// Contents:
//
//   main() - Main entry for list editor...
//

#include "ListEditor.h"
#include <FL/Fl_File_Icon.H>


//
// Globals that the core functions are looking for...
//

int	Verbosity = 0;


//
// 'main()' - Main entry for software wizard...
//

int						// O - Exit status
main(int  argc,					// I - Number of command-line arguments
     char *argv[])				// I - Command-line arguments
{
  ListEditor	*w;				// Main window...


//  Fl::args(argc, argv);

  w = new ListEditor(argv[1]);

  w->show();

  Fl::run();

  return (0);
}


//
// End of "$Id: epmeditor.cxx,v 1.3 2005/01/11 21:36:57 mike Exp $".
//
