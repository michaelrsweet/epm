//
// "$Id: ListEditor3.cxx,v 1.3 2005/01/11 21:36:57 mike Exp $"
//
//   ESP List Editor callback methods for the ESP Package Manager (EPM).
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
//

#include "ListEditor.h"
#include <FL/Fl_File_Chooser.H>


//
// 'ListEditor::add_subpkg_cb()' - Add a subpackage to the project.
//

void
ListEditor::add_subpkg_cb()			// I - List editor window
{
}


//
// 'ListEditor::build_cb()' - Build callback handler.
//

void
ListEditor::build_cb()				// I - List editor window
{
}


//
// 'ListEditor::close_cb()' - Close callback handler.
//

void
ListEditor::close_cb()				// I - List editor window
{
  if (!check_save())
    return;

  if (next_ || first_)
  {
    // This isn't the only window open; just destroy it...
    delete this;
  }
  else
  {
    // This is the last window open; initialize things...
    if (dist_)
      free_dist(dist_);

    dist_ = (dist_t *)0;
    filename_[0] = '\0';
    list->clear();
    modified(0);
  }
}


//
// 'ListEditor::copy_cb()' - Copy callback handler.
//

void
ListEditor::copy_cb()				// I - List editor window
{
}


//
// 'ListEditor::cut_cb()' - Cut callback handler.
//

void
ListEditor::cut_cb()				// I - List editor window
{
}


//
// 'ListEditor::delete_cb()' - Delete callback handler.
//

void
ListEditor::delete_cb()				// I - List editor window
{
}


//
// 'ListEditor::delete_subpkg_cb()' - Remove a subpackage from the project.
//

void
ListEditor::delete_subpkg_cb()			// I - List editor window
{
}


//
// 'ListEditor::file_settings_cb()' - File settings callback handler.
//

void
ListEditor::file_settings_cb(int save)		// I - 1 = save, 0 = load
{
  int			i, j;			// Looping vars
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

    for (i = 1, count = 0, file = dist_->files; i <= list->size(); i ++, file ++)
      if (list->selected(i))
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
    if (perm_group->active())
    {
      newmode = 0;

      if (user_read_button->value())
	newmode |= 00400;
      if (user_write_button->value())
	newmode |= 00200;
      if (user_exec_button->value())
	newmode |= 00100;
      if (user_set_button->value())
	newmode |= 04000;

      if (group_read_button->value())
	newmode |= 00040;
      if (group_write_button->value())
	newmode |= 00020;
      if (group_exec_button->value())
	newmode |= 00010;
      if (group_set_button->value())
	newmode |= 02000;

      if (other_read_button->value())
	newmode |= 00004;
      if (other_write_button->value())
	newmode |= 00002;
      if (other_exec_button->value())
	newmode |= 00001;
      if (other_temp_button->value())
	newmode |= 01000;
    }
    else
      newmode = 0;

    // Then apply any changes...
    for (i = 1, file = dist_->files; i <= list->size(); i ++, file ++)
      if (list->selected(i))
      {
	if (count > 1)
	{
	  if (type)
	    file->type = types[type_chooser->value()];

          if (dst_path_field->value() &&
	      dst_path_field->value()[0])
	  {
	    // Remap destination directory...
	    if ((slash = strrchr(file->dst, '/')) != NULL)
	      slash ++;
	    else
	      slash = file->dst;

            snprintf(newdst, sizeof(newdst), "%s/%s",
	             dst_path_field->value(), slash);

            strncpy(file->dst, newdst, sizeof(file->dst) - 1);
	  }
	}
	else
	{
	  file->type = types[type_chooser->value()];

	  strncpy(file->dst, dst_path_field->value(),
	          sizeof(file->dst) - 1);
	  strncpy(file->src, src_path_field->value(),
	          sizeof(file->src) - 1);
	}

	if (upgrade_button->value())
	  file->type = toupper(file->type);
	else
	  file->type = tolower(file->type);

        if (newmode)
	  file->mode = newmode;

	strncpy(file->user, user_field->value(), sizeof(file->user) - 1);
	strncpy(file->group, group_field->value(), sizeof(file->group) - 1);

        if (subpackage_chooser->value())
          file->subpackage = dist_->subpackages[
	                         subpackage_chooser->value() - 1];
        else
	  file->subpackage = NULL;
      }

    file_window->hide();
    modified(1);
    update_list();
  }
  else
  {
    // Look through the selected files...
    type = '\0';

    for (i = 1, count = 0, file = dist_->files; i <= list->size(); i ++, file ++)
      if (list->selected(i))
      {
	count ++;

	if (count > 1)
	{
          if (type != tolower(file->type))
	  {
	    type_chooser->deactivate();
	    perm_group->deactivate();
	  }

	  src_path_field->deactivate();
	  src_path_field->value("");
	  dst_path_field->value("");
	}
	else
	{
	  type = tolower(file->type);

	  type_chooser->activate();

	  switch (type)
	  {
	    case 'f' :
		type_chooser->value(0);
		break;
	    case 'c' :
		type_chooser->value(1);
		break;
	    case 'm' :
		type_chooser->value(2);
		break;
	    case 'i' :
		type_chooser->value(3);
		break;
	    case 'd' :
		type_chooser->value(4);
		break;
	    case 'l' :
		type_chooser->value(5);
		break;
	    case 'r' :
		type_chooser->value(6);
		break;
	  }

          if (isupper(file->type))
	    upgrade_button->set();
	  else
	    upgrade_button->clear();

	  perm_group->activate();

          if (file->mode & 00400)
	    user_read_button->set();
	  else
	    user_read_button->clear();
          if (file->mode & 00200)
	    user_write_button->set();
	  else
	    user_write_button->clear();
          if (file->mode & 00100)
	    user_exec_button->set();
	  else
	    user_exec_button->clear();
          if (file->mode & 04000)
	    user_set_button->set();
	  else
	    user_set_button->clear();

          if (file->mode & 00040)
	    group_read_button->set();
	  else
	    group_read_button->clear();
          if (file->mode & 00020)
	    group_write_button->set();
	  else
	    group_write_button->clear();
          if (file->mode & 00010)
	    group_exec_button->set();
	  else
	    group_exec_button->clear();
          if (file->mode & 02000)
	    group_set_button->set();
	  else
	    group_set_button->clear();

          if (file->mode & 00004)
	    other_read_button->set();
	  else
	    other_read_button->clear();
          if (file->mode & 00002)
	    other_write_button->set();
	  else
	    other_write_button->clear();
          if (file->mode & 00001)
	    other_exec_button->set();
	  else
	    other_exec_button->clear();
          if (file->mode & 01000)
	    other_temp_button->set();
	  else
	    other_temp_button->clear();

	  user_field->value(file->user);
	  group_field->value(file->group);

	  src_path_field->activate();
	  src_path_field->value(file->src);
	  dst_path_field->value(file->dst);

          if (file->subpackage)
	  {
            for (j = 0; j < dist_->num_subpackages; j ++)
	      if (file->subpackage == dist_->subpackages[j])
	      {
	        subpackage_chooser->value(j + 1);
		break;
	      }
	  }
	  else
	    subpackage_chooser->value(0);
	}
      }

    file_ok_button->deactivate();
    file_window->hotspot(file_window);
    file_window->show();
  }

  printf("count = %d\n", count);
}


//
// 'ListEditor::help_cb()' - Help callback handler.
//

void
ListEditor::help_cb(const char *html)		// I - HTML file to show
{
}


//
// 'ListEditor::list_cb()' - List callback handler.
//

void
ListEditor::list_cb()				// I - List editor window
{
  int	i;					// Looping var


  if (Fl::event_clicks())
    file_settings_cb(0);
  else
  {
    file_settings_item->deactivate();

    for (i = 1; i <= list->size(); i ++)
      if (list->selected(i))
      {
        file_settings_item->activate();
        break;
      }
  }
}


//
// 'ListEditor::list_subpkg_cb()' - Chooser a subpackage in the project.
//

void
ListEditor::list_subpkg_cb()			// I - List editor window
{
}


//
// 'ListEditor::margins_cb()' - Margins callback handler.
//

void
ListEditor::margins_cb()			// I - List editor window
{
  int		i, j;				// Looping vars
  int		update;				// Update the list?
  char		name[32];			// Attribute name
  ListColumn	*c;				// Current column


  for (i = 0, j = 0, update = 0, c = margin_manager->columns();
       i < margin_manager->num_columns();
       i ++, c ++)
  {
    if (margin_items[i].value())
    {
      if (!c->width)
      {
	update   = 1;
	c->width = c->min_width;

	margin_manager->redraw();
      }

      margins_[j] = c->width;
      j ++;
    }
    else if (c->width)
    {
      update   = 1;
      c->width = 0;

      margin_manager->redraw();
    }

    sprintf(name, "margin%d", i);
    prefs_.set(name, c->width);
  }

  margins_[0] -= list->iconsize() + 9;
  margins_[j] = 0;
  list->column_widths(margins_);

  if (update)
    update_list();
  else
    list->redraw();
}


//
// 'ListEditor::new_cb()' - New callback handler.
//

void
ListEditor::new_cb()				// I - List editor window
{
  ListEditor	*temp;				// New list editor window


  if (dist_)
  {
    // Open the file in a new window...
    temp = new ListEditor(NULL);
    temp->show();
  }
  else
  {
    // Open it in this window...
    open(NULL);
  }
}


//
// 'ListEditor::open_cb()' - Open callback handler.
//

void
ListEditor::open_cb()				// I - List editor window
{
  const char	*listfile;			// New list file...
  ListEditor	*temp;				// New list editor window


  if ((listfile = fl_file_chooser("List File?", "*.list", filename_)) != NULL)
  {
    if (dist_)
    {
      // Open the file in a new window...
      temp = new ListEditor(listfile);
      temp->show();
    }
    else
    {
      // Open it in this window...
      open(listfile);
    }
  }
}


//
// 'ListEditor::open_history_cb()' - Open history callback handler.
//

void
ListEditor::open_history_cb(const char *listfile)// I - List file to load
{
  ListEditor	*temp;				// New list editor window


  if (dist_)
  {
    // Open the file in a new window...
    temp = new ListEditor(listfile);
    temp->show();
  }
  else
  {
    // Open it in this window...
    open(listfile);
  }
}


//
// 'ListEditor::paste_cb()' - Paste callback handler.
//

void
ListEditor::paste_cb()				// I - List editor window
{
}


//
// 'ListEditor::project_ok_cb()' - Save the project settings.
//

void
ListEditor::project_ok_cb()			// I - List editor window
{
}


//
// 'ListEditor::project_settings_cb()' - Project settings callback handler.
//

void
ListEditor::project_settings_cb()		// I - List editor window
{
  int	i;					// Looping var
  int	type;					// Dependency type


  if (dist_)
  {
    // General tab...
    name_field->value(dist_->product);
    version_field->value(dist_->version);
    version_counter->value(dist_->vernumber);
    copyright_field->value(dist_->copyright);
    vendor_field->value(dist_->vendor);
    packager_field->value(dist_->packager);
    license_field->value(dist_->license);
    readme_field->value(dist_->readme);

    // Packages tab...
    subpackage_list->clear();
    subpackage_list->add("(default)");

    for (i = 0; i < dist_->num_subpackages; i ++)
      subpackage_list->add(dist_->subpackages[i]);

    description_field->value("");
    for (i = 0; i < dist_->num_descriptions; i ++)
      if (dist_->descriptions[i].subpackage == NULL)
      {
        if (description_field->size())
	{
	  description_field->position(description_field->size());
	  description_field->insert("\n");
	}

	description_field->position(description_field->size());
	description_field->insert(dist_->descriptions[i].description);
      }

    for (type = 0; type < 4; type ++)
      depends_field[type]->value("");

    for (i = 0; i < dist_->num_depends; i ++)
      if (dist_->depends[i].subpackage == NULL)
      {
        type = dist_->depends[i].type;

        if (depends_field[type]->size())
	{
	  depends_field[type]->position(depends_field[type]->size());
	  depends_field[type]->insert(", ");
	}

	depends_field[type]->position(depends_field[type]->size());
	depends_field[type]->insert(dist_->depends[i].product);
      }

    // GUI Setup tab...
    setup_image_field->value("");
    setup_types_field->value("");
  }
  else
  {
    // General tab...
    name_field->value("");
    version_field->value("");
    version_counter->value(0);
    copyright_field->value("");
    vendor_field->value("");
    packager_field->value("");
    license_field->value("");
    readme_field->value("");

    // Packages tab...
    subpackage_list->clear();
    subpackage_list->add("(default)");

    description_field->value("");

    for (type = 0; type < 4; type ++)
      depends_field[type]->value("");

    // GUI Setup tab...
    setup_image_field->value("");
    setup_types_field->value("");
  }

  project_window->show();
}


//
// 'ListEditor::quit_cb()' - Quit callback handler.
//

void
ListEditor::quit_cb()				// I - List editor window
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
ListEditor::save_cb()				// I - List editor window
{
  if (write_dist(filename_, dist_))
    fl_alert("Unable to save \"%s\":\n\n%s", filename_, strerror(errno));
  else
    modified(0);
}


//
// 'ListEditor::save_as_cb()' - Save As callback handler.
//

void
ListEditor::save_as_cb()			// I - List editor window
{	
}


//
// End of "$Id: ListEditor3.cxx,v 1.3 2005/01/11 21:36:57 mike Exp $".
//
