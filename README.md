[blackbox -- read me first file.  2018-05-28]: #

blackbox
===============

Package `blackbox-0.73` was released under MIT license 2018-05-28.

This is a fork of the original blackbox CVS that is on [Sourceforge][12].
This fork is hosted on [GitHub][2].  What it includes is all changes made
on the official blackbox CVS repository on branch `blackbox-0_70_2cvs`, as
well as patches collected from pdl-linux, Debian, the blackbox bug list,
other GitHub forks.  It also includes enhanced EWMH/ICCCM compliance.


Release
-------

This is the `blackbox-0.73` package, released 2018-05-28.  This release, and
the latest version, can be obtained from [GitHub][1], using a command such as:

    $> git clone https://github.com/bbidulock/blackboxwm.git

Please see the [NEWS][3] file for release notes and history of user visible
changes for the current version, and the [ChangeLog][4] file for a more
detailed history of implementation changes.  The [TODO][5] file lists features
not yet implemented and other outstanding items.

The file [COMPLIANCE][6] lists the current state of EWMH/ICCCM compliance.

Please see the [INSTALL][7] file for installation instructions.

When working from `git(1)`, please use this file.  An abbreviated
installation procedure that works for most applications appears below.

This release is published under the MIT/X Consortium license that can be found
in the file [LICENSE][8].
This release is published under GPLv3.  Please see the license
in the file [COPYING][9].


Quick Start
-----------

The quickest and easiest way to get blackbox up and running is to run the
following commands:

    $> git clone https://github.com/bbidulock/blackboxwm.git blackbox
    $> cd blackbox
    $> ./autogen.sh
    $> ./configure
    $> make
    $> make DESTDIR="$pkgdir" install

This will configure, compile and install blackbox the quickest.  For those who
like to spend the extra 15 seconds reading `./configure --help`, some compile
time options can be turned on and off before the build.

For general information on GNU's `./configure`, see the file [INSTALL][7].


Configuring blackbox
--------------------

The next thing most users want to do after installing blackbox is to
configure the colors, fonts, menus, etc. to their liking.  This is covered
by the files [README][9], [README.menu][10] and [README.style][11].  These
files give detailed information on how to customize your new window
manager.


Included utilities
------------------

Currently, the only included utilities are a program named `bsetroot(1)`
and a script called `bsetbg(1)`. `bsetroot` is a replacement for
`xsetroot(1)`, minus a few options.  The difference between `xsetroot(1)`
and `bsetroot` is that `bsetroot` has been coded for multiple screens (e.g.
multi-headed displays), where as the stock `xsetroot(1)` is not. The
`bsetbg` script acts as a wrapper for most of the popular programs used to
set background pixmaps, making it possible for styles to provide a
machine-independent `rootCommand`.


Third-party utilities
---------------------

With the start of the `0.60.x` series blackbox no longer handles any
keyboard shortcuts; instead it supports a communication protocol which
allows other programs to handle these and related tasks. If you'd like to
be able to use keyboard shortcuts with blackbox, [`bbkeys(1)`][13] can
provide you with all the previous functionality and more.

If you're looking for a GUI with which to configure your blackbox menu
and/or styles, check out [`bbconf(1)`][14].  `bbconf(1)` is a QT program
that does just that, as well as providing a GUI for editing your key
bindings for the above mentioned `bbkeys(1)`.


Running
-------

Read the manual page after installation:

    man blackbox


Issues
------

Report issues on GitHub [here][2].



[1]: https://github.com/bbidulock/blackboxwm
[2]: https://github.com/bbidulock/blackboxwm/issues
[3]: https://github.com/bbidulock/blackboxwm/blob/0.73/NEWS
[4]: https://github.com/bbidulock/blackboxwm/blob/0.73/ChangeLog
[5]: https://github.com/bbidulock/blackboxwm/blob/0.73/TODO
[6]: https://github.com/bbidulock/blackboxwm/blob/0.73/COMPLIANCE
[7]: https://github.com/bbidulock/blackboxwm/blob/0.73/INSTALL
[8]: https://github.com/bbidulock/blackboxwm/blob/0.73/LICENSE
[9]: https://github.com/bbidulock/blackboxwm/blob/0.73/COPYING
[9]: https://github.com/bbidulock/blackboxwm/blob/0.73/data/README
[10]: https://github.com/bbidulock/blackboxwm/blob/0.73/data/README.menu
[11]: https://github.com/bbidulock/blackboxwm/blob/0.73/data/README.style
[12]: http://blackboxwm.sourceforge.net
[13]: http://bbkeys.sourceforge.net
[14]: http://bbconf.sourceforge.net

[ vim: set ft=markdown sw=4 tw=80 nocin nosi fo+=tcqlorn spell: ]: #
