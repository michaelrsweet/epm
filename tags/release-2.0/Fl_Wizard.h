//
// "$Id: Fl_Wizard.h,v 1.2 2000/01/04 13:45:51 mike Exp $"
//
//   Fl_Wizard widget definitions.
//
//   Copyright 1999-2000 by Easy Software Products.
//
//   These coded instructions, statements, and computer programs are the
//   property of Easy Software Products and are protected by Federal
//   copyright law.  Distribution and use rights are outlined in the file
//   "COPYING" which should have been included with this file.  If this
//   file is missing or damaged please contact Easy Software Products
//   at:
//
//       Attn: ESP Licensing Information
//       Easy Software Products
//       44141 Airport View Drive, Suite 204
//       Hollywood, Maryland 20636-3111 USA
//
//       Voice: (301) 373-9600
//       EMail: info@easysw.com
//         WWW: http://www.easysw.com
//

//
// Include necessary header files...
//

#ifndef _FL_WIZARD_H_
#  define _FL_WIZARD_H_

#  include <FL/Fl_Group.H>


//
// FileBrowser class...
//

class Fl_Wizard : public Fl_Group
{
  Fl_Widget *value_;

  void draw();

  public:

  Fl_Wizard(int, int, int, int, const char * = 0);

  void		next();
  void		prev();
  Fl_Widget	*value();
  void		value(Fl_Widget *);
};

#endif // !_FL_WIZARD_H_

//
// End of "$Id: Fl_Wizard.h,v 1.2 2000/01/04 13:45:51 mike Exp $".
//
