#
# "$Id: Makefile,v 1.1 1999/05/20 14:18:58 mike Exp $"
#
#   Packaging makefile for the Common UNIX Printing System (CUPS).
#
#   Copyright 1997-1999 by Easy Software Products, all rights reserved.
#
#   These coded instructions, statements, and computer programs are the
#   property of Easy Software Products and are protected by Federal
#   copyright law.  Distribution and use rights are outlined in the file
#   "LICENSE.txt" which should have been included with this file.  If this
#   file is missing or damaged please contact Easy Software Products
#   at:
#
#       Attn: CUPS Licensing Information
#       Easy Software Products
#       44145 Airport View Drive, Suite 204
#       Hollywood, Maryland 20636-3111 USA
#
#       Voice: (301) 373-9603
#       EMail: cups-info@cups.org
#         WWW: http://www.cups.org
#

include ../Makedefs

TARGETS	=	makedist
OBJS	=	makedist.o

#
# Make all targets...
#

all:	$(TARGETS)

#
# Clean all object files...
#

clean:
	rm -f $(OBJS) $(TARGETS)

#
# Install all targets...
#

install:
	(cd ..; packages/makedist packages/cups.list packages/cups.tar.gz)

#
# makedist
#

makedist:	makedist.o
	$(CC) $(LDFLAGS) -o makedist makedist.o

makedist.o:	../Makedefs

#
# End of "$Id: Makefile,v 1.1 1999/05/20 14:18:58 mike Exp $".
#
