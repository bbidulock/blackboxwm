ChangeLog for 0.51.x

Changes from 0.50.5 to 0.51.0:
  - new default theme, shows off new gradients (see below)
  - many themes updated to show off new menu bullet configuration
  - added new source file Display.cc... it offers an easy way to connect to
    an X display and setup screen info, this was done to make life easier
    for John Kennis, the author of the bbtools.  Image.cc and Image.hh have
    been modified to use classes from this abstraction, so that drop in
    replacements are all that is necessary to update the bbtools image code.
  - configurable menu bullet... 2 new resources for in your style file:

        menu.bulletStyle:    (round|triangle|square|diamond|empty)
        menu.bulletPosition: (left|right)

  - new style resource for setting the borderWidth on menus, client windows and
    the buttons/frame/handle/titlebar... the default theme uses a borderWidth
    of zero... it's pretty neat
  - udpated Image code... blackbox now supports 8 types of gradients (thanks
    to mosfet@kde.org... in exchange for helping him get the diagonal gradient
    code from blackbox into kde, kde gave me the source to their new gradients)
    the 8 gradients are:

        diagonal, vertical, horizontal, crossdiagonal, pipecross, elliptic,
        pyramid, rectangle

    use them just like you would normally (ie. raised elliptic gradient bevel1)
  - merged John Kennis' patch for notifying KDE modules of windows that are
    raised/lowered/activated(focused)
  - new geometry window that is displayed when a window is moved/resized
  - cleaned up code for detecting slit apps
  - window stacking code changed to keep menus above windows, and to keep the
    slit raised when the toolbar is raised
  - fixed compiler error from gcc 2.95 about frame.frame in several places
  - fixed some bugs with shaped windows that set decorations via MWM hints,
    and also fixed bugs with such windows changing their shape
  - more complete ICCCM compliance, default window gravity is now NorthWest
    instead of Static...
  - focus code revamped... window focusing is alot faster and simpler, i
    mimicked the way TWM does it's focusing... proved much faster
  - the window menu always has "Kill Client" as an option now
  - fixed window stacking for windows that have multiple transients (like
    netscape)
  - smartplacement from 0.50.4 has been reinstated... i quickly grew tired of
    waiting on windows to be placed with the old version (if you like the way
    0.50.5 did it... send me an email and i'll consider making it an option)
  - added some new signal handling code (using sigaction, if available on your
    system)...
  - fixed some bugs with KDE support... this makes bbpager behave properly
  - workspace editing via the toolbar has been made a little nicer with the
    new focus code... right clicking on the workspace label will put you into
    edit mode, but no windows can be focused until you leave edit mode...
    ALSO... the window that had focus when you entered edit mode will have the
    focus returned to it after editing is finished
  - added new option to blackbox... -rc <filename> will read <filename> instead
    of .blackboxrc for it's base configuration
  - the option for opaque window moves has been moved from the stylefile into
    .blackboxrc... set the session.opaqueMove: resource to True or False,
    depending on what you want
  - general namespace cleanups... just stuff to make maintaining the code a
    little easier...
  - any form of "beta" has been removed from the version number for 0.51.x
    i am declaring this series as "stable" so that i can begin a major overhaul
    of blackbox, which will be done with John Kennis and Jason Kasper, two
    very sharp guys that think the same way i do ;)

