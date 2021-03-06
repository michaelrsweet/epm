ESP Package Manager (EPM)
=========================

What is EPM?
------------

EPM is a simple cross-platform tool that generates software and patch
distributions in various formats from a list of files.  Supported formats
include:

- BSD packages ("pkg_create")
- Debian Package Manager ("dpkg")
- macOS software packages ("name.pkg")
- Portable (installation and removal scripts with tar files)
- Red Hat Package Manager ("rpm")

EPM also includes graphical "setup" and "uninstall" programs that can be
provided with your distributions to make installation and removal of more than
one package a snap.  The installers can be customized with product logos,
"readme" files, and click-wrap licenses as desired.

EPM is provided as free software under version 2 of the GNU General Public
license.

> Note: Future versions of this software will be maintained and developed by
> [Jim Jagielski](https://github.com/jimjag).


How Do I Compile EPM?
---------------------

See the file `INSTALL.md` for more info on that.


How Do I Use EPM?
-----------------

Please look at the EPM manual.  A preformatted copy is included with the source
archive in the file `doc/epm-book.html`.

An example EPM software list file is provided with this distribution in the
file `epm.list`.


Do I Have to Pay to Distribute Software Using EPM?
--------------------------------------------------

No!  EPM is free software and any installations you create are unencumbered by
licensing of any kind, not even the GPL.


What's New in EPM?
------------------

See the file `CHANGES.md` for change information.


Resources
---------

The official home page for EPM is <https://jimjag.github.io/epm/>.

Report all problems and submit all patches/pull requests using the Github issue
tracking pages at <https://github.com/jimjag/epm/issues>.


Legal Stuff
-----------

EPM is Copyright © 1999-2020 by Michael R Sweet. All rights reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
