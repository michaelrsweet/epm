//
// "$Id: ListEditor3.cxx,v 1.1.2.3 2002/05/10 00:19:46 mike Exp $"
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
ListEditor::file_settings_cb(ListEditor *le,	// I - List editor window
                             int        save)	// I - 1 = save, 0 = load
{
  int			i;			// Looping var
  int			count;			// Number of selected files
  char			type;			// Type of file
  file_t		*file;			// Current file
  const char		*slash;			// Directory separator
  char			newdst[512];		// New destination path
  int			newmode;		// New permissions
  static const char	*types = "fcmidlr";	// File types


  // Load or save the file settings...
  if (save)
  {
    // First figure out how many files are selected...
    type = '\0';

    for (i = 1, count = 0, file = le->dist_->files; i <= le->list->size(); i ++, file ++)
      if (le->list->selected(i))
      {
	count ++;

	if (count > 1)
	{
	  if (type != tolower(file->type))
	    type = '\0';
	}
	else
	  type = tolower(file->type);
    }

    // Get new file permissions...
    if (le->perm_group->active())
    {
      newmode = 0;

      if (le->user_read_button->value())
	newmode |= 00400;
      if (le->user_write_button->value())
	newmode |= 00200;
      if (le->user_exec_button->value())
	newmode |= 00100;
      if (le->user_set_button->value())
	newmode |= 04000;

      if (le->group_read_button->value())
	newmode |= 00040;
      if (le->group_write_button->value())
	newmode |= 00020;
      if (le->group_exec_button->value())
	newmode |= 00010;
      if (le->group_set_button->value())
	newmode |= 02000;

      if (le->other_read_button->value())
	newmode |= 00004;
      if (le->other_write_button->value())
	newmode |= 00002;
      if (le->other_exec_button->value())
	newmode |= 00001;
      if (le->other_temp_button->value())
	newmode |= 01000;
    }
    else
      newmode = 0;

    // Then apply any changes...
    for (i = 1, file = le->dist_->files; i <= le->list->size(); i ++, file ++)
      if (le->list->selected(i))
      {
	if (count > 1)
	{
	  if (type)
	    file->type = types[le->type_chooser->value()];

          if (le->dst_path_field->value() &&
	      le->dst_path_field->value()[0])
	  {
	    // Remap destination directory...
	    if ((slash = strrchr(file->dst, '/')) != NULL)
	      slash ++;
	    else
	      slash = file->dst;

            snprintf(newdst, sizeof(newdst), "%s/%s",
	             le->dst_path_field->value(), slash);

            strncpy(file->dst, newdst, sizeof(file->dst) - 1);
	  }
	}
	else
	{
	  file->type = types[le->type_chooser->value()];

	  strncpy(file->dst, le->dst_path_field->value(),
	          sizeof(file->dst) - 1);
	  strncpy(file->src, le->src_path_field->value(),
	          sizeof(file->src) - 1);
	}

	if (le->upgrade_button->value())
	  file->type = toupper(file->type);
	else
	  file->type = tolower(file->type);

        if (newmode)
	  file->mode = newmode;

	strncpy(file->user, le->user_field->value(), sizeof(file->user) - 1);
	strncpy(file->group, le->group_field->value(), sizeof(file->group) - 1);

        file->subpackage = find_subpackage(le->dist_, 
	                                   le->subpackage_field->value());
      }

    le->file_window->hide();
    le->modified(1);
    le->update_list();
  }
  else
  {
    // Look through the selected files...
    type = '\0';

    for (i = 1, count = 0, file = le->dist_->files; i <= le->list->size(); i ++, file ++)
      if (le->list->selected(i))
      {
	count ++;

	if (count > 1)
	{
          if (type != tolower(file->type))
	  {
	    le->type_chooser->deactivate();
	    le->perm_group->deactivate();
	  }

	  le->src_path_field->deactivate();
	  le->src_path_field->value("");
	  le->dst_path_field->value("");
	}
	else
	{
	  type = tolower(file->type);

	  le->type_chooser->activate();

	  switch (type)
	  {
	    case 'f' :
		le->type_chooser->value(0);
		break;
	    case 'c' :
		le->type_chooser->value(1);
		break;
	    case 'm' :
		le->type_chooser->value(2);
		break;
	    case 'i' :
		le->type_chooser->value(3);
		break;
	    case 'd' :
		le->type_chooser->value(4);
		break;
	    case 'l' :
		le->type_chooser->value(5);
		break;
	    case 'r' :
		le->type_chooser->value(6);
		break;
	  }

          if (isupper(file->type))
	    le->upgrade_button->set();
	  else
	    le->upgrade_button->clear();

	  le->perm_group->activate();

          if (file->mode & 00400)
	    le->user_read_button->set();
	  else
	    le->user_read_button->clear();
          if (file->mode & 00200)
	    le->user_write_button->set();
	  else
	    le->user_write_button->clear();
          if (file->mode & 00100)
	    le->user_exec_button->set();
	  else
	    le->user_exec_button->clear();
          if (file->mode & 04000)
	    le->user_set_button->set();
	  else
	    le->user_set_button->clear();

          if (file->mode & 00040)
	    le->group_read_button->set();
	  else
	    le->group_read_button->clear();
          if (file->mode & 00020)
	    le->group_write_button->set();
	  else
	    le->group_write_button->clear();
          if (file->mode & 00010)
	    le->group_exec_button->set();
	  else
	    le->group_exec_button->clear();
          if (file->mode & 02000)
	    le->group_set_button->set();
	  else
	    le->group_set_button->clear();

          if (file->mode & 00004)
	    le->other_read_button->set();
	  else
	    le->other_read_button->clear();
          if (file->mode & 00002)
	    le->other_write_button->set();
	  else
	    le->other_write_button->clear();
          if (file->mode & 00001)
	    le->other_exec_button->set();
	  else
	    le->other_exec_button->clear();
          if (file->mode & 01000)
	    le->other_temp_button->set();
	  else
	    le->other_temp_button->clear();

	  le->user_field->value(file->user);
	  le->group_field->value(file->group);

	  le->src_path_field->activate();
	  le->src_path_field->value(file->src);
	  le->dst_path_field->value(file->dst);

          le->subpackage_field->value(file->subpackage ? file->subpackage : "");
	}
      }

    le->file_ok_button->deactivate();
    le->file_window->hotspot(le->file_window);
    le->file_window->show();
  }
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
  int	i;					// Looping var


  if (Fl::event_clicks())
    file_settings_cb(le, 0);
  else
  {
    le->file_settings_item->deactivate();

    for (i = 1; i <= le->list->size(); i ++)
      if (le->list->selected(i))
      {
        le->file_settings_item->activate();
        break;
      }
  }
}


//
// 'ListEditor::margins_cb()' - Margins callback handler.
//

void
ListEditor::margins_cb(ListEditor *le)		// I - List editor window
{
  int		i, j;				// Looping vars
  int		update;				// Update the list?
  char		name[32];			// Attribute name
  ListColumn	*c;				// Current column


  for (i = 0, j = 0, update = 0, c = le->margin_manager->columns();
       i < le->margin_manager->num_columns();
       i ++, c ++)
  {
    if (le->margin_items[i].value())
    {
      if (!c->width)
      {
	update   = 1;
	c->width = c->min_width;

	le->margin_manager->redraw();
      }

      le->margins_[j] = c->width;
      j ++;
    }
    else if (c->width)
    {
      update   = 1;
      c->width = 0;

      le->margin_manager->redraw();
    }

    sprintf(name, "margin%d", i);
    le->prefs_.set(name, c->width);
  }

  le->margins_[0] -= le->list->iconsize() + 9;
  le->margins_[j] = 0;
  le->list->column_widths(le->margins_);

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
  if (write_dist(le->filename_, le->dist_))
    fl_alert("Unable to save \"%s\":\n\n%s", le->filename_, strerror(errno));
  else
    le->modified(0);
}


//
// 'ListEditor::save_as_cb()' - Save As callback handler.
//

void
ListEditor::save_as_cb(ListEditor *le)		// I - List editor window
{
}


//
// End of "$Id: ListEditor3.cxx,v 1.1.2.3 2002/05/10 00:19:46 mike Exp $".
//
