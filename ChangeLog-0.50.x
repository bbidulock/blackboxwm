ChangeLog for Blackbox 0.50.x
Enjoy! :)

Changes from 0.50.4. to 0.50.5:
  - modified and merged some patches from several contributors.  added their
    names to AUTHORS
  - major documentation updates
  - added a few more platform success reports.  changed development platform
    again :)
  - added new texture option: Interlaced... it is an extension to the gradient
    texture that looks really neat... it is compiled in by default but may
    be removed with --disable-interlace
  - let's see... where do i begin... the code for 0.51.x has been GREATLY
    enhanced over 0.50.4 (and the stupid little compile error for KDE has been
    fixed ;))  Blackbox has undergone major renovation... and i can proudly
    say that this release is rock solid.  Also, i reinstated ccmalloc's tour
    of duty, and spent several days with it stomping memory leaks in blackbox.
    i can proudly say that there are no major memory leaks present in 0.51.x
  - the toolbar has changed it appearance a little bit... the menu button has
    been removed and the labels and buttons are now symmetrically placed on the
    toolbar...  oh no!? how do you get to your icons/workspaces now?  the
    middle click patch from Greg Barlow has been modified, enhanced and merged
    with 0.51.x... the workspace menu now behaves just like the root menu...
    and it can be pinned to the root window (just move it)
  - the image code has once again been worked over... this time a local LUG
    friend and i have hashed it out many times and into the wee hours of the
    morning... this stuff is FAST now... before i added interlacing... we
    had doubled the speed of the dgradient function... yes... *doubled*
  - the code to generate error tables, color tables and other tables that are
    used in image dithering has been rewritten, which severs the last tie to
    window maker's wrlib that blackbox had.  i now understand why and how all
    the code that i "borrowed" works... and it's been improved... because of
    this change... dithering is a lot cleaner... and dithering on 8bpp displays
    is less grainy and less obtrusive...
  - the linked list code has also been rewritten... blackbox has been using
    a doubly linked list, and not taking advantage of all the list's
    capabilities (because it doesnt need them)... so the linked lists are now
    single-link and much quicker at inserting, removing, searching...
  - once again... the menu parsing code has been rewritten... this code is
    very efficient and very extensible... so extensible infact that after
    implementing the current menu syntax... i added a new tag! you can now
    insert the workspaces list into your root menu with this:

	[workspaces] (descriptive label)

  - the slit menu is now spacially correct... if you want the slit in the top
    right corner of the root menu... click the top right corner of the slit
    menu... i think this is a little more user friendly
  - window gravity should be better supported now... restarts and what not
    shouldn't produce all those one pixel shifts or moves anymore...
  - the modifiers for the keygrabs in blackbox are now configurable...
    the "keys" are still hard wired (left/right for workspace changing, tab
    for window cycling)... but you may now configure which modifiers to use
    with the key combos...  this introduces two new resources into your
    .blackboxrc:

	session.workspaceChangeModifier

    and

	session.windowCycleModifier

    these resources may be set to any combination of the following:

	Control Mod1 Mod2 Mod3 Mod4 Mod5 Lock Shift

    also... for convenience... "Alt" is parsed as "Mod1"...
    session.workspaceChangeModifier defaults to "Control" and
    session.windowCycleModifier defaults to "Mod1"
  - smart placement has been made smarter thanks to Dyon Balding's smarter
    placement patch... this patch has been modified from the original slightly
    (mostly speed concerns)
  - signal handling has been made more robust... this allows it to compile on
    more platforms and now prefers to use sigaction() over signal()
  - over all... many code clean ups were made and old commented code was
    purged... this is a very clean very stable release... enjoy people :)


Changes from 0.50.3 to 0.50.4:
  - changed some Copyright information to include the current year
  - added a number of platforms to the Supported Platforms section of the
    README
  - added the Slit... the Slit is a window maker dockapp util that lets users
    use all of applications with Blackbox, and allows users to easily switch
    between Window Maker and Blackbox more easily... it is included by default,
    but you can remove it from the source with --disable-slit on your configure
    command line
  - large Brueghel styles and images removed from the base distribution
  - merged a patch from Benjamin Stewart for very robust menu parsing... this
    patch allows for parenthesis in menu files, and works well for
    automatically generating menus from shellscripts and programs... the menu
    syntax has not changed... it just is understood better :)
  - added shell style tilde-slash (~/) home directory expansion for the
    [include] and [style] tags in menu files
  - added some sanity to window position/gravity code to for GTK applications
  - added Window Maker style Mod1+MouseButton1+Motion window moving (for those
    few braindead apps that like to be positioned where no decorations are
    visible)
  - added a SIGCHLD handler to clean up processes started by a startup script
    that then exec's blackbox (gets rid of all those zombie processes)
  - added a new resource to .blackboxrc which tells Blackbox where to put the
    Slit... editing your .blackboxrc to change this is discouraged and
    discarded, as the Slit has a menu that lets you select where to put it
    (click any mouse button on the slit and see for yourself)
  - fixed a bug in the workspace renaming feature that ate all Shift keypresses


Changes from 0.50.2 to 0.50.3:
  - few documentation updates
  - fixes to let -lgen actually get linked with the executable (fixes compile
    errors on some platforms, most notably, IRIX 6.5)
  - a new series of styles has been added to the distribution (this accounts
    for the increased size)
  - fix to let 16 color servers run blackbox (colormap reduction)
  - various bug fixes... numerous strncpy's changed to sprintfs...
  - default font set internally to "fixed" (to let it run on servers that don't
    have any fonts installed)
  - fixed bug to let blackbox remove all but the last workspace (instead of the
    last two)
  - window gravity offset changes
  - the default key grabs have changed... there are now 4: alt-tab,
    shift-alt-tab, ctrl-alt-right, ctrl-alt-left... these keys perform
    as would be expected
  - fixed wire move bug for transient windows
  - passified error handing for the main window class
  - fixed gravity restore for restart/exit purposes


Changes from 0.50.1 to 0.50.2:
  - minimal KDE integration (configure/compile time option, turned off by
    default).  This is unfinished and i can't really say if i ever will finish
    it, but there is enough there to integrate the panel and other modules
    with Blackbox.
  - changed the regexp in building menus to use a comma (,) as the separator,
    instead of a period
  - various bug fixen (like the one where the window list would stay put after
    the workspace menu went away)
  - some hacks to improve speed in the LinkedList routines
  - new stacking method (to better integrate with the KDE support)... windows
    are no longer in different "levels", raising windows brings them ALL the
    way to the front (so it's possible to obscure override redirect windows
    like image splashes etc.) and lowering throws them ALL the way to the
    back (even under kfm's icons)... however, the rootmenu and the toolbar
    (if configured to be ontop) will be placed above raised windows
  - sticky windows have changed due to the new stack implementation, they can
    be anywhere in the stack (and not always ontop or onbottom)
  - session.screenNUM.toolbarRaised resource has changed to
    session.screenNum.toolbarOnTop
  - the workspace label in the toolbar is sized a little more sanely now
    (i found that it looks the best when the workspace label width == clock
    label width)
  - colormap focus now has it's own resource, session.colormapFocusModel, which
    is set to "Click" by default, which means you have to click a window's
    decorations or the root window to (un)install a colormap... setting this
    resource to "FollowsMouse" will work just as it says... the window under
    the pointer will have it's colormap installed


Changes from 0.50.0 to 0.50.1:
  - eliminated the need for XConvertCase... workspace editing should now print
    any and all characters correctly
  - added check for libgen.h (which provides the prototype for basename() on
    some systems, like OpenBSD)
  - some code obfuscation (i've been removing comments, as some of them don't
    relate to some of the code below them... i plan on recommenting the code
    some time soon)
  - clicking button 3 will hide ANY menu now, and in the case of the workspace
    and or client menus, any other menus and/or buttons associated will be
    closed as well
  - added a patch for multi-screen which sets the DISPLAY env variable so that
    items selected from one screen don't show up on another... many thanks to
    F Harvell <fharvell@fts.net> for this
  - fixed a clock bug... again thanks to F Harvell for this one
  - complete and proper window placement and window restore has been
    implemented... windows that are partially off screen will be placed in the
    center of the root window
  - the toolbar's workspace label is now dynamically sized according to the
    length of the workspace names
  - as stated above... workspace name editing has been completely redone, i
    discovered XLookupString() this weekend and have deemed it the function of
    the week... any and all characters should be printed properly now
  - window placement now has it's own resource...
    session.screen<NUM>.windowPlacement which may be set to SmartPlacement
    (which has been implemented) or anything else to default back to cascade
    placement
  - a new resource, session.screen<NUM>.toolbarWidthPercent has been added, and
    should be set to an integer representing what percentage of the root window
    width the toolbar should occupy (default has been changed back to 67)


Changes from 0.40.14 to 0.50.0:
  - added util/ subdirectory to place small, utility programs to use in
    conjunction with blackbox.
  - updated the README... it's still vague and useless, but gives a better
    view of whats going on
  - the configure script now checks for a few more headers, setlocale and
    strftime in addition to basename functions to better include support for
    multiple arch/langs/etc.
  - updated default menu file... made it a little more general... and made
    the default style menu [include]'d instead of explicitly included...
    this break off of the style menu allows for custom menus to include the
    default style menu for a create selection of styles
  - changed all the default styles to use bsetroot instead of xsetroot
  - menu handling has been improved... no more than one menu at a time may be
    visible on the desktop (save for the root menu and it's tear off menus)
    this means that you can't have multiple window menus and the workspace menu
    open all at once... which saves screen space and reduces clutter
  - much of the code has been reorganzied and reformatted for better
    readability... this consists of function name changes and function
    "ownership" (which basically means workspaces aren't managed by the toolbar
    itself anymore, but by a general screen class on which the toolbar can
    operate)
  - the workspacemenu now autohides when selecting a window from one of the
    window lists
  - removed many empty destructors for Basemenu subclasses to improve code
    readability
  - two new files, Screen.cc and Screen.hh, have been added to the distribution
    they add the new class BScreen which was needed for the biggest change of
    the Blackbox code base, the addition of multiple screen (i.e. multihead)
    support.  A separate BScreen is created for each screen, and all screens
    work inconjunction with the other... windows can't be passed between
    screens, because the X server doesn't allow this (more investigation on
    this later)
  - the toolbar's clock format is now controlled by the strftime() function...
    if configure can't find this function on your system, the old date/time
    format is used... with strftime, clicking on the clock doesn't display the
    date... as the date may now be part of the clock display... read the man
    page for the strftime function to write a format string for your clock,
    and place it in .blackboxrc (i.e.
      session.strftimeFormat: %I:%M %p on %a, %b %d is my strftime format
    string)
  - the toolbar has been stripped of it's workspace responsibilities, but this
    change has no effect on the end user.
  - common code interspersed through out the code has been consolodated into
    small functions and called multiple times instead of having the same or
    similar code repeated in the same class
  - the window startup code has been improved upon again so that shaded windows
    are restored between restarts
  - some ICCCM code has been updated to properly reflect the state of windows
    while shaded or on different workspaces... this state code change should
    also fix the JX toolkit problem of deiconifying and nothing being redrawn
  - the main Blackbox class has been changed to purely handle X events... it
    doesn't manage resources (save for those necessary for proper event
    handling, like the focus model for each screen)
  - the format of .blackboxrc has changed slightly, the session.menuFile,
    session.doubleClickInterval, session.imageDither, session.styleFile,
    and session.colorsPerChannel resources are unchanged.  However, the
    following resources are screen dependant:

	session.screen<num>.strftimeFormat
	session.screen<num>.workspaces
	session.screen<num>.workspaceNames
	session.screen<num>.toolbarRaised
	session.screen<num>.focusModel

    where <num> is the screen number (zero being default and all that would be
    present on a single screen/monitor setup).
  - a utility named bsetroot (mentioned above) has been included in the
    blackbox distribution, to aid in setting root window patterns (ala
    xsetroot).  the only different between xsetroot and bsetroot is that
    bsetroot doesn't redefine cursors, and doesn't restore defaults if no
    arguments are given.  bsetroot does support multiple screens, and is ideal
    for those setups (instead of running xsetroot for each screen)

