dnl configure.in for Blackbox - an X11 Window manager
dnl Initialize autoconf and automake

AC_INIT([blackbox], [0.70.0broken], [http://blackboxwm.sourceforge.net])
AM_INIT_AUTOMAKE([blackbox], [0.70.0broken])
AC_CONFIG_SRCDIR([src/blackbox.cc])

dnl Determine default prefix
test "x$prefix" = "xNONE" && prefix="$ac_default_prefix"

dnl Locate required external software
AC_PROG_CXX
AC_LANG([C++])

if test "x$GXX" = "xyes"; then
   CXXFLAGS="${CXXFLAGS} -Wall -W -pedantic"
   gxx_ver=`$CXX -dumpversion|sed -e 's/\..*//'`
   AC_MSG_CHECKING([for GNU C++ version])
   AC_MSG_RESULT([$gxx_ver.])
   if test "$gxx_ver" -ge 3; then
     CXXFLAGS="${CXXFLAGS} -Woverloaded-virtual -Wshadow -Winline"
   fi
else
   mips_pro_ver=`$CC -version 2>&1 | grep -i mipspro | cut -f4 -d ' '`
   if test "x$mips_pro_ver" != "x"; then
      AC_MSG_CHECKING([for MIPSpro version])
      AC_MSG_RESULT([$mips_pro_ver.])
      AC_MSG_CHECKING(for -LANG:std in CXXFLAGS)
      lang_std_not_set=`echo $CXXFLAGS | grep "\-LANG:std"`
      if test "x$lang_std_not_set" = "x"; then
         AC_MSG_RESULT([not set, setting.])
         CXXFLAGS="${CXXFLAGS} -LANG:std"
      else
         AC_MSG_RESULT([already set.])
      fi
   fi 
fi

AC_PROG_RANLIB
AC_PROG_INSTALL

AC_CHECK_PROGS([regex_cmd], [sed])
if test "x$regex_cmd" = "x"; then
  AC_MSG_ERROR([error. sed is required to build the default menu file.])
fi

AC_CHECK_PROGS([gencat_cmd], [gencat])
if test "x$gencat_cmd" = "x"; then
  NLS=
fi

dnl Check for system header files
AC_CHECK_HEADERS(ctype.h dirent.h fcntl.h libgen.h locale.h nl_types.h process.h signal.h stdarg.h time.h sys/param.h sys/select.h sys/signal.h sys/time.h sys/wait.h)
AC_HEADER_TIME

dnl Check for existance of basename(), setlocale() and strftime()
AC_CHECK_FUNCS([basename],
               [],
	       [AC_CHECK_LIB([gen],
	                     [basename],
			     [AC_DEFINE([HAVE_BASENAME])
			      LIBS="$LIBS -lgen"])
               ])

AC_CHECK_FUNCS([setlocale],
               [],
               [AC_CHECK_LIB([xpg4],
                             [setlocale],
                             [AC_DEFINE([HAVE_SETLOCALE])
                              LIBS="$LIBS -lxpg4"])
               ])

AC_CHECK_FUNCS(getpid sigaction strftime strcasestr snprintf vsnprintf catopen catgets catclose)
AC_CHECK_LIB([nsl],
             [t_open],
	     [LIBS="$LIBS -lnsl"])
AC_CHECK_LIB([socket],
             [socket],
	     [LIBS="$LIBS -lsocket"])

dnl Check for X headers and libraries
AC_PATH_X

if test "x$no_x" = "xyes"; then
  AC_MSG_ERROR([Blackbox requires the X Window System libraries and headers.])
fi

if test "x$x_includes" != "x"; then
  CPPFLAGS="$CPPFLAGS -I$x_includes"
  CXXFLAGS="$CXXFLAGS -I$x_includes"
fi
if test "x$x_libraries" != "x"; then
  LIBS="$LIBS -L$x_libraries"
fi

dnl Check for required functions in -lX11
XLIB=
AC_CHECK_LIB([X11], [XOpenDisplay], [XLIB=yes], [XLIB=no])
test "x$XLIB" = "xno" && AC_MSG_ERROR([standard X11 libraries not found])

AC_CHECK_HEADERS([X11/Xlib.h], [XLIB=yes], [XLIB=no])
test "x$XLIB" = "xno" && AC_MSG_ERROR([standard X11 headers not found])

AC_CHECK_HEADERS([X11/Xatom.h X11/Xlocale.h X11/Xresource.h X11/Xutil.h X11/keysym.h], [XLIB=yes], [XLIB=no],
[
#include <X11/Xlib.h>
])
test "x$XLIB" = "xno" && AC_MSG_ERROR([standard X11 headers not found])

dnl Check for XShape extension support and proper library files.
AC_MSG_CHECKING([whether to build support for the XShape extension])
AC_ARG_ENABLE([shape],
              AC_HELP_STRING([--enable-shape],
	                     [enable support of the XShape extension @<:@default=yes@:>@]),
	      [SHAPE="$enableval"]
	      [SHAPE=yes])
AC_MSG_RESULT([$SHAPE])

if test "x$SHAPE" = "xyes"; then
  AC_CHECK_LIB([Xext], [XShapeCombineShape], [SHAPE=yes], [SHAPE=no])
  
  if test "x$SHAPE" = "xyes"; then
    save_LIBS="$LIBS"
      
    LIBS="$LIBS -lXext"
    AC_CHECK_HEADERS([X11/extensions/shape.h], [SHAPE=yes], [SHAPE=no],
[
#include <X11/Xlib.h>
])
      
    if test "x$SHAPE" = "xyes"; then
      SHAPE="-DSHAPE"
    else
      SHAPE=
      LIBS="$save_LIBS"
    fi
  fi
fi
AC_SUBST([SHAPE])

dnl Check for MIT-SHM extension support and proper library files.
AC_MSG_CHECKING([whether to build support for the MIT-SHM extension])
AC_ARG_ENABLE([mitshm],
              AC_HELP_STRING([--enable-mitshm],
	                     [enable support of the MIT-SHM extension @<:@default=yes@:>@]),
	      [MITSHM="$enableval"],
	      [MITSHM=yes])
AC_MSG_RESULT([$MITSHM])

if test "x$MITSHM" = "xyes"; then
  AC_CHECK_LIB([Xext], [XShmPutImage], [MITSHM=yes], [MITSHM=no])

  if test "x$MITSHM" = "xyes"; then
    save_LIBS="$LIBS"

    LIBS="$LIBS -lXext"
    AC_CHECK_HEADERS([X11/extensions/XShm.h], [MITSHM=yes], [MITSHM=no],
[
#include <X11/Xlib.h>
])

    if test "x$MITSHM" = "xyes"; then
      MITSHM="-DMITSHM"
    else
      MITSHM=
      LIBS="$save_LIBS"
    fi
  fi
fi
AC_SUBST([MITSHM])

dnl Check whether to include debugging code
AC_MSG_CHECKING([whether to include verbose debugging code])
AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug],
	                     [include verbose debugging code @<:@default=no@:>@]),
	      [DEBUG="$enableval"],
	      [DEBUG=no])
AC_MSG_RESULT([$DEBUG])

if test "x$DEBUG" = "xyes"; then
    DEBUG="-DDEBUG -fno-inline -g"
else
    DEBUG=
fi
AC_SUBST(DEBUG)

dnl Check whether to include natural language support (i18n)
AC_MSG_CHECKING([whether to include NLS support])
AC_ARG_ENABLE([nls],
              AC_HELP_STRING([--enable-nls],
	                     [include natural language support @<:@default=yes@:>@]),
              [NLS="$enableval"],
	      [NLS=yes])
AC_MSG_RESULT([$NLS])

if test "x$NLS" = "xyes"; then
  NLS="-DNLS"
else
  NLS=
fi
AC_SUBST(NLS)

dnl Determine if maintainer portions of the Makefiles should be included.
AM_MAINTAINER_MODE

dnl Print results
AC_MSG_RESULT([])
AC_MSG_RESULT([	$PACKAGE version $VERSION configured successfully.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Using '$prefix' for installation.])
AC_MSG_RESULT([Using '$CXX' for C++ compiler.])
AC_MSG_RESULT([Building with '$CXXFLAGS' for C++ compiler flags.])
AC_MSG_RESULT([Building with '$LIBS' for linker flags.])
AC_MSG_RESULT([])

dnl Output files
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
Makefile
lib/Makefile
src/Makefile
util/Makefile
data/Makefile
data/styles/Makefile
doc/Makefile
doc/fr_FR/Makefile
doc/ja_JP/Makefile
doc/nl_NL/Makefile
doc/sl_SI/Makefile
nls/Makefile
nls/C/Makefile
nls/da_DK/Makefile
nls/de_DE/Makefile
nls/es_AR/Makefile
nls/es_ES/Makefile
nls/et_EE/Makefile
nls/fr_FR/Makefile
nls/hu_HU/Makefile
nls/it_IT/Makefile
nls/ja_JP/Makefile
nls/ko_KR/Makefile
nls/lv_LV/Makefile
nls/nl_NL/Makefile
nls/no_NO/Makefile
nls/pl_PL/Makefile
nls/pt_BR/Makefile
nls/ro_RO/Makefile
nls/ru_RU/Makefile
nls/sk_SK/Makefile
nls/sl_SI/Makefile
nls/sv_SE/Makefile
nls/tr_TR/Makefile
nls/uk_UA/Makefile
nls/zh_CN/Makefile
nls/zh_TW/Makefile
version.h])
AC_OUTPUT
