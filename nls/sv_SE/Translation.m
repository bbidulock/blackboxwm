$set 1 #BaseDisplay

1 %s:  X fel: %s(%d) opkod %d/%d\n  resurs 0x%lx\n
2 %s: signal %d fångad\n
3 stänger ner\n
4 avbryter... dumpar kärna\n
5 BaseDisplay::BaseDisplay: anslutning till X server misslyckades.\n
6 BaseDisplay::BaseDisplay: kunde inte markera skärmanslutning som stäng-vid-exekvering\n
7 BaseDisplay::eventLoop(): tar bort dåligt fönster från händelse-kön\n

$set 2 #Basemenu

1 Blackboxmeny

$set 3 #Configmenu

1 Installningar
2 Fokusmodell
3 Fonsterplacering
4 Bild dithering
5 Oklar fonsterrorelse
6 Full maximering
7 Fokus pa nya fonster
8 Fokus pa senaste fonster
9 Klicka for fokus
10 Hafsig fokus
11 Hoj automatiskt
12 Smartplacering (Rader)
13 Smartplacering (Kolumner)
14 Kaskadplacering
15 Fran vanster
16 Fran hoger
17 Uppifran
18 Nerifran

$set 4 #Icon

1 Ikoner
2 Inget namn

$set 5 #Image

1 BImage::render_solid: fel under skapande av pixmap\n
2 BImage::renderXImage: fel under skapande av XImage\n
3 BImage::renderXImage: icke stöd för färgdjup\n
4 BImage::renderPixmap: fel under skapande av pixmap\n
5 BImageControl::BImageControl: ogiltig färgkarta storlek %d (%d/%d/%d) - reducerar\n
6 BImageControl::BImageControl: fel under allokering av färgkarta\n
7 BImageControl::BImageControl: misslyckades att allokera färg %d/%d/%d\n
8 BImageControl::~BImageControl: pixmap cache - släpper %d pixmappar\n
9 BImageControl::renderImage: stort cache, tvingar upprensning\n
10 BImageControl::getColor: färgfel: '%s'\n
11 BImageControl::getColor: färgallokerings fel: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: ett fel inträffade under en query till X servern.\n  \
en annan fönsterhanterare körs redan på skärmen %s.\n
2 BScreen::BScreen: hanterar skärm %d med visuell 0x%lx, färgdjup %d\n
3 BScreen::LoadStyle(): kunde inte ladda font '%s'\n
4 BScreen::LoadStyle(): kunde inte ladda standard font.\n
5 %s: tom menyfil\n
6 xterm
7 Starta om
8 Avsluta
9 BScreen::parseMenuFile: [exec] fel, ingen menu-etikett och/eller kommando definierat\n
10 BScreen::parseMenuFile: [exit] fel, ingen meny-etikett definierad\n
11 BScreen::parseMenuFile: [style] fel, ingen meny-etikett och/eller filnamn definierat\n
12 BScreen::parseMenuFile: [config] fel, ingen meny-etikett definierad\n
13 BScreen::parseMenuFile: [include] fel, inget filnamn definierat\n
14 BScreen::parseMenuFile: [include] fel, '%s' är inte en vanlig fil\n
15 BScreen::parseMenuFile: [submenu] fel, ingen meny-etikett definierad\n
16 BScreen::parseMenuFile: [restart] fel, ingen meny-etikett definiera\n
17 BScreen::parseMenuFile: [reconfig] fel, ingen meny-etikett definierad\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] fel, ingen katalog definierad\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] fel, '%s' är inte en katalog\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] fel, '%s' existerar inte\n
21 BScreen::parseMenuFile: [workspaces] fel, ingen meny-etikett definierad\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d


$set 7 #Slit

1 Slit
2 Slits riktning
3 Slits placering

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Verktygsbom
9 Andra namn pa skrivbordet
10 Verktygsbommens placering

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: skapar 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributes misslyckades\n
3 BlackboxWindow::BlackboxWindow: kan inte hitta skärm för rootfönster 0x%lx\n
4 Inget namn
5 BlackboxWindow::mapRequestEvent() för 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() för 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: 0x%lx till root\n

$set 10 #Windowmenu

1 Skicka till ...
2 Skugga
3 Ikonifiera
4 Maximera
5 Hoj
6 Sank
7 Klibbig
8 Doda klient
9 Stang

$set 11 #Workspace

1 Skrivbord %d

$set 12 #Workspacemenu

1 Skrivbord
2 Nytt skrivbord
3 Ta bort sista

$set 13 #blackbox

1 Blackbox::Blackbox: inga hanterbara skärmar hittades, avslutar\n
2 Blackbox::process_event: MapRequest för 0x%lx\n

$set 14 #Common

1 Ja
2 Nej

3 Riktning
4 Horisontell
5 Vertikal

6 Alltid overst

7 Placering
8 Uppe till vanster
9 Mitten till vanster
10 Nere till vanster
11 Uppe i mitten
12 Nere i mitten
13 Uppe till hoger
14 Mitten till hoger
15 Nere till hoger

$set 15 #main

1 error: '-rc' kräver ett argument\n
2 error: '-display' kräver ett argument\n
3 varning: kunde inte sätta variabeln 'DISPLAY'\n
4 Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <string>\t\tanvänd skärmanslutning.\n\
  -rc <string>\t\t\tanvänd alternativ resursfil.\n\
  -version\t\t\tvisa version och avsluta.\n\
  -help\t\t\t\tvisa denna hjälptext och avsluta.\n\n
5 Kompilerad med:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Form:\t\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

$set 16 #bsetroot

1 %s: fel: måste specifiera en av: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        skärmanslutning\n\
  -mod <x> <y>             modulamönster\n\
  -foreground, -fg <color> förgrundsfärg för modulamönster\n\
  -background, -bg <color> bakgrundsfärg för modulamönster\n\n\
  -gradient <texture>      texturlutning\n\
  -from <color>            startfärg\n\
  -to <color>              slutfärg\n\n\
  -solid <color>           fast färg\n\n\
  -help                    visa denna hjälptext och avsluta\n

