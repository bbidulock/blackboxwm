$set 14 #main

$ #RCRequiresArg
# error: '-rc' kräver ett argument\n
$ #DISPLAYRequiresArg
# error: '-display' kräver ett argument\n
$ #WarnDisplaySet
# varning: kunde inte sätta variabeln 'DISPLAY'\n
$ #Usage
# Blackbox %s: (c) 1997 - 2000 Brad Hughes\n\n\
  -display <string>\t\tanvänd skärmanslutning.\n\
  -rc <string>\t\t\tanvänd alternativ resursfil.\n\
  -version\t\t\tvisa version och avsluta.\n\
  -help\t\t\t\tvisa denna hjälptext och avsluta.\n\n
$ #CompileOptions
# Kompilerad med:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Form:\t\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n
