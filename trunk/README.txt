README - CUPS v1.0b2
--------------------

BETA SOFTWARE BETA SOFTWARE BETA SOFTWARE BETA SOFTWARE BETA SOFTWARE

    WARNING - This is a BETA release of CUPS, which means that it
              may contain "bugs" that could prevent you from
	      printing. If you are concerned that this may cause
	      you lost time or money, please STOP and do not install
	      this software!

BETA SOFTWARE BETA SOFTWARE BETA SOFTWARE BETA SOFTWARE BETA SOFTWARE


INTRODUCTION

The Common UNIX Printing System provides a portable printing layer for
UNIX® operating systems. It has been developed by Easy Software
Products to promote a standard printing solution for all UNIX vendors
and users. CUPS provides the System V and Berkeley command-line
interfaces.

CUPS uses the Internet Printing Protocol (IETF-IPP) as the basis for
managing print jobs and queues. The Line Printer Daemon (LPD, RFC1179),
Server Message Block (SMB), and AppSocket protocols are also supported
with reduced functionality.

CUPS adds network printer browsing and PostScript Printer Description
("PPD")-based printing options to support real world applications under
UNIX.

CUPS also includes a customized version of GNU GhostScript (currently
based off GNU GhostScript 4.03) and an image file RIP that can be used
to support non-PostScript printers.

CUPS is Copyright 1993-1999 by Easy Software Products, All Rights
Reserved. CUPS is currently licensed under the terms of the Aladdin
Free Public License. Please see the file "cups.license" for details.


SYSTEM REQUIREMENTS

This binary distribution requires a minimum of 10MB of free disk space.
We do not recommend using CUPS on a workstation with less than 32MB of
RAM or a PC with less than 16MB of RAM.


SOFTWARE REQUIREMENTS

The following operating system software is required to install this
software:

    - Digital UNIX (aka OSF1 aka Compaq Tru64 UNIX) 4.0 or higher
    - HP-UX 10.20 or higher
    - IRIX 6.5.x
    - Linux 2.0.36 with glibc2 or higher (tested with RedHat 5.2)
    - Solaris 2.5 or higher (SPARC or Intel)


INSTALLING CUPS

We are currently distributing CUPS binary distributions in TAR format
with installation and removal scripts.

WARNING: Installing CUPS will overwrite your existing printing system.
If you experience difficulties with the CUPS software and need to go
back to your old printing system, you will need to remove the CUPS
software with the provided script and reinstall the printing system
from your operating system CDs.

To install the CUPS software you will need to be logged in as root
(doing an "su" is good enough). Once you are the root user, run the
installation script with:

    ./cups.install ENTER

After asking you a few yes/no questions the CUPS software will be
installed and the scheduler will be started.


SETTING UP PRINTER QUEUES

CUPS works best with PPD (PostScript Printer Description) files. In a
pinch you can also use System V style printer interface scripts.

Two sample PPD files are provided with this distribution that utilize the
PostScript and image file RIPs and the sample HP printer driver. To add
the sample DeskJet driver to the system for a printer connected to the
parallel port, use one of the following commands:

    Digital UNIX:

        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/lp0

    HP-UX:

        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/c2t0d0_lp

    IRIX:

        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/plp

    Linux:

        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/par0
        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/par1
        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/par2

    Solaris:

        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/bpp0
        /usr/lib/lpadmin -p DeskJet -m deskjet.ppd -v parallel:/dev/ecpp0

Similarly, for the sample LaserJet driver you can use "LaserJet" and
"laserjet.ppd". Once you have run the lpadmin command you need to enable
the printer with:

    /usr/lib/accept DeskJet
    enable DeskJet

For other printers and interfaces see the CUPS System Administator's
Manual included with this software.


PRINTING FILES

CUPS provides both the System V "lp" and Berkeley "lpr" commands for
printing:

    lp filename
    lpr filename

The "lp" command is the preferred way of submitting files for printing
since you can pass options to the driver:

    lp -oPageSize=A4 -oResolution=600dpi filename

CUPS recognizes many types of images files as well as PostScript, HP-GL/2,
and text files, so you can print those files directly rather than through
an application.

If you have an application that generates output specifically for your
printer then you need to use the "-oraw" or "-l" options:

    lp -oraw filename
    lpr -l filename

This will prevent the filters from corrupting your print file.


KNOWN PROBLEMS

The following known problems are being worked on and should be resolved for
the third beta release of CUPS:

   - Documentation is not completed.
   - The lpq command is currently not provided.
   - The lpc command currently only supports the help and status commands.
   - Automatic classing is currently not supported.
   - The CUPS server should disable core dumps by filters, backends, and CGI
     programs.
   - The CUPS server should increase the FD limit to the maximum allowed on
     the system.
   - The CUPS server should close stdin, stdout, and stderr and run in the
     background ("daemon" mode...)
   - The class and job CGIs are currently not provided.

CUPS has been built and tested on the following operating systems:

   - Digital UNIX 4.0d
   - HP-UX 10.20 and 11.0
   - IRIX 5.3, 6.2, 6.5.3
   - Linux (RedHat 5.2)
   - Solaris 2.5.1, 2.6, 2.7 (aka 7)


REPORTING PROBLEMS

If you have problems, please send an email to cups-support@cups.org. Include
your operating system and version, compiler and version, and any errors or
problems you've run into.


OTHER RESOURCES

See the CUPS web site at "http://www.cups.org" for other site links.

You can subscribe to the CUPS mailing list by sending a message containing
"subscribe cups" to majordomo@cups.org. This list is provided to discuss
problems, questions, and improvements to the CUPS software. New releases of
CUPS are announced to this list as well.


LEGAL STUFF

CUPS is Copyright 1993-1999 by Easy Software Products. CUPS, the CUPS logo,
and the Common UNIX Printing System are the trademark property of Easy
Software Products.

CUPS is provided under the terms of the Aladdin Free Public License which is
located in the files "LICENSE.html" and "LICENSE.txt". For commercial
licensing information, please contact:

     Attn: CUPS Licensing Information
     Easy Software Products
     44141 Airport View Drive, Suite 204
     Hollywood, Maryland 20636-3111 USA

     Voice: +1.301.373.9603
     Email: cups-info@cups.org
     WWW: http://www.cups.org

If you're interested in a complete, commercial printing solution for UNIX,
check out our ESP Print software at http://www.easysw.com/print.html.
