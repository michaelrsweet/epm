//
// "$Id: ListManager.h,v 1.3 2005/01/11 21:36:57 mike Exp $"
//
//   List manager widget definitions for the ESP Package Manager (EPM).
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

#include <FL/Fl_Widget.H>


struct ListColumn
{
  const char	*label;		// Label
  int		lx,		// Actual label position
		lw,		// Actual label width
		min_width,	// Minimum width
		max_width,	// Maximum width
		width;		// Current width (0 = hide)
};


class ListManager : public Fl_Widget
{
  private:

  int		num_columns_;
  ListColumn	*columns_;
  ListColumn	*drag_;

  protected:

  virtual void	draw();

  public:

  ListManager(int X, int Y, int W, int H, const char *L = (const char *)0);
  ~ListManager();

  void		add(const char *l, int width, int minw = 0, int maxw = 1024);
  void		clear();
  virtual int	handle(int event);
  int		num_columns() const { return (num_columns_); }
  ListColumn	*columns() { return (columns_); }
  ListColumn	*column(int c) { return (columns_ + c); }
};


//
// End of "$Id: ListManager.h,v 1.3 2005/01/11 21:36:57 mike Exp $".
//
