$set 1 #BaseDisplay

1 %s:  X error: %s(%d) opcodes %d/%d\n  resource 0x%lx\n
2 %s: signal %d caught\n
3 shutting down\n
4 aborting... dumping core\n
5 BaseDisplay::BaseDisplay: connection to X server failed.\n
6 BaseDisplay::BaseDisplay: couldn't mark display connection as close-on-exec\n
7 BaseDisplay::eventLoop(): removing bad window from event queue\n

$set 2 #Basemenu

1 Blackbox Menu

$set 3 #Configmenu

1 Config Options
2 Focus Model
3 Window Placement
4 Image Dithering
5 Opaque Window Moving
6 Full Maximization
7 Focus New Windows
8 Focus Last Window on Workspace
9 Click to Focus
10 Sloppy Focus
11 Auto Raise
12 Smart Placement (Rows)
13 Smart Placement (Columns)
14 Cascade Placement
15 Left to Right
16 Right to Left
17 Top to Bottom
18 Bottom to Top

$set 4 #Icon

1 Icons
2 Unnamed

$set 5 #Image

1 BImage::render_solid: error creating pixmap\n
2 BImage::renderXImage: error creating XImage\n
3 BImage::renderXImage: unsupported visual\n
4 BImage::renderPixmap: error creating pixmap\n
5 BImageControl::BImageControl: invalid colormap size %d (%d/%d/%d) - reducing\n
6 BImageControl::BImageControl: error allocating colormap\n
7 BImageControl::BImageControl: failed to alloc color %d/%d/%d\n
8 BImageControl::~BImageControl: pixmap cache - releasing %d pixmaps\n
9 BImageControl::renderImage: cache is large, forcing cleanout\n
10 BImageControl::getColor: color parse error: '%s'\n
11 BImageControl::getColor: color alloc error: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: an error occured while querying the X server.\n  \
another window manager is already running on display %s.\n
2 BScreen::BScreen: managing screen %d using visual 0x%lx, depth %d\n
3 BScreen::LoadStyle(): couldn't load font '%s'\n
4 BScreen::LoadStyle(): couldn't load default font.\n
5 %s: empty menu file\n
6 xterm
7 Restart
8 Exit
9 BScreen::parseMenuFile: [exec] error, no menu label and/or command defined\n
10 BScreen::parseMenuFile: [exit] error, no menu label defined\n
11 BScreen::parseMenuFile: [style] error, no menu label and/or filename \
defined\n
12 BScreen::parseMenuFile: [config] error, no menu label defined\n
13 BScreen::parseMenuFile: [include] error, no filename defined\n
14 BScreen::parseMenuFile: [include] error, '%s' is not a regular file\n
15 BScreen::parseMenuFile: [submenu] error, no menu label defined\n
16 BScreen::parseMenuFile: [restart] error, no menu label defined\n
17 BScreen::parseMenuFile: [reconfig] error, no menu label defined\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, no directory defined\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, '%s' is not a \
directory\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, '%s' does not exist\n
21 BScreen::parseMenuFile: [workspaces] error, no menu label defined\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d


$set 7 #Slit

1 Slit
2 Slit Direction
3 Slit Placement

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Toolbar
9 Edit current workspace name
10 Toolbar Placement

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: creating 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres failed\n
3 BlackboxWindow::BlackboxWindow: cannot find screen for root window 0x%lx\n
4 Unnamed
5 BlackboxWindow::mapRequestEvent() for 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() for 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 Send To ...
2 Shade
3 Iconify
4 Maximize
5 Raise
6 Lower
7 Stick
8 Kill Client
9 Close

$set 11 #Workspace

1 Workspace %d

$set 12 #Workspacemenu

1 Workspaces
2 New Workspace
3 Remove Last

$set 13 #blackbox

1 Blackbox::Blackbox: no managable screens found, aborting\n
2 Blackbox::process_event: MapRequest for 0x%lx\n

$set 14 #Common

1 Yes
2 No

3 Direction
4 Horizontal
5 Vertical

6 Always on top

7 Placement
8 Top Left
9 Center Left
10 Bottom Left
11 Top Center
12 BottomCenter
13 Top Right
14 Center Right
15 Bottom Right

$set 15 #main

1 error: '-rc' requires an argument\n
2 error: '-display' requires an argument\n
3 warning: could not set environment variable 'DISPLAY'\n
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <string>\t\tuse display connection.\n\
  -rc <string>\t\t\tuse alternate resource file.\n\
  -version\t\t\tdisplay version and exit.\n\
  -help\t\t\t\tdisplay this help text and exit.\n\n
5 Compile time options:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

$set 16 #bsetroot

1 %s: error: must specify one of: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        display connection\n\
  -mod <x> <y>             modula pattern\n\
  -foreground, -fg <color> modula foreground color\n\
  -background, -bg <color> modula background color\n\n\
  -gradient <texture>      gradient texture\n\
  -from <color>            gradient start color\n\
  -to <color>              gradient end color\n\n\
  -solid <color>           solid color\n\n\
  -help                    print this help text and exit\n

