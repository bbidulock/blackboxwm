ChangeLog for 0.62.x

Changes from 0.62.0 to 0.62.1:
  - the lock modifier code handles user redefined modifiers better
  - check if the locale actually needs multibyte support before using multibyte
    functions
  - use srcdir in all of the makefiles
  - added zh_CN (Chinese) nls support

Changes from 0.61.1 to 0.62.0:
  - the immorel release
  - added the ja_JP nls directory and man pages
  - general code touchups
  - blackbox-nls.hh is always generated even if --disable-nls is used.
    This allows us to not have all of those hideous #ifdef NLS chunks.
    Nothing to worry about, if you do not want NLS this does not affect you
  - Workspace::placeWindow() cleanups.  Also a speed bump from reducing the
    use of iterator->current() and changing the delta from 1 to 8
  - cleanups to compile with g++ 3.0
  - make distclean actually removes Translation.m and blackbox-nls.hh.
    Also fixed Makefile.am to pass --foreign instead of --gnu when calling
    the autotools.
  - fixed a desciptor leak in BScreen::parseMenuFile, seems opendir
    lacked a matching closedir.
  - fix transient window handling code in Workspace::removeWindow() so
    transients give focus back to their parents properly.  The code originally
    handled sloppy focus then transient windows, so we just flopped the
    if/elsif.  This is immediately noticable with web browsers and their open
    location windows.
  - plugged a small leak in ~Toolbar
  - fixed list::insert so you really can insert at item number 2.  While there
    I cleaned up the code a bit.
  - added decoration to the atom state stored in a window
  - fixed a typo in bsetroot.cc: 'on of' -> 'one of'.
  - fixed the window menu gets left open when another window button is pressed
    issue with a call to windowmenu->hide() in window->maximize()
  - applied xOr's patch for decoration handling
  - applied xOr's patch for the maximize, shade, unmaximize bug
  - applied Kennis' patch for sending incorrect Slit configure notices
  - BlackboxWindow's flags have been moved into a flags structure
  - applied xOr's patch for border handling
  - resizing a window turns off its maximized flag.  Before a resized window
    thought it was still maximized and maximizing a double action
  - BlackboxWindow::withdraw no longet sets the state to Withdrawn.
    This confused some X clients.
  - updated the manpages and added Dutch NLS support (thanks Wilbert)
  - added it_IT nls files, thanks Luca Marrazzo <marra.luca@libero.it>
  - the menu file mentioned in the manpage is now based on DEFAULT_MENU
  - configure script found basename in -lgen, but did not set HAVE_BASENAME
    causing compilation problems on irix and possibly others.  Added a call
    to AC_DEFINE in AC_CHECK_LIB to fix this.
  - menu is no longer installed, you need to copy it yourself
  - cleaned up i18n code a little.  Several member functions were declared
    but never used and getMessage() had a default argument which was also
    never used.
  - i18n will now compile cleanly on machines without nl_types.h
  - the lock modifiers no longer stop blackbox!
  - maximize a window via bbkeys and the maximize button is not redrawn, fixed
  - now exit with an error code if an unknown option is passed
  - autoraise and multiple dialog windows yields segv bug fixed
    also lengthened the default auto raise delay from 250 to 400
  - another iteration of autoraise and dialog box handling, this time we
    noticed that nothing ever reset blackbox.focused_window to 0 when a window
    was removed
  - check if the window is visible before changeBlackboxHints() calls maximize
  - placeWindow no longer takes edgeSnapThreshhold into account
  - ignore style files ending in ~
  - support locale specifiers with @euro in them
  - added Slovenian man pages and nls, thanks Ales Kosir
  - Toolbar name editing buffer reduced to 128 chars, logic added to make sure
    this buffer is not overrun
  - added German nls files, thanks Jan Schaumann
  - added my name to the code, updated the version output
    
Changes from 0.61.0 to 0.61.1:
  - fixed some of the code to explicitly use colormaps so that when
    blackbox decides to use a non-default visual everything will
    still work (although it may look darn ugly)
  - optimizations to the deiconify/raising code to (hopefully) deal
    with a rather nasty bug, plus make things a little more efficient
  - changed the code so that the close button is always redrawn on
    button release events, just in case the client decides not to
    close in response to the message (see Acroread)
  - tinkered with the Makefiles again to make sure Blackbox
    completely cleans up after itself during an uninstall
  - fixed a glitch in window placement that was making Blackbox
    place some larger windows at coordinates near 2**31
  - merged in a patch from nyz which fixed a bug with not sending
    configure events when a window is both moved and resized (eg
    when the left resize grip is used) as well as optimized some
    of the show/hide code to use the stacking order
  - fixed a bug in blackbox's support of the X shape extension...
    it wasn't correctly resetting the bounding region after a window
    was resized
  - fixed a glitch with the geometry window where it would persist
    if the client was unmapped while in motion
  - tweaked the code for decorating transient windows so that it
    is possible to use MOD1+Mouse3 to resize transients as long as
    there is not some other reason to disable functions.resize


Changes from 0.60.3 to 0.61.0:
  - added slightly updated copies of the blackbox/bsetroot manpages.
  - reworked the Windowmenu code so that using the second mouse
    button on the Send To menu moves you along with the window
  - merged in bsd-snprintf.(h|c) from openssh so that Blackbox can
    compile on older boxes without (v)snprintf in their standard lib.
  - fixed a pair of problems where blackbox was not returning icons
    and slit apps to a useable state at shutdown
  - fixed a problem with menus not getting layered correctly after
    a reconfigure or menu reload
  - changed the behavior of the various MOD1+ButtonPresses on windows...
    they should now be more consistent with the button behavior on the
    decorations :
            . MOD1+Button1 raises and moves the window (unchanged)
	    . MOD1+Button2 lowers the window (used to resize the window)
	    . MOD1+Button3 resizes the window (new button combo)
  - fixed a small but _extremely_ annoying bug exposed by cvsup
  - styled frames are now a thing of the past... the textures formerly
    known as window.frame.(un)focus have been replaced by solid colors
    window.frame.(un)focusColor... the thickness of the frame is now
    determined by frameWidth, which will default to bevelWidth if not
    specified
  - middle clicking on a window in a workspace's window list now moves
    the window to the current workspace
  - fixed a minor glitch with the appearance of window labels for
    certain newly-started apps (i.e. rxvt)
  - added a new configure option for both the toolbar and the slit --
    autohide. Hopefully this should help quell the demands for the
    removal of the toolbar...
  - added code to better handle apps that change the window focus
  - changed the command execution code (used to handle rootCommands
    and executable menu items) to be more robust... compound commands
    should now work
  - a new-and-slightly-improved implementation of unstyled frames should
    mean slightly better performance than previously
  - fixed a couple of stupid bugs in the new code for handling
    Solid Flat textures more efficiently
  - fixed the nls makefiles so that they respect DESTDIR, behave better
    if you reinstall over an existing installation, and actually remove
    their files on a make uninstall
  - added cthulhain's bsetbg script to the util directory... see the
    file README.bsetbg for more information
  - added Estonian, French and Danish translations

