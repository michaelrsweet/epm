//
// "$Id: ListEditor3.cxx,v 1.1.2.2 2002/05/08 17:59:15 mike Exp $"
//
//   ESP List Editor callback methods for the ESP Package Manager (EPM).
//
//   Copyright 1999-2002 by Easy Software Products.
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
//

#include "ListEditor.h"
#include <FL/Fl_File_Chooser.H>


//
// 'ListEditor::build_cb()' - Build callback handler.
//

void
ListEditor::build_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::close_cb()' - Close callback handler.
//

void
ListEditor::close_cb(ListEditor *le)		// I - List editor window
{
  if (!le->check_save())
    return;

  if (le->next_ || first_)
  {
    // This isn't the only window open; just destroy it...
    delete le;
  }
  else
  {
    // This is the last window open; initialize things...
    if (le->dist_)
      free_dist(le->dist_);

    le->dist_ = (dist_t *)0;
    le->filename_[0] = '\0';
    le->list->clear();
    le->modified(0);
  }
}


//
// 'ListEditor::copy_cb()' - Copy callback handler.
//

void
ListEditor::copy_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::cut_cb()' - Cut callback handler.
//

void
ListEditor::cut_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::delete_cb()' - Delete callback handler.
//

void
ListEditor::delete_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::file_settings_cb()' - File settings callback handler.
//

void
ListEditor::file_settings_cb(ListEditor *le)	// I - List editor window
{
}


//
// 'ListEditor::help_cb()' - Help callback handler.
//

void
ListEditor::help_cb(ListEditor *le,		// I - List editor window
                    const char *html)		// I - HTML file to show
{
}


//
// 'ListEditor::list_cb()' - List callback handler.
//

void
ListEditor::list_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::margins_cb()' - Margins callback handler.
//

void
ListEditor::margins_cb(ListEditor *le)		// I - List editor window
{
  int	i, j, k, m, x;				// Looping vars
  int	update;					// Update the list?
  char	name[32];				// Attribute name


  for (i = 0, j = 0, x = 0, update = 0; i < 6; i ++)
    if (le->margin_items[i].value())
    {
      if (!le->margin_buttons[i]->visible())
      {
        update = 1;
	le->margin_buttons[i]->show();
	le->margin_buttons[i]->size(50, 20);

	for (k = i, m = x; k < 6; k ++)
	{
	  if (!le->margin_buttons[k]->visible())
	    continue;

	  le->margin_buttons[k]->redraw();
	  le->margin_buttons[k]->position(m, 25);
	  if ((m + le->margin_buttons[k]->w()) > le->window->w())
	    le->margin_buttons[k]->size(le->window->w() - m, 20);

          m += le->margin_buttons[k]->w();
	}
      }

      le->margins_[j] = le->margin_buttons[i]->w();
      x += le->margins_[j];
      j ++;
    }
    else if (le->margin_buttons[i]->visible())
    {
      update = 1;
      le->margin_buttons[i]->hide();

      for (k = i + 1, m = x; k < 6; k ++)
      {
	if (!le->margin_buttons[k]->visible())
	  continue;

	le->margin_buttons[k]->redraw();
	le->margin_buttons[k]->position(m, 25);
	if (k == 5)
	  le->margin_buttons[k]->size(le->window->w() - m, 20);

        m += le->margin_buttons[k]->w();
      }
    }

  le->margins_[0] -= le->list->iconsize() + 9;
  le->margins_[j] = 0;
  le->list->column_widths(le->margins_);

  for (i = 0; i < 6; i ++)
  {
    sprintf(name, "margin%d", i);

    if (le->margin_items[i].value())
      le->prefs_.set(name, le->margin_buttons[i]->w());
    else
      le->prefs_.set(name, 0);
  }

  if (update)
    le->update_list();
  else
    le->list->redraw();
}


//
// 'ListEditor::new_cb()' - New callback handler.
//

void
ListEditor::new_cb(ListEditor *le)		// I - List editor window
{
  ListEditor	*temp;				// New list editor window


  if (le->dist_)
  {
    // Open the file in a new window...
    temp = new ListEditor(NULL);
    temp->show();
  }
  else
  {
    // Open it in this window...
    le->open(NULL);
  }
}


//
// 'ListEditor::open_cb()' - Open callback handler.
//

void
ListEditor::open_cb(ListEditor *le)		// I - List editor window
{
  const char	*listfile;			// New list file...
  ListEditor	*temp;				// New list editor window


  if ((listfile = fl_file_chooser("List File?", "*.list", le->filename_)) != NULL)
  {
    if (le->dist_)
    {
      // Open the file in a new window...
      temp = new ListEditor(listfile);
      temp->show();
    }
    else
    {
      // Open it in this window...
      le->open(listfile);
    }
  }
}


//
// 'ListEditor::open_history_cb()' - Open history callback handler.
//

void
ListEditor::open_history_cb(ListEditor *le,	// I - List editor window
                            const char *listfile)// I - List file to load
{
  ListEditor	*temp;				// New list editor window


  if (le->dist_)
  {
    // Open the file in a new window...
    temp = new ListEditor(listfile);
    temp->show();
  }
  else
  {
    // Open it in this window...
    le->open(listfile);
  }
}


//
// 'ListEditor::paste_cb()' - Paste callback handler.
//

void
ListEditor::paste_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::project_settings_cb()' - Project settings callback handler.
//

void
ListEditor::project_settings_cb(ListEditor *le)	// I - List editor window
{
}


//
// 'ListEditor::quit_cb()' - Quit callback handler.
//

void
ListEditor::quit_cb(ListEditor *le)		// I - List editor window
{
  ListEditor	*temp;				// Current window


  // Make sure all windows are saved...
  for (temp = first_; temp; temp = temp->next_)
    if (!temp->check_save())
      return;

  // Delete all windows...
  while (first_)
    delete first_;

  // Exit...
  exit(0);
}


//
// 'ListEditor::save_cb()' - Save callback handler.
//

void
ListEditor::save_cb(ListEditor *le)		// I - List editor window
{
}


//
// 'ListEditor::save_as_cb()' - Save As callback handler.
//

void
ListEditor::save_as_cb(ListEditor *le)		// I - List editor window
{
}


//
// End of "$Id: ListEditor3.cxx,v 1.1.2.2 2002/05/08 17:59:15 mike Exp $".
//
