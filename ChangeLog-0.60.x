ChangeLog for 0.60.x


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


