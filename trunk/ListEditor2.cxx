//
// "$Id: ListEditor2.cxx,v 1.2 2005/01/11 21:20:17 mike Exp $"
//
//   ESP List Editor file methods for the ESP Package Manager (EPM).
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
// Contents:
//
//

#include "ListEditor.h"
#include <FL/Fl_File_Icon.H>
#include <FL/fl_ask.H>


//
// Global window and history lists...
//

ListEditor	*ListEditor::first_ = (ListEditor *)0;
char		ListEditor::history_[10][1024] =
		{
		  "",
		  "",
		  "",
		  "",
		  "",
		  "",
		  "",
		  "",
		  "",
		  ""
		};
Fl_Preferences	ListEditor::prefs_(Fl_Preferences::USER, "easysw.com", "epmeditor");


//
// 'ListEditor::~ListEditor()' - Close a list editor window.
//

ListEditor::~ListEditor()
{
  ListEditor	*temp;				// Current list editor window...


  // First remove this window from the window list...
  if (first_ == this)
  {
    // Easy case - first window in the list, so just bump the window
    // pointer ahead...
    first_ = next_;
  }
  else
  {
    // OK, we need to remove the window from inside the list...
    for (temp = first_; temp; temp = temp->next_)
      if (temp->next_ == this)
        break;

    if (temp)
    {
      // The next window is this one; give the previous window the
      // next windows pointer...
      temp->next_ = temp->next_->next_;
    }
  }

  // Free all memory used...
  if (dist_) free_dist(dist_);

  delete window;
  delete file_window;
}


//
// 'ListEditor::check_save()' - See if we need to save the list file.
//

int						// O - 1 on success, 0 on failure
ListEditor::check_save()
{
  return (1);
}


//
// 'ListEditor::open()' - Open a list file.
//

int						// O - 1 on success, 0 on failure
ListEditor::open(const char *listfile)		// I - Name of distribution to load
{
  dist_t	*temp;				// Temporary distribution
  struct utsname platform;			// Platform information
  char		newfile[1024],			// New filename
		*slash;				// Final slash in filename


  if (listfile)
  {
    // Make a copy of the filename and chdir as necessary...
    strncpy(newfile, listfile, sizeof(newfile) - 1);
    newfile[sizeof(newfile) - 1] = '\0';

    if ((slash = strrchr(newfile, '/')) != NULL)
    {
      *slash++ = '\0';
      listfile = slash;
      chdir(newfile);
    }

    // Get the platform ID and load the distribution...
    get_platform(&platform);

    temp = read_dist(listfile, &platform, "");

    if (temp)
    {
      fl_filename_absolute(filename_, sizeof(filename_), listfile);

      sort_dist_files(temp);

      if (dist_)
        free_dist(dist_);

      dist_ = temp;
    }
    else
    {
      fl_alert("Unable to open \"%s\"!\n\n%s", listfile, strerror(errno));
      return (0);
    }
  }
  else
  {
    filename_[0] = '\0';
    dist_        = (dist_t *)0;
  }

  update_list();
  update_subpkgs();

  modified(0);

  return (1);
}


//
// 'ListEditor::save()' - Save a list file.
//

int						// O - 1 on success, 0 on failure
ListEditor::save(const char *filename)		// I - Name of distribution to save
{
  modified(0);
  return (1);
}


//
// 'ListEditor::set_title()' - Set the window title and icon.
//

void
ListEditor::set_title()
{
  const char	*f,				// Filename to show in titlebar
		*m;				// " (modified)" or ""


  // Get the filename
  if (filename_[0])
  {
    // Just show the root name...
    if ((f = strrchr(filename_, '/')) != NULL)
      f ++;
    else
      f = filename_;
  }
  else
  {
    // Show "(new project)"...
    f = "(new project)";
  }

  // Get the modified status...
  if (modified_)
    m = " (modified)";
  else
    m = "";

  // Format the title label...
  snprintf(title_, sizeof(title_), "%s%s - EPM List Editor 4.0", f, m);

  // Format the icon label...
  snprintf(icontitle_, sizeof(icontitle_), "%s%s", f, m);

  // Set the window labels...
  window->label(title_, icontitle_);
}


//
// 'ListEditor::update_history()' - Update the file history...
//

void
ListEditor::update_history(const char *listfile)// I - List file
{
}


//
// 'ListEditor::update_list()' - Update the file list.
//

void
ListEditor::update_list()
{
  int			i;			// Looping var
  file_t		*file;			// Current file
  char			text[2048],		// Text for line
			*textptr;		// Pointer into line
  Fl_File_Icon		*temp;			// Temporary icon
  static Fl_File_Icon	*file_icon = (Fl_File_Icon *)0,
			*exe_icon = (Fl_File_Icon *)0,
			*dir_icon = (Fl_File_Icon *)0,
			*link_icon = (Fl_File_Icon *)0;


  if (!file_icon)
  {
    Fl_File_Icon::load_system_icons();

    file_icon = Fl_File_Icon::find("foo", Fl_File_Icon::PLAIN);
    exe_icon  = Fl_File_Icon::find("foo.shell", Fl_File_Icon::PLAIN);
    dir_icon  = Fl_File_Icon::find("foo", Fl_File_Icon::DIRECTORY);
    link_icon = Fl_File_Icon::find("foo", Fl_File_Icon::LINK);

    if ((temp = Fl_File_Icon::find("foo.man", Fl_File_Icon::PLAIN)) != NULL)
    {
      new Fl_File_Icon("{*.[012345678z]|*.1m}", Fl_File_Icon::PLAIN,
                       temp->size(), temp->value());
    }
    else
    {
      static const char *types[] =
      {
        "ANY",
	"PLAIN",
	"FIFO",
	"DEVICE",
	"LINK",
	"DIRECTORY"
      };

      for (temp = Fl_File_Icon::first(); temp; temp = temp->next())
        printf("%p: pattern=\"%s\", type = %s\n", temp, temp->pattern(),
	       types[temp->type()]);
    }
  }

  list->clear();
  list->topline(1);

  margins_cb();

  if (dist_)
  {
    for (i = dist_->num_files, file = dist_->files; i > 0; i --, file ++)
    {
      textptr = text;

      if (margin_items[0].value())
      {
        snprintf(textptr, sizeof(text) - (textptr - text), "%s%04o",
	         textptr != text ? "\t" : "", file->mode);
        textptr += strlen(textptr);
      }

      if (margin_items[1].value())
      {
        snprintf(textptr, sizeof(text) - (textptr - text), "%s%s",
	         textptr != text ? "\t" : "", file->user);
        textptr += strlen(textptr);
      }


      if (margin_items[2].value())
      {
        snprintf(textptr, sizeof(text) - (textptr - text), "%s%s",
	         textptr != text ? "\t" : "", file->group);
        textptr += strlen(textptr);
      }

      if (margin_items[3].value())
      {
        snprintf(textptr, sizeof(text) - (textptr - text), "%s%s",
	         textptr != text ? "\t" : "", file->dst);
        textptr += strlen(textptr);
      }

      if (margin_items[4].value())
      {
        snprintf(textptr, sizeof(text) - (textptr - text), "%s%s",
	         textptr != text ? "\t" : "", file->src);
        textptr += strlen(textptr);
      }

      if (margin_items[5].value())
      {
        snprintf(textptr, sizeof(text) - (textptr - text), "%s%s",
	         textptr != text ? "\t" : "",
	         file->subpackage ? file->subpackage : "(default)");
        textptr += strlen(textptr);
      }

      switch (file->type)
      {
	default :
	    temp = Fl_File_Icon::find(file->dst, Fl_File_Icon::PLAIN);

	    if (temp && (!(file->mode & 111) || temp != file_icon))
              list->add(text, temp);
	    else if (file->mode & 0111)
              list->add(text, exe_icon);
	    else
              list->add(text, file_icon);
	    break;
	case 'd' :
            list->add(text, dir_icon);
	    break;
	case 'l' :
            list->add(text, link_icon);
	    break;
      }
    }
  }
}


//
// 'ListEditor::update_margins()' - Update the margin settings...
//

void
ListEditor::update_margins()
{
  int			i;			// Looping var
  int			w;			// Column width
  char			name[32];		// Attribute name
  static const char	*labels[] =		// Labels for columns
			{
			  "Mode",
			  "User",
			  "Group",
			  "Destination",
			  "Source",
			  "Subpackage"
			};


  if (margin_manager->num_columns() == 0)
  {
    // Initialize the margin list manager widget...
    for (i = 0; i < 6; i ++)
    {
      sprintf(name, "margin%d", i);
      prefs_.get(name, w, 50);

      margin_manager->add(labels[i], w);

      if (w)
        margin_items[i].set();
      else
        margin_items[i].clear();
    }
  }
}


//
// 'ListEditor::update_subpkgs()' - Update subpackages...
//

void
ListEditor::update_subpkgs()
{
  int	i;					// Looping var


  subpackage_chooser->clear();
  subpackage_chooser->add("(default)");

  if (dist_)
  {
    for (i = 0; i < dist_->num_subpackages; i ++)
      subpackage_chooser->add(dist_->subpackages[i]);
  }
}


//
// End of "$Id: ListEditor2.cxx,v 1.2 2005/01/11 21:20:17 mike Exp $".
//
