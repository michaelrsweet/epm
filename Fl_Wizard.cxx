//
// "$Id: Fl_Wizard.cxx,v 1.2 2000/01/04 13:45:51 mike Exp $"
//
//   Fl_Wizard widget routines.
//
//   Copyright 1997-2000 by Easy Software Products.
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
// Contents:
//
//   Fl_Wizard::Fl_Wizard() - Create an Fl_Wizard widget.
//   Fl_Wizard::draw()      - Draw the wizard border and visible child.
//   Fl_Wizard::next()      - Show the next child.
//   Fl_Wizard::prev()      - Show the previous child.
//   Fl_Wizard::value()     - Return the current visible child.
//   Fl_Wizard::value()     - Set the visible child.
//

//
// Include necessary header files...
//

#include "Fl_Wizard.h"
#include <FL/fl_draw.H>


//
// 'Fl_Wizard::Fl_Wizard()' - Create an Fl_Wizard widget.
//

Fl_Wizard::Fl_Wizard(int        xx,	// I - Lefthand position
                     int        yy,	// I - Upper position
		     int        ww,	// I - Width
		     int        hh,	// I - Height
		     const char *l) :	// I - Label
    Fl_Group(xx, yy, ww, hh, l)
{
  box(FL_THIN_UP_BOX);

  value_ = (Fl_Widget *)0;
}


//
// 'Fl_Wizard::draw()' - Draw the wizard border and visible child.
//

void
Fl_Wizard::draw()
{
  Fl_Widget	*kid;	// Visible child


  kid = value();

  if (damage() & FL_DAMAGE_ALL)
  {
    // Redraw everything...
    if (kid)
    {
      draw_box(box(), x(), y(), w(), h(), kid->color());
      draw_child(*kid);
    }
    else
      draw_box(box(), x(), y(), w(), h(), color());

  }
  else if (kid)
    update_child(*kid);
}


//
// 'Fl_Wizard::next()' - Show the next child.
//

void
Fl_Wizard::next()
{
  int			num_kids;
  Fl_Widget	* const *kids;


  if ((num_kids = children()) == 0)
    return;

  for (kids = array(); num_kids > 0; kids ++, num_kids --)
    if ((*kids)->visible())
      break;

  if (num_kids > 1)
    value(kids[1]);
}


//
// 'Fl_Wizard::prev()' - Show the previous child.
//


void
Fl_Wizard::prev()
{
  int			num_kids;
  Fl_Widget	* const *kids;


  if ((num_kids = children()) == 0)
    return;

  for (kids = array(); num_kids > 0; kids ++, num_kids --)
    if ((*kids)->visible())
      break;

  if (num_kids > 0 && num_kids < (children() - 1))
    value(kids[-1]);
}


//
// 'Fl_Wizard::value()' - Return the current visible child.
//

Fl_Widget *
Fl_Wizard::value()
{
  int			num_kids;
  Fl_Widget	* const *kids;
  Fl_Widget		*kid;


  if ((num_kids = children()) == 0)
    return ((Fl_Widget *)0);

  for (kids = array(), kid = (Fl_Widget *)0; num_kids > 0; kids ++, num_kids --)
  {
    if ((*kids)->visible())
    {
      if (kid)
        (*kids)->hide();
      else
        kid = *kids;
    }
  }

  if (!kid)
  {
    kids --;
    kid = *kids;
    kid->show();
  }

  return (kid);
}


//
// 'Fl_Wizard::value()' - Set the visible child.
//

void
Fl_Wizard::value(Fl_Widget *kid)
{
  int			num_kids;
  Fl_Widget	* const *kids;


  if ((num_kids = children()) == 0)
    return;

  for (kids = array(); num_kids > 0; kids ++, num_kids --)
  {
    if (*kids == kid)
    {
      if (!kid->visible())
        kid->show();
    }
    else
      (*kids)->hide();
  }
}


//
// End of "$Id: Fl_Wizard.cxx,v 1.2 2000/01/04 13:45:51 mike Exp $".
//
