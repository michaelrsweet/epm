How to Install EPM
==================

> Note: Complete installation instructions can be found in the file
> `doc/epm-book.html`.


What is Required for EPM?
-------------------------

On your development system you just need a C compiler, a make program, a POSIX
shell (Bourne, Korn, Bash, etc.), and gzip.

The graphical setup program needs a C++ compiler and the FLTK library, version
1.1.x or 1.3.x, available at <http://www.fltk.org>.

EPM can generate so-called "portable" distributions that are based on shell
scripts and tar files.  For these types of distributions your customers/users
will need a POXIX shell (Bourne, Korn, Bash, etc.), a tar program, and gzip.
The first two are standard items, and gzip is being shipped by most vendors as
well.

EPM can also generate vendor-specific distributions.  These require the
particular vendor tool (rpm, dpkg, etc.) to load the software.


How Do I Compile EPM?
---------------------

EPM uses GNU autoconf to configure itself for your system.  To build it, use:

    ./configure ENTER
    make ENTER

The default installation prefix is `/usr/local`; if you want to put EPM in a
different location, use the `--prefix` option to the configure script:

    ./configure --prefix=/path/to/use ENTER
    make ENTER

Once EPM is compiled you can type:

    sudo make install ENTER

to install the software.
