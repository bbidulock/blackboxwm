ChangeLog for 0.60.x

Changes from 0.62.1 to 0.65.0:
  - added Taiwan Chinese (zh_TW), Hungarian (hu_HU), Latvian (lv_LV),
    Korean (ko_KR), Norwegian (no_NO), Polish (pl_PL), Romanian (ro_RO) and
    Ukrainian (uk_UA) nls files and updated most of the others.
  - removed the Estonian (ee_ET) and Turkish (tr_TR) locales due to their
    being heavily out of date and unmaintained
  - remove slit and netwm as compile time options
  - strip much of BaseDisplay's original functionality and move it to the
    blackbox class.
  - huge amounts of internal cleanups
  - added emacs local variables to each file that prevent the addition of tabs
  - added a Util.cc file which contains useful functions with no obvious home.
  - move code over to the STL
  - removed several unused variables and otherwise reduced the memory usage
    of the objects in Blackbox.  For the record the binary is roughly 100k
    larger than 0.62.0 and that is mostly due to the STL but there is also a
    fair bit of new code.  However for the most part blackbox runs faster and
    is still one of the leanest window managers out there today.
  - bsetroot now sets _XROOTPMAP_ID, so pseudo transparent apps will be happy
  - beginnings of a strut implementation.  toolbar and slit are removed from
    the available screen area if 'full maximize' is not set
  - XReparentWindow sends an UnmapNotify to the window manager however
    in certain cases the window is already unmapped so the window manager
    never gets the event and the unmapNotify event is where reparentNotify was
    handled.  Added a reparentNotifyEvent handler in the BlackboxWindow class
    and a new case in the Blackbox class's process_event function.
  - no more blackbox->grab/ungrab calls everywhere
  - compression of motion and expose
  - Now we have one function which turns ~/ into /home/user/.  This is now
    called everywhere this expansion should be done.  Even added this to the
    resource.menu_file so now the menu file may be specified as
    ~/blackbox_menu.
  - added a TimerQueue which is a priority_queue with the ability to release
    items it contains before they reach the top of the queue.  Also added a
    TimerQueueManager protocol class which BaseDisplay now inherits from.
  - BTimer now defaults to NOT recurring.  Most of the timers in blackbox were
    one shots so I saw little benefit in defaulting to repeating timers.
  - update transient handling, should solve issues with apps like acroread.
    added a getTransientInfo() method of the BlackboxWindow class which
    handles checking the transient state in X and setting the appropriate
    variables on the window.  To attack the infinite loops this
    function ensures that client.transient != this and we check for loops
    of the form A -> B -> C -> A.  The new transient code also allows for one
    window to have multiple transients so applications like xmms and web
    browsers are better behaved.
  - even better ICCCM support and focus handling
  - wmswallow works
  - fix for clock clipping in the toolbar
  - better support for non decorated windows and toggling decor
  - the geometry window shown when moving or resizing a window now handles the
    parentrelative setting better.  parentrelative support has been improved
    for all of the other widgets as well.
  - better window group handling
  - improved edge snap support (still no window to window snapping)
  - changing preferences no longer leads to windows being raised
  - the window's "send to" menu ignores the current workspace, which is a
    better UI approach
  - new placeWindow algorithm.  Blows the old one out of the water.  Not only
    is it faster but it is also cleaner code too (-:  Went from number 5 in
    the profiling results to under 30.  Image rendering is now the slowest
    part of managing of new windows.
    Because of the new code layout, support is now there for new and
    different layout options but this will wait for after 0.65.0.
  - smart window placement ignores shaded windows now
  - new option in the Config menu which allows Scroll Lock to disable
    Blackbox's keybindings.

Changes from 0.62.0 to 0.62.1:
  - the lock modifier code handles user redefined modifiers better
  - check if the locale actually needs multibyte support before using
    multibyte functions
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

Changes from 0.60.2 to 0.60.3:
  - put in a (temporary?) fix for a bug with the new way icons are
    handled. Previously an icon was created only for non-transient
    windows, which means that 1) minimized transient windows were
    not getting cleaned up at shutdown, and that 2) one could
    conceivably lose access to a minimized transient if there were
    a break in the transient chain.

    For the time being, every iconified window gets an icon. In
    order to make this a little nicer, if a window doesn't provide
    an icon title, the window title is used in the icon menu, rather
    than 'Unnamed'.
  - fixed a bug in handling the destruction of intermediate
    transient windows. The code was leaving the transient of a
    destroyed window with a reference to the now non-existent
    window. This can lead to all sorts of problems.
  - fixed a slight positioning error when the slit is on the right
    side of the screen
  - included a new style, Minimal, which is designed for use on 8-bit
    displays. It tries to use a bare minimum of colors, and with the
    new code regarding Flat Solid, should consume very little memory.
  - made yet another alteration to the way focus changes after a window
    closes under ClickToFocus. Blackbox now tracks the stacking order of
    all windows and uses this information to give the focus to the topmost
    window.
  - new configure option :
	--enable-styled-frames   include support for fully-styled window
	                         frames -- these are the decorations which
				 are affected by the window.frame* theme
				 entries. This option is turned on by
				 default.

				 Because of the way they are implemented,
				 these are typically the most memory and
				 render intensive of the various blackbox
				 decorations, even if they are typically
				 only a pixel or so wide. Disabling this
				 feature can result in a substantial
				 decrease in X memory usage, but it's
				 enabled by default to remain compatible
				 with previous versions.

  - added a whole mess of logic so that blackbox will use
    XSetWindowBackground for Flat Solid textures instead generating
    a pixmap (which would be subsequently cached)... should help cut
    down some on the X memory usage
  - altered the behavior of the BImageControl timer... now it will
    fire every cacheLife minutes, regardless of when anything has
    been removed from the cache
  - modified the NLS build code yet again... at this point we've
    hopefully hit the least common denominator and it should work
    for everyone
  - dealt with a possible problem in the BlackboxWindow constructor
    where we referred to a member after deletion
  - removed a last lingering bit of the allocate()/deallocate() code
  - fixed a pair of string formatting problems


Changes from 0.60.1 to 0.60.2:
  - updated README.bbtools, since bbpager and bbkeys were updated to work with
    0.60.x (also removed the .diffs from the source tree)
  - fix for compiling with NLS support on Solaris
  - added Turkish, Russian and Swedish translations
  - applied patch for more correct Spanish translations
  - added completed pt_BR (Brazillian Porteguese) translation
  - removed mem.h and the allocate()/deallocate() calls throughout blackbox
    these have been unused for a long time, and needed to go away :)
  - compile fixes for --enable-debug
  - changed the font loading/drawing code... XFontSets are only used if
    the locale is set properly.  So you can still compile with nls support,
    but do not set your LANG environment variable, and your fonts will be
    loaded and drawn the old way
  - smarter Basemenu::drawItem() code added, i noticed alot of flicker when
    moving menus, because of code constantly redrawing menus items... this
    has been significatly modified and sped up quite a bit
  - fixed a bug where iconified windows wouldn't remove themselves from the
    icon menu when they unmapped/closed themselves (which would result in a
    crash if you selected this dead item)
  - fixed a potential crash in Workspace::removeWindow() that had relation
    to focus last window on workspace... one person experience gibberish being
    displayed, another experienced a crash
  - fixed a flicker problem when changing focus between windows rapidly
    (the toolbar's window label was getting redrawn twice per focus, not
    optimum behavior)
  - fixed the infamous bsetroot segfault... this was quite a feat... took 3
    people in excess of 8 hours to find... and it was a simple one line change


Changes from 0.51.3.1 to 0.60.1: (note:  0.60.1 is 0.60.0 non-alpha)
  - changed licensing for Blackbox from GNU GPL to more open BSD license
    see the file LICENSE
  - removed alot of empty files that did nothing but passify automake/autoconf
    Blackbox now passes --foreign to automake to lessen the requirements for
    files like NEWS,AUTHORS,COPYING,README,etc.
  - new configure options:
	--enable-ordered-pseudo	this enables a new algorithm for dithering
				on pseudocolor (8 bit) displays... a noticable
				pattern is visible when using this.  you may or
				may not like it... just something different
				if you want it, but is turned off by default.

	--enable-debug		turn on verbose debugging output... this
				isn't very complete or really very helpful...
				right now it just describes memory usage and
				tracks a few X event handlers... this is turned
				off by default

	--enable-nls		turn on natural language support... this option
				will turn on the use of catgets(3) to read
				native language text from any of the supported
				locales (see the nls/ directory for current
				translations)...

				This option also turns on the use of XFontSets,
				which allows the display of multibyte
				characters, like Japanese or Korean.
				This option is turned on by default.

	--enable-timed-cache	turn on/off the new timed pixmap cache... this
				differs from the old pixmap cache in that
				instead of releasing unused pixmaps
				immediately, it waits for <X> minutes, where
				<X> is set with session.cacheLife in your
				~/.blackboxrc...  this option is turned on
				by default.

  - changed the default menu to include a listing of workspaces (and their
    window lists) and the new configuration menu (see below)
  - included new default styles, contributed by regulars on
    irc.openproject.net's #blackbox
  - generated default "translation" catalog for the C/POSIX locale... the
    same catalog is used for English (en_US for now, will add others
    as necessary)
  - included translation for Spanish (es_ES) and Brazilian
    Portuguese (pt_BR)... if you are interested in doing a
    translation, email me at blackbox@alug.org
  - properties and hints added for communication with bbpager and bbkeys, the
    two most common "blackbox addons"
  - KDE 1.x support has been completely removed, pending approval of the new
    window manager specification to be used by KDE2 and GNOME
  - a (broken!) base for the new window manager spec was put in place, but
    using --enable-newspec will result in code that will not compile
  - added a timer class to handle internal timeouts without using
    getitimer/setitimer/SIGARLM... this will enable other things to be done,
    as any number of timers with any timeout can be used
  - Blackbox will search for the highest depth supported by each visual on
    each screen... basically this means that blackbox will try to use
    TrueColor if a TrueColor visual exists (but it's not the default visual)
  - menu hilite changed from being just a color to being a texture and new
    window decoration layout... as a result the style file syntax has changed,
    old styles for 0.5x.x will not work.  See the included styles for examples,
    and browse by http://bb.themes.org/
  - added support for enabled/disabled and selectable menuitems, this is for
    use in the configmenu mostly (but is used in the windowmenu)
  - added the Configmenu, which is insertable into your menu by using:

	[config] (Catchy Label)

    changes made in the configmenu take effect immediately, and are saved in
    your ~/.blackbxrc... current tunable settings:

	Focus model
	Window placement
	Image dithering
	Opaque window moving
	Full Maximization
	Focus New Windows
	Focus Last Window on Workspace

    the window placement and focus model options will be discussed below
  - added texture type "ParentRelative" which causes the decoration to display
    the contents of it's parent... this is a sort of pseudo-transparent option
    and doesn't work for all decorations... see the included style named
    "Operation" for an example of ParentRelative
  - added support for solid interlacing... for example:

	toolbar:		raised interlaced solid bevel1
	toolbar.color:		grey
	toolbar.colorTo:	darkgrey

    will cause the toolbar base to be draw with solid lines, one line grey,
    the next darkgrey, the next grey, the next darkgrey, ...
  - changed dithering algorithm for TrueColor displays from Floyd-Steinberg to
    an ordered dither... dithering at 8bpp (Pseudocolor) can be either FS or
    ordered, but must be selected at compile time...

    NOTE:  when using ordered pseudocolor (8bpp) dithering, your
    session.colorsPerChannel ***MUST*** be 4, otherwise your display will
    not display *any* correct colors
  - fixed TrueColor rendering to do aligned writes (suppresses warnings on
    Alpha Linux machines)
  - added support for GrayScale/StaticGray displays (completely untested)
  - made linked lists smarter, they can now have as many iterators assigned to
    them as you want... no more FATAL errors
  - added the Netizen class, which is a client that has 
    _BLACKBOX_STRUCTURE_MESSAGES listed in their WM_PROTOCOLS... these clients
    get notified of window add, remove, focus, shade, iconify, maximize,
    resize, etc.

    the two most common Netizens are bbpager and bbkeys
  - when loading an incomplete style, blackbox now uses default colors to
    draw decorations (instead of the annoying "see-through" effect)
  - added menu tag [config]... which inserts the Configmenu into your rootmenu
  - made [include] handling smarter, it will only read regular files (it
    won't read a directory in case you ever accidentally put one there)
  - the slit and toolbar menus now include a placement option, which will place
    them in various positions on the screen
  - included a slit menu option to choose it's direction (horizontal or
    vertical)
  - added options to the slit and toolbar menus to have them always stacked
    above other windows
  - right clicking on the workspace label no longer initiates a workspacename
    edit... right clicking anywhere on the toolbar brings up the toolbar menu,
    which has an entry that lets you change the workspace name
  - iconified windows no longer show up in the window list for the current
    workspace... just in the icon submenu
  - ClickToFocus now works like one would think, clicking anywhere in a window
    will focus it
  - overall... this version of blackbox has and does alot more than previously
    just take it for a test drive and see how well you like it


