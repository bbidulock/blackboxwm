-- ChangeLog for Blackbox 0.40.x  - an X11 Window manager

Changes from 0.40.12 to 0.40.13:
	- added some compile time parameters to allow for clean compiling
	- added support for vertical/horizontal maximization (i did this by
	  hand, but kudos to John Martin for the idea ;)
	- added basename() to the distribution... it will only be compiled in
	  if basename is not present in standard libraries
	- window focus code has changed yet again... i've decided to completely
	  rewrite the focus handling code, instead of trying to fix it... let
	  me know how this does
	- a new resource has been added to the style loader... a resource of
	  the form:  rootCommand: <shell command string> will execute this
	  command when the style is loaded, suitable for setting the root
	  window background to an image/pattern/color... this should make
	  style integration more seamless


Changes from 0.40.11 to 0.40.12:
	- more migration to autoconf/automake/autoheader etc.
	- changed the default installation prefix... /usr/local is now the
	  default... all default config files will be stored in
	  ${prefix}/share/Blackbox... any old files will not be used, and
	  should be removed
	- a small internal rework has made the "Inverted" option for
	  pressed button textures obsolete... please update your configs
	- Makefile.generic has been removed
	- Laurie's tear off menu patch has been adapted into the source tree...
	  sorry Laurie, but i had to rework your patch to make it completely
	  bullet proof ;)
	- rework of Image code... resizes and maximizations should be much
	  faster now
	- existance of XConvertCase is checked by configure... if it is NOT
	  found, then when editing the workspace name, pressing shift will
	  not print capital letters... sorry... get an uptodate X distribution
	  (R6.3 or higher) so that XConvertCase exists...
	- the date format on the clock is controlled by a new .blackboxrc
	  resource... session.dateFormat... accepted values are:
		American ( mm/dd/yy )  and
		European ( dd.mm.yy )
	  the default is american... if any other string is entered for the
	  resource, blackbox defaults again to american...
	- changed some window positioning code so that windows aren't thrown
	  to the middle of the screen unless they are completely hidden when
	  shown
	- time bugs have been fixed... this is too detailed to go into... so
	  read the source if you are curious, otherwise just hit Reconfigure
	  when ever you change the system time, and blackbox will update and
	  continue to monitor/display the correct time (also... wrt y2k...
	  blackbox is y2k compliant is your libc's localtime() is y2k
	  compliant)
	- this release has a major internals rework... let me know of any
	  problems... i would also love to hear about improved/degraded
	  performance... enjoy people... 


Changes from 0.40.10 to 0.40.11:
	- changed the blackbox distribution to use autoconf instead of
	  imake... let me know how this works
	- removed all the Imakefiles and Imakeconfig in favor of autoconf...
	- added necessary files for automake and autoconf
	- fixed a bug that would automatically shift focus to the workspace
	  label after switching to an empty workspace which would edit the
	  workspace name if pressing ctrl-arrow...
	- fixed a bug that wouldn't focus any windows with alt-tab after
	  switching workspaces
	- new feature:  click button 1 on the clock to display today's date
	  releasing the mouse button redraws the current time
	- implemented double-click window shading by adapting David Edwards'
          <david@dt031n1a.tampabay.rr.com> shade patch
	- added new .blackboxrc resource - session.doubleClickInterval - which
	  controls the time between double clicks... used by the double-click
	  shade feature... defaults to 250ms is not specified


Changes from 0.40.9 to 0.40.10:
	- fixed the broken menu highlights - they are now a dot in front of
	  the menu label
	- enhanced the image rendering code to prebuild dithering lookup
	  tables... this saves some multiply and divide instructions during the
	  rendering loop... it makes a noticable difference on my lowly p133 ;)
	- just for completeness... i've added some error output for various
	  things that could (but rarely do) go awry
	- the focus code has been updated yet again... but this time it's for
	  the better ;)  the ctrl arrow keys continue working after a window
	  has been closed etc. etc... this should be the final change... unless
	  i find more bugs in it

Changes from 0.40.8 to 0.40.9:
	- fixed a menu bug to keep as much of the menu on screen as possible
	- added a patch from Peter Kovacs <kovacsp@egr.uri.edu> to raise the
	  current focused window when the user clicks on the window label on
	  the toolbar
	- changed some window gravity defaults... nothing major here
	- focus handling code has been spruced up... and majorly tested...
	  0.40.8's focus code was about as good as a full tank of gas but
	  no corvette... let me know how the focus handles in 0.40.9


Changes from 0.40.7 to 0.40.8:
	- more menu fixes... highlights are handled as normal... constant
	  highlights are draw differently... the rounded edges minus the
	  highlighted bar...
	- hand strength reduction in the BImage::renderXImage() method...
	  this doesn't offer much of a speed up... but every little bit
	  counts
	- stuck clients that open transients now have their transients stuck
	  by default
	- changed some input focus code to better handle the sloppy focus
	  model... the little annoyances like two focused windows should now
	  be fixed...
	- removed gcc specific code... changed use of strsep to strtok (which
	  is defined by ANSI C)
	- this is strictly a maintainence release... no new features have
	  been introduced


Changes from 0.40.6 to 0.40.7:
	- changed bhughes@arn.net to bhughes@tcac.net throughout the source
	  tree
	- menu sanity fixes... like unmapping a submenu when an item is removed
	- image code fixes... no memory is allocated during the rendering...
	  only when the BImage is created... thanks to lee.salzman@lvdi.net for
	  the frame work for these changes
	- fewer floating point division in gradient rendering routines... again
	  thanks to lee.salzman@lvdi.net for the basis of these changes
	- reading workspace names is now a little more robust, but probably not
	  bullet proof... events are handled normally while reading the
	  workspace name... instead of blocking them all... the label changes
	  color... and reverts back to normal when enter is pressed (which
	  applies the new workspace name)
	- the window geometry label drawn during window resizing has moved to
          inside the window frame, this allows us to see what size windows are
	  being resized to when the right edge is close to the edge of root
	- a lock system has been implemented for the blackbox objects... this
	  fixes a nasty little problem of stale windows (decorations with no
	  client window) because of on object grabbing the server and another
	  unlocking it... the current system works similar to XLockDisplay
	  (which is used in threaded X programs).
	- icccm code enhancements for XWMHints and NormalHints
	- window maximizing now properly returns the maximized client to its
	  previous location (this is a bug fix... maximize netscape, then
	  maximize an xterm... unmaximizing netscape will put it where the
	  xterm was previously)


Changes from 0.40.5 to 0.40.6:
	- the workspace and client menus now keep the current workspace and
	  focus window highlighted so that we know which window is in focus
	  (especially useful with multiple xterms in the same workspace)
	- image dithering code has been updated slightly to hopefully squeeze
	  every last drop of performance out of it
	- pixel computation has been simplified
	- gradient code has been changed to use less floating point division
	  this breaks Jon Denardis' gradient hack, but the option has been
	  left in place in case Jon wants to re-implement it :)
	- more ICCCM compliance code added... window colormap focus has a
	  click-to-focus policy... any window that wants to use it's own
	  colormap (i.e.  netscape -install) will have it's colormap installed
	  when button1 is pressed anywhere on the decorations (like when
          raising)  the default root colormap is reinstalled when pressing
	  button1 on the root window
	- workspace names can now be changed on the fly... they are stored in
	  the users ~/.blackboxrc file, and may be edited from there, although
	  any changes made to the file will be over written when blackbox is
	  shutdown or restarted...
	- workspace names can be edited *while blackbox is running* by pressing
	  button3 on the workspace label on the toolbar... pressing enter ends
	  the edit and normal event processing resumes... these names are saved
	  on exit/restart for convienence
	- support for window gravity has been added... this is addition is
	  another step closer to complete ICCCM compliance
	- window resizing is a bit more sane now... a bug once pointed out long
	  ago that i never noticed plagued me the other day... when resizing a
	  window to a large size... blackbox can delay a bit while it renders
	  the new decorations... if the user tries to move the window before
	  these decorations are finished... the window is resized again... this
	  has been fixed


Changes from 0.40.4 to 0.40.5:
	- updated the default style to reflect the button resource change in
	  0.40.4
	- added internal menu alignment
	- added internal linked list insertion at a certain point, used in
	  menus
	- submenus now update their parent menu to reflect that the submenu
	  is no longer open
	- right clicking anywhere on the rootmenu or window menus will now
	  unmap them... NOTE: this doesn't work on submenus or the rootmenu
          or on the SendToWorkspaceMenu
	- cleaned up some of the image rendering code to use less comparisons
	  while rendering an image... also removed alot of
          multiplication/division use in beveling loops to increase speed
	- changed dithering error distribution to make images smoother at
	  15 and 8bpp (8bpp got the most benefit from this change)
	- changed the toolbar appearance... removed the raise/lower button,
          changing the level of the toolbar isn't possible as of yet... a new
	  button has been added on the left of the toolbar, pressing it will
	  map the workspace menu, which has a few changes
	- the workspace menu now conatins submenus of all the window lists of
	  all workspaces... it is now possible to see which window is on which
	  workspace... also... the icon button has been removed from the
	  toolbar and the iconmenu is now a submenu of the workspace menu
	- window placement has been slightly modified... if clients request a
	  certain position, the request is honored, otherwise the client is
	  cascaded... if either the cascade or requested position obscures part
	  of the window when the window is created, the window is centered in 
	  the root window...
	- window state updated... when blackbox is restarted, all client
	  windows are placed on the workspace they were previously occupying...
	  this is only between restarts... not when X is restarted...
	  however... applications may be coded specifically for blackbox to
	  start on a certain workspace... for more information on this, email
	  me
	- window menu placement has been made a little more sane


Changes from 0.40.3 to 0.40.4:
	- removed the window.handle{.color,.colorTo} resources... the handle
	  is now treated as a button, and uses the button resources
	- added window.focus.button and window.unfocus.button resources to the
	  style file... this allows colors to be set different from the
	  titlebar... the colors are controlled with window.focus.button.color,
	  window.focus.button.colorTo, window.unfocus.button.color and
	  window.unfocus.button.colorTo
	- transient focus policy has changed... if any window has an open
	  transient window... focus is awarded to the trasient instead of the
	  parent window, even if the mouse doesn't occupy the trasient window
	- cleaned up some namespace in Basemenu.cc and Basemenu.hh
	- changed dithering error diffusion
	- fixed Bevel2 so that it doesn't sig11 anymore
	- changed stacking code slightly... "stuck" windows now are placed
	  in relation to the toolbar... i.e. if the toolbar is on top... stuck
	  windows are on top of any other client windows... if the toolbar
	  is underneath client windows... stuck windows are also stacked
	  underneath the client windows
	- major reworking of Window.cc and Window.hh to fully support the
	  _MOTIF_WM_HINTS on client windows... if this hint is present... then
	  the window is decorated according to those hints... all the move/
	  resize/configure code had to be updated because of this... one step
	  closer to gnome compliance
	- window menus now contain different items based on the functions
	  available to the client window... if a window cannot be maximized...
	  then no maximize item is present in the window menu... also, "Close"
	  and "Kill Client" are no longer present at the same time... if the
	  client supports the WM_DELETE_WINDOW Protocol, then "Close" is
	  present, "Kill Client" is only present for clients that do not
	  support the protocol
	- windows may now be moved by the titlebar, handle or thin border
	  around the window... window menus are also accessible by pressing
	  button 3 on any of these 3 windows
	- a new focus model has been added... it works... but is mostly
	  untested... session.focusModel: AutoRaiseSloppyFocus in .blackboxrc
	  will retain the sloppy focus model... but raise windows to the
	  top when focused...


Changes from 0.40.2 to 0.40.3:
	- fixed a bug in Blackbox::nextFocus that would put blackbox into an
	  infinite loop when 2 windows where open, window 0 was iconified and
	  window 1 had focus and pressing alt-tab or the next window button
	  on the toolbar
	- completely recoded all the graphics stuffs to support all visual
	  classes and color depths, also the image code is more compact and
          faster than previous releases
	- removed graphics.cc and graphics.hh from the distribution and added
	  Image.cc and Image.hh for the new graphics implementation.  a new
	  class called BImageControl is now in charge of all pixmap caching
	  and color allocation on displays that don't run/support TrueColor...
	  this takes the job away from the Blackbox class... whose job is now
	  to manage all it's children and disperse events read from the display
	- fixed bug that didn't handle windows created before blackbox is
	  running (again :/)


Changes from 0.40.1 to 0.40.2:
	- added a variable initialize line of code to keep blackbox from
	  splattering from a sigsegv on startup


Changes from 0.35.0 to 0.40.1:
	- cosmetic menu rendering fixes, changed the way the submenu dot is
	  sized; changed an off by one error in drawing the rounded edges;
	  fixed the text to be draw in the center of the item instead of at the
	  bottom
	- major changes to the toolbar (formerly the workspace manager) to
	  change the way it looks and works.  The large blank space is gone,
	  and the toolbar is now half the height it used to be (roughly). the
	  workspace label displays the current workspace, with the workspace
	  menu accessible by clicking button 1 on the label. the two buttons
	  directly to the right of the workspace label change the workspace
	  when pressed. the window label displays the current focused window,
	  which makes it easier to identify which window has input focus (for
	  some people like me that have very dark or very closely colored
	  decorations).  the window menu is accessible by pressing button
	  one on the window label, and selecting an item from the window menu
	  will set input focus to that window (if it can receive focus). the
	  two buttons to the right of the window label circulate focus (up and
	  down, respectively) through the window list, skipping windows that
	  cannot receive focus.  the icon button displays a menu of all
	  iconified applications.  both the icon menu and the window menu will
	  not become visible if they are empty.  the next button on the toolbar
	  is a raise/lower button for the workspace manager. the toolbar is
	  stacked on startup according to the resource set in ~/.blackboxrc,
	  but this button will raise and lower the workspace to the users
	  desire, saving the stack order when blackbox is exited or restarted.
	  the clock is still the same, but editing the session.clockFormat
	  will change it from normal time (session.clockFormat: 12) to 24hour
	  format (session.clockFormat: 24)
	- a pixmap cache has been implemented. a linked list stores all images
	  rendered, removing them from the list and freeing them with the X
	  server when all applications have removed references to them. for
	  those who start man instances of the same applications will benefit
	  greatly from this, as the same decorations are not redraw for each
	  and every window.  this greatly reduces the load on the X server
	  (my X server went from taking 20-28mb of memory to 8-11mb, a dramatic
	  improvement, especially on this 32mb machine).  as a result of this,
	  reconfiguring is faster, as is startup and restarting.
	- click to focus has been implemented, with some restrictions. other
	  window managers allow the user to click anywhere on the decorations
	  OR the client itself to set focus.  i have not found an elegant way
	  to do this yet, so focus can only be set by pressing button 1 on
	  the decorations (like the titlebar, handle, buttons, border, etc.)
	  just not on the client itself.  i am looking more into this, but
	  don't expect anymore than what is in place now.  to use
	  click-to-focus, put session.focusModel: ClickToFocus in ~/.blackboxrc
	- 2 new commands have been added to the menu syntax, [include] and
	  [style]... the [reconfig] command still has the option to reconfigure
	  after a command has been run, but probably will be faded out...
	  [include] (/path/to/file) includes the file inline with the current
	  menu, meaning that a submenu isnot created for the separate file,
	  if a submenu is desired, the file should include the [submenu] and
	  [end] tags explicitly.
	  [style] is a new addition for the style file support. syntax is:
	  [style] (label here) {/path/to/style/file} which will read the new
	  style file and reconfigure when selected.
	- style files have been added to allow for easier switching between
	  configurations.  the style file resources are dramtically different
	  from those in 0.3x.x, see app-defaults/Blackbox-style.ad for an
	  example...
	- with the addition of style files, menus have been given their own
	  justification resource, allowing (for example) menus to be left
	  justified while titles are center or right justified.
	- please read the sample configuration files in app-defaults/ for the
	  new and improved configuration system.  NOTE: Blackbox.ad is a
	  sample ~/.blackboxrc, but you shouldn't copy this file to
	  ~/.blackboxrc, as Blackbox will store the resources it needs
	  automatically
	- an unofficial release numbered 0.40.0 was given out to some 
	  testers, and even this release needs the same treatment as 0.35.0
          with respect to the new config system (0.40.0 only implemented the
          pixmap cache, the new toolbar and *part* of the new config system,
          but not the style files or automatic generation of ~/.blackboxrc)


ChangeLog for 0.3x.xx:
----------------------
	  
Changes from 0.34.5 to 0.35.0:
	- changed the way menus are draw to round both end of the highlight...
	- cosmetic enhancements for the various justifications...
	- this is the first stable release of blackbox


Changes from 0.34.4 to 0.34.5:
	- hopefully... this will be the last bug fix... so i can begin working
	  on new features... i fixed event mask selection on client windows
	  after reparenting them to the decoration frame... this should get
	  xv working again...
	- changed the signal hander to core on sigsegv and sigfpe... sigint and
	  sigterm will just exit blackbox cleanly... sighup will cause blackbox
	  to reconfigure itself
	- changed the way the version string is printed... 


Changes from 0.34.3 to 0.34.4:
	- changed the window stacking code to stack windows and their menus
	  more sanely... window menus are stacked directly ontop of the client
	  windows... instead of on top of every other client window... the
	  workspace manager is now by default stacked above client windows...
	- reworked alot of code in Window.cc, blackbox.cc, Workspace.cc and
	  WorkspaceManager.cc to properly handle ICCCM state hints... the
	  startup and shutdown code has been completely reworked as a result of
	  this

Changes from 0.34.2 to 0.34.3:
	- this was a small change in the code... but a BIG change for the user
	  base... the X error handler is now non fatal... yes... this means if
	  blackbox encounters an X error (like a bad window or a bad match) it
	  will fprintf() the error and continue running... the quick window bug
          has been mostly fixed... i have a small app that i wrote that quickly
	  maps a window, calls XSync()... then destroys the window and exits...
	  the first time i ran this little beauty... blackbox died a horrible
	  death... blackbox now handles this app nicely... but does
	  occasionally report an error (during the decoration creation... which
	  is promptly destroyed from the destroy notify event placed in the
	  queue by the X server... thus... no memory leaks... no memory
	  corruption... blackbox just keeps chugging along nicely


Changes from 0.34.1 to 0.34.2 (unreleased):
	- fixed MSBFirst byte order image rendering at 32bpp (24bpp pending)
          (for machines better than this intel machine of mine)
	- changed BImage to allocate dithering space when the image is created
	  and to delete it when the image is destroyed... instead of allocating
	  the space and deleting the space each time the image is rendered to 
	  an XImage... hopefully this will provide a speed increase (albeit a
	  small one)
	- changed blackbox to call XListPixmapFormats once at startup...
	  instead of each time an image is rendered... this should afford some
	  speed increase (a small one at best :)
	- fixed a bug in Window.cc that re-reads the window name...
          Jon Denardis discovered this bug while playing with netscape 4.5...
          the validation call is now directly before the XFetchName call...
          instead of before an if() { } block that calls strcmp and XFree()
	- edit Window.cc to change the way buttons are decorated and sized
	  the associatedWindow.button.color(To) resources have been removed,
          but the associatedWindow.button texture resource is still there


Changes from 0.34.0 to 0.34.1:
	- fixed the unmanaged rxvt/xconsole/whatever problem that didn't
	  decorate windows at start up... just a little logic error that was
	  fixed with a few braces
	- fixed the shutdown code so that X and blackbox don't die a gruesome
	  death while reparenting the small applets on the workspace manager
	  toolbar... the above bug fixed also fixed a bug that didn't reparent
	  any existing app windows...
	- updated libBox code to allow for flaws in it's design (forgotten from
	  0.34.0)
	- removed #include <sys/select.h> from blackbox.cc so that it compiles
	  on any platform (since select is supposed to be defined in unistd.h)
	- removed the NEED_STRNCASECMP block in blackbox.cc until i can get
	  a working posix like routine to work (needed for OS2 platforms)
	- edited the Imakefile scheme to have the the toplevel Imakefile and 
	  Imakefiles in app-defaults/ lib/ and src/... there now is Imakeconfig
	  which includes all the options in one file... so that editing all
	  the Imakefiles is no longer necessary


Changes from 0.33.6 to 0.34.0:
	- edited some Imakefiles so that rpm creaters have an easier time
	- added stuff to lib/  which contains a small (VERY small) library for
	  letting applications open a window on the workspace manager toolbar
	  this is very very new... restarting will cause the app to crash
	  (at best) or take X with it (the worst)... play with it an let me
	  know how it works
	- further revised window.cc and blackbox.cc to provide better error
	  checking... window.cc received the most updates... validating a
	  window is now done in the statement before the window is used... not
	  at the beginning of the function the window is used in...
	- fixed the stacking order bug when changing workspaces... the windows
	  will now be restored in the order that you left them... not in the
	  order they were created in...
	- updated the README... a little bit anyway :)
	- updated BlackboxWindow::maximizeWindow() in window.cc to properly
	  maximize windows that have specified size increments
	- fixed BlackboxWindow::configureWindow so that shaded windows that re
	  size themselves only resize the titlebar
	- added ccmalloc 0.2.3 to the main source tree to aid in debugging...
	  this is NOT maintained by myself, see the source tree for details
	- eliminated a double delete call with the aid of ccmalloc!@#!
	

Changes from 0.33.5 to 0.33.6:
	- added Makefile.generic for those of you with foobared imake configs.
	  the use of xmkmf -a (i.e. imake) is still prefered... but this should
	  work on any system... with a little editing
	- added static int handleXErrors(Display *, XErrorEvent *) in
          blackbox.cc to handle any and all X lib errors while blackbox is
	  running... this should produce a coredump and thus the -moron
	  community should be able to flood my inbox with stack trace upon
	  stack trace :)
	- added some sub directories and moved the sources around, this allows
	  for easier inclusion of the library for blackbox specific programs
	  (which will run in the dock)
	- hopefully fixed the "disappearing-rxvt-trick"... since i can't
	  reproduce it i don't know for sure
	- removed the use of alloca in graphics.cc... i was noticing very odd
	  behaviour from malloc() and free()... where blackbox would sig11 
	  when exiting because of XCloseDisplay doing something naughty...
	  and this seems to have done the trick... no more sig11's from malloc
	  or new... everything i've thrown at blackbox is gently but firmly
	  beaten into submission...
	- added docboy's curved gradient hack as a compile time option... see
	  src/Imakefile and src/graphics.cc


Changes from 0.33.4 to 0.33.5:
	- added a small test to cascade windows that start out partially hidden
	  (like netscape, Xnest, xv, etc.)
	- changed icon handling to include a menu of icons accessible from the
	  workspace manager toolbar
	- deiconifying a window now takes it to the top of the stack
	- clicking on a menuitem that has a submenu no longer hides the submenu
	- added resource "workspaceManager.24hourClock",  a value of True turns
	  on the 24hour clock on the toolbar
	- removed icon pixmap/window/mask support/handling from window.cc and
	  window.hh... since icons are now handled in a menu, this is no
	  longer needed
	- added session.handleWidth and session.bevelWidth
	  to control window sizes (instead of hardcoded defaults)
	- changed parts of Basemenu.cc and WorkspaceManager.cc to follow the
	  sizes set by session.bevelWidth
	- fixed Alt-Tab window switching... also fixed some focus handling bugs
	  which let two windows become focused at the same time (which is bad
	  mojo)


Changes from 0.33.3 to 0.33.4:
	- corrected a typo in the sample Blackbox.ad file to correctly show
	  which resource to set for the menu file
	- added moderate window group support for programs like netscape and 
	  other motif applications... modified window stacking code and
	  internal list code to support window groups (this makes transients
	  behave properly... another step towards more complete ICCCM
	  compliance)
	- modified focus event handlers to stop applications from focusing out
	  when pressing menubars... also window focus is returned to root if 
	  the focus window is closed... if another window is under the focus
	  window when it is closed... that window is awarded input focus
	- fixed tiny little bug that didn't move the close button when resizing
	  a window


Changes from 0.33.2 to 0.33.3: 
	- changed some of the menu code ("updated" in 0.33.1) back to the
	  original 0.33.0, which seems to perform better.  Reason behind it? -
	  blackbox died too often with 0.33.1/2
	- added "Kill Client" option to window menus... for those applications
	  that don't accept the WM_DELETE_WINDOW atom
	- menus that are not partially moved off the root window are shifted to
	  a visible position when the pointer enters the frame... it is also
	  shifted back to it's original position when left (this is new... let
	  me know how it works)


Changes from 0.33.1 to 0.33.2: (unreleased)
	- changed BlackboxIcon to not try and read its config when it was
	  created.  This was forgotten from the 0.31.0 -> 0.33.0 move :/


Changes from 0.33.0 to 0.33.1: (unreleased)
	- improved menu handling, less possibilty for SIGSEGV
	- menus now make copies of all label, exec strings and titles, to
	  make less loose pointers
	- fixed typo to allow submenus of submenus of submenus (...)
	- fixed workspace menu and window list menu placements


Changes from 0.31.0 to 0.33.0:
	- added #ifdef statements so the C preprocessor doesn't complain about
	  _GNU_SOURCE being redefined.
	- changed internal resource data structures
	- added Sticky windows functionality
	- remove old animation code bound with #ifdef ANIMATIONS
	- fixed a silly little bug that sometimes mapped a submenu when its
	  parent was unmapping itself
	- added ExecReconfigure option to execute a shell statement before
	  performing reconfiguration
	- rearranged window config code to reduce wait time while resizing
	- added internal macro BImageNoDitherSolid to make window frame
	  rendering faster (dithering a solid image is silly anyway)
	- added new menu file format
	- added Blackbox::validateWindow to provide a stabler environment for
	  Blackbox.  This gives blackbox more error checking and greater
	  stability.  For me, random crashes have (nearly) disappeared.
	- removed window name/class dependant frame texture/color
	- with 0.31.0, each entity read it's configuration from the rc database
	  loaded at start.  this has changed back to the old behaviour of
	  reading all configuration parameters at start, no database reads are
	  performed after the initial setup (save for reconfiguring).
	- configuration has changed to be a little cleaner, and a little more
          thorough.  See the Blackbox.ad and BlackboxMenu.ad for exmaples.

