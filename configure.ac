dnl configure.in for Blackbox - an X11 Window manager
dnl Initialize autoconf and automake

AC_INIT([blackbox], [0.70.1], [http://blackboxwm.sourceforge.net])
AM_INIT_AUTOMAKE([blackbox], [0.70.1])
AC_CONFIG_SRCDIR([src/blackbox.cc])

dnl Determine default prefix
test "x$prefix" = "xNONE" && prefix="$ac_default_prefix"

dnl Look in the most logical places for external libraries
CPPFLAGS="$CPPFLAGS -I$prefix/include"
LDFLAGS="$LDFLAGS -L$prefix/lib"
if test "x$prefix" != "x/usr/local"; then
  CPPFLAGS="$CPPFLAGS -I/usr/local/include"
  LDFLAGS="$LDFLAGS -L/usr/local/lib"
fi

dnl Locate required external software
AC_PROG_CC

dnl Blackbox requires ANSI C headers
AC_HEADER_STDC
if test "$ac_cv_header_stdc" = "no"; then
  AC_MSG_ERROR([Blackbox requires ANSI C headers.])
fi

AC_PROG_CXX
AC_PROG_INSTALL

dnl libbt shouldn't be shared by default (yet)
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AC_CHECK_PROGS([regex_cmd], [sed])
if test "x$regex_cmd" = "x"; then
  AC_MSG_ERROR([error. sed is required to build the default menu file.])
fi

AC_CHECK_PROGS([gencat_cmd], [gencat])
if test "x$gencat_cmd" = "x"; then
  NLS=
fi

dnl Blackbox uses C++
AC_LANG([C++])

dnl Compiler specific options
if test "x$GXX" != "xyes"; then
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

dnl Check if some required functions live in the C library, or in an
dnl external library

ICONV=
AC_CHECK_FUNCS([iconv_open],
               [],
               [AC_CHECK_LIB([iconv],
                             [iconv_open],
                             [ICONV="-liconv"],
                             [AC_CHECK_LIB([iconv],
					   [libiconv_open],
					   [ICONV="-liconv"],
					   [AC_MSG_ERROR([Blackbox requires iconv(3) support.])])
			     ])
               ])
AC_SUBST(ICONV)

dnl check if we are using GNU libiconv
AC_MSG_CHECKING([for GNU libiconv])
AC_COMPILE_IFELSE([
#include <sys/types.h>
#include <iconv.h>

int main(int, char **) {
    iconv_t cd;
    const char *inp;
    char *outp;
    size_t inbytes, outbytes;
    iconv(cd, &inp, &inbytes, &outp, &outbytes);
    return 0;
}
],
 		  [AC_DEFINE([HAVE_GNU_LIBICONV], [1],
                             [Define to 1 when using GNU libiconv])
		   AC_MSG_RESULT([yes])],
		  [AC_MSG_RESULT([no, assuming POSIX compliance])]
                 )

LOCALE=
AC_CHECK_FUNCS([setlocale],
               [],
               [AC_CHECK_LIB([xpg4],
                             [setlocale],
                             [LOCALE="-lxpg4"],
                             [AC_MSG_ERROR([Blackbox requires setlocale(3) support.])])
               ])
AC_SUBST(LOCALE)

AC_MSG_CHECKING([for nl_langinfo])
AC_COMPILE_IFELSE([
#include <langinfo.h>

int main(int, char **)
{
    const char *x;
    x = nl_langinfo(CODESET);
    return 0;
}
],
                  [AC_DEFINE([HAVE_NL_LANGINFO], [1],
                             [Define to 1 if you system has nl_langinfo(3)])
                   AC_MSG_RESULT([yes])],
                  [AC_MSG_RESULT([no])]
                 )

dnl needed for some X11 libs
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

dnl Check for SHAPE extension support and proper library files.
AC_MSG_CHECKING([whether to build support for the SHAPE extension])
AC_ARG_ENABLE([shape],
              AC_HELP_STRING([--enable-shape],
	                     [enable support of the SHAPE extension @<:@default=yes@:>@]),
	      [SHAPE="$enableval"],
	      [SHAPE=yes])
AC_MSG_RESULT([$SHAPE])

if test "x$SHAPE" = "xyes"; then
  AC_CHECK_LIB([Xext], [XShapeCombineShape], [SHAPE=yes], [SHAPE=no])
  
  if test "x$SHAPE" = "xyes"; then
    save_LIBS="$LIBS"
      
    if echo "$LIBS" | grep -q Xext 2>&1 >/dev/null; then
      LIBS="$LIBS -lXext"
    fi
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
  else
    SHAPE=
  fi
else
  SHAPE=
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
  else
    MITSHM=
  fi
else
  MITSHM=
fi
AC_SUBST([MITSHM])

LIBS="$LIBS -lX11"

dnl Check for Xft libraries
AC_MSG_CHECKING([whether to build support for Xft])
AC_ARG_ENABLE([xft],
              AC_HELP_STRING([--enable-xft],
                             [enable support for the Xft library @<:@default=yes@:>@]),
	      [XFT="$enableval"],
	      [XFT=yes])
AC_MSG_RESULT([$XFT])

if test "x$XFT" = "xyes"; then
  PKG_CHECK_MODULES([xft],
                    [xft >= 2.0.0], 
                    [XFT="-DXFT"
                     XFT_PKGCONFIG="xft >= 2.0.0"
                     CXXFLAGS="$CXXFLAGS $xft_CFLAGS"
                     LIBS="$LIBS $xft_LIBS"],
                    [XFT=
                     XFT_PKGCONFIG=])
else
  XFT=
  XFT_PKGCONFIG=
fi
AC_SUBST([XFT])
AC_SUBST([XFT_PKGCONFIG])

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

dnl Output files
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
version.h
Makefile
data/Makefile
data/styles/Makefile
doc/Makefile
doc/fr_FR/Makefile
doc/ja_JP/Makefile
doc/nl_NL/Makefile
doc/sl_SI/Makefile
lib/Makefile
lib/libbt.pc
src/Makefile
util/Makefile
])
AC_OUTPUT

dnl Print results
AC_MSG_RESULT([])
AC_MSG_RESULT([	$PACKAGE version $VERSION configured successfully.])
AC_MSG_RESULT([])
AC_MSG_RESULT([Using '$prefix' for installation.])
AC_MSG_RESULT([Using '$CXX' for C++ compiler.])
AC_MSG_RESULT([Building with '$CPPFLAGS $CXXFLAGS' for C++ compiler flags.])
AC_MSG_RESULT([Building with '$LDFLAGS $LIBS' libraries.])
AC_MSG_RESULT([])
