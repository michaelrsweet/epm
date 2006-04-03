//
// "$Id: ListManager.cxx,v 1.3 2005/01/11 21:36:57 mike Exp $"
//
//   List manager widget methods for the ESP Package Manager (EPM).
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
//   ListManager::ListManager()  - Create a new list manager widget.
//   ListManager::~ListManager() - Destroy a list manager widget.
//   ListManager::add()          - Add a new column to the list manager.
//   ListManager::clear()        - Delete any columns.
//   ListManager::draw()         - Redraw the widget.
//   ListManager::handle()       - Handle events in the list manager.
//

#include "ListManager.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include "epmstring.h"


//
// 'ListManager::ListManager()' - Create a new list manager widget.
//

ListManager::ListManager(int        X,	// I - X position of widget
                         int        Y,	// I - Y position of widget
			 int        W,	// I - Width of widget
			 int        H,	// I - Height of widget
			 const char *L)	// I - Label string (not used)
  : Fl_Widget(X, Y, W, H, L)
{
  num_columns_ = 0;
  columns_     = 0;
}


//
// 'ListManager::~ListManager()' - Destroy a list manager widget.
//

ListManager::~ListManager()
{
  clear();
}


//
// 'ListManager::add()' - Add a new column to the list manager.
//

void
ListManager::add(const char *l,		// I - Label for string
                 int        width,	// I - Initial width of column
		 int        minw,	// I - Minimum width of column
		 int        maxw)	// I - Maximum width of column
{
  ListColumn	*temp;			// New column array


  temp = new ListColumn[num_columns_ + 1];

  if (num_columns_ > 0)
    memcpy(temp, columns_, sizeof(ListColumn) * num_columns_);

  delete[] columns_;
  columns_ = temp;

  temp += num_columns_;
  num_columns_ ++;

  temp->label     = l;
  temp->lx        = 0;
  temp->lw        = 0;
  temp->width     = width;
  temp->max_width = maxw;
  temp->min_width = minw;

  fl_font(labelfont(), labelsize());
  minw = (int)fl_width(l) + 10;

  if (minw > temp->min_width)
    temp->min_width = minw;

  redraw();
}


//
// 'ListManager::clear()' - Delete any columns.
//

void
ListManager::clear()
{
  if (num_columns_)
    delete[] columns_;

  num_columns_ = 0;
  columns_     = 0;

  redraw();
}


//
// 'ListManager::draw()' - Redraw the widget.
//

void
ListManager::draw()
{
  int		i, j;			// Looping vars
  int		lx;			// Current X position
  int		lw;			// Current width
  ListColumn	*c;			// Current column
  Fl_Color	light, dark;		// Colors...


  // Draw the box...
  draw_box();

  // Get the separator colors...
  light = fl_color_average(color(), FL_WHITE, 0.5f);
  dark  = fl_color_average(color(), FL_BLACK, 0.5f);

  // Draw the columns...
  for (i = num_columns_, c = columns_, lx = 0; i > 0; i --, c ++, lx += lw)
  {
    // Stop if we've gone past the end of the widget...
    if (lx >= w())
      break;

    // Don't do anything if this column is hidden...
    lw    = c->width;
    c->lx = lx;
    c->lw = lw;

    if (!lw)
      continue;

    // See if any of the remaining columns are active...
    for (j = 1; j < i; j ++)
      if (c[j].width)
        break;

    // Nope, adjust this last column to use all remaining space...
    if (j >= i)
    {
      lw    = w() - lx;
      c->lw = lw;
    }

    // Draw the label text...
    fl_color(labelcolor());
    fl_draw((const char *)c->label, x() + lx, y(), (int)lw, h(),
            (Fl_Align)(FL_ALIGN_INSIDE | FL_ALIGN_CLIP));

    // Draw the separator...
    if (j < i)
    {
      fl_color(dark);
      fl_line(x() + lx + lw, y() + 2,
              x() + lx + lw, y() + h() - 3);

      fl_color(light);
      fl_line(x() + lx + lw + 1, y() + 3,
              x() + lx + lw + 1, y() + h() - 2);
    }
  }
}


//
// 'ListManager::handle()' - Handle events in the list manager.
//

int
ListManager::handle(int event)		// I - Event to handle
{
  int		i;			// Looping var
  ListColumn	*c;			// Current column
  int		mx;			// Relative mouse X position


  switch (event)
  {
    case FL_LEAVE :
        window()->cursor(FL_CURSOR_DEFAULT);
	drag_ = 0;
        return (1);

    case FL_MOVE :
    case FL_PUSH :
        if (Fl::event_button() != FL_LEFT_MOUSE)
	  return (0);

    case FL_ENTER :
	drag_ = 0;

	for (i = num_columns_, c = columns_; i > 0; i --, c ++)
	{
	  // Stop if we've gone past the end of the widget...
	  if ((c->lx + c->lw) >= w())
	    break;

	  // Don't do anything if this column is hidden...
	  if (!c->lw)
	    continue;

          // See if we are over the border...
          mx = Fl::event_x() - x() - c->lx - c->lw;
	  if (mx >= -5 && mx <= 5)
	  {
	    // Yes, hover and/or drag...
	    drag_ = c;
	    break;
	  }
	}

	// Update the cursor as needed...
	if (drag_)
          window()->cursor(FL_CURSOR_WE);
	else
          window()->cursor(FL_CURSOR_DEFAULT);

	return (1);

    case FL_DRAG :
        mx = Fl::event_x() - x() - drag_->lx;

	if (mx < drag_->min_width || mx > drag_->max_width)
	  return (1);

        drag_->width = mx;
	redraw();

        do_callback();
        return (1);

    default :
        return (0);
  }
}


//
// End of "$Id: ListManager.cxx,v 1.3 2005/01/11 21:36:57 mike Exp $".
//
