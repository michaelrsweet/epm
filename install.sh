#/bin/sh
#
# "$Id: install.sh,v 1.1 1999/05/20 14:18:59 mike Exp $"
#
#   Installation script for the Common UNIX Printing System.
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

#
# First see if we are using GNU tar, which has the unfortunate "feature"
# of stripping leading slashes...
#

if test "`tar --help 2>&1 | grep GNU`" = ""; then
	tar="tar xf -"
else
	tar="tar xzPf cups.tar.gz"
fi

#
# See if we are running as the root user...
#

#if test "`whoami`" != "root"; then
#	echo "Sorry, you must be root to install this software."
#	exit 1
#fi

#
# Then show the user what they are getting into...
#

echo "Welcome to the CUPS installation script."
echo ""
echo "This installation script will install the Common UNIX Printing System"
echo "version 1.0b2 on your system."
echo ""

yesno=""
while true ; do
	echo "Do you wish to continue? \c"
	read yesno
	case "$yesno" in
		y | yes | Y | Yes | YES)
		break
		;;
		n | no | N | No | NO)
		exit 0
		;;
		*)
		echo "Please enter yes or no."
		;;
	esac
done

echo "Installing software..."

