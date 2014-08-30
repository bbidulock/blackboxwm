#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([blackbox],[0.70.2.37],[http://github.com/bbidulock/blackboxwm])
AC_REVISION([0.70.2.37])
AC_CONFIG_SRCDIR([src/blackbox.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS

DATE='2014-08-29'
AC_SUBST([DATE])

# Initialize Automake
AM_INIT_AUTOMAKE([subdir-objects dist-bzip2 no-dist-gzip color-tests silent-rules tar-pax])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_SED
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AC_PROG_MKDIR_P
AC_ARG_VAR([LD], [Linker loader command])

dnl libbt shouldn't be shared by default (yet)
AC_DISABLE_SHARED
LT_PREREQ([2.4.2])
LT_INIT([])
LT_LANG([C++])
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

AM_GNU_GETTEXT_VERSION([0.19.2])
AM_GNU_GETTEXT([external])

AC_HEADER_STDC
AC_LANG([C++])

PKG_PROG_PKG_CONFIG

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h langinfo.h limits.h locale.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([gethostname gettimeofday memmove memset mkdir nl_langinfo putenv select setlocale sqrt strcasecmp strncasecmp strtol strtoul])

PKG_CHECK_MODULES([X11],[x11])
PKG_CHECK_MODULES([XEXT],[Xext], [],
	[enable_shape=no
	 enable_mitshm=no])

AC_ARG_ENABLE([shape],
    AC_HELP_STRING([--disable-shape],
	[Disable use of SHAPE extension @<:@default=auto@:>@]))
if test x$enable_shape != xno ; then
    AC_CHECK_LIB([Xext],[XShapeCombineShape],
	[AC_DEFINE([SHAPE],[1],[Define to enable SHAPE extension.])],
	[enable_shape=no])
fi

AC_ARG_ENABLE([mitshm],
    AC_HELP_STRING([--disable-mitshm],
	[Disable use of the MIT-SHM extension @<:@default=auto@:>@]))
if test x$enable_mitshm != xno ; then
    AC_CHECK_LIB([Xext],[XShmPutImage],
	[AC_DEFINE([MITSHM],[1],[Define to enable MIT-SHM extension.])],
	[enable_mitshm=no])
fi

AC_ARG_ENABLE([xft],
    AC_HELP_STRING([--disable-xft],
	[Disable use of XFT library @<:@default=auto@:>@]))
if test x$enable_xft != xno ; then
    PKG_CHECK_MODULES([XFT],[xft >= 2.0.0],
	[AC_DEFINE([XFT],[1],[Defint to enable XFT library.])],
	[enable_xft=no])
fi

AC_ARG_ENABLE([debug],
    AC_HELP_STRING([--enable-debug],
	[Enable use of verbose debugging code @<:@default=no@:>@]))
if test x$enable_debug = xyes ; then
    AC_DEFINE([DEBUG],[1],[Define to enable debugging code.])
fi

AC_ARG_ENABLE([debug-colorcache],
    AC_HELP_STRING([--enable-debug-colorcache],
	[Enable color cache debugging code @<:@default=no@:>@]))
if test x$enable_debug_colorcache = xyes ; then
    AC_DEFINE([COLORCACHE_DEBUG],[1],[Define to debug color cache code.])
fi

AC_ARG_ENABLE([debug-colortable],
    AC_HELP_STRING([--enable-debug-colortable],
	[Enable color table debugging code @<:@default=no@:>@]))
if test x$enable_debug_colortable = xyes ; then
    AC_DEFINE([COLORCACHE_DEBUG],[1],[Define to debug color table code.])
fi

AC_ARG_ENABLE([debug-focus],
    AC_HELP_STRING([--enable-debug-focus],
	[Enable focus debugging code @<:@default=no@:>@]))
if test x$enable_debug_focus = xyes ; then
    AC_DEFINE([FOCUS_DEBUG],[1],[Define to debug focus code.])
fi

AC_ARG_ENABLE([debug-fontcache],
    AC_HELP_STRING([--enable-debug-fontcache],
	[Enable font cache debugging code @<:@default=no@:>@]))
if test x$enable_debug_fontcache = xyes ; then
    AC_DEFINE([FONTCACHE_DEBUG],[1],[Define to debug font cache code.])
fi

AC_ARG_ENABLE([debug-mitshm],
    AC_HELP_STRING([--enable-debug-mitshm],
	[Enable MIT-SHM debugging code @<:@default=no@:>@]))
if test x$enable_debug_mitshm = xyes ; then
    AC_DEFINE([MITSHM_DEBUG],[1],[Define to debug MIT-SHM code.])
fi

AC_ARG_ENABLE([debug-pixmapcache],
    AC_HELP_STRING([--enable-debug-pixmapcache],
	[Enable pixmap cache debugging code @<:@default=no@:>@]))
if test x$enable_debug_pixmapcache = xyes ; then
    AC_DEFINE([FONTCACHE_DEBUG],[1],[Define to debug pixmap cache code.])
fi

AC_ARG_ENABLE([print-sizes],
    AC_HELP_STRING([--enable-print-sizes],
	[Enable printing of structure sizes on startup. @<:@default=no@:>@]))
if test x$enable_print_sizes = xyes ; then
    AC_DEFINE([PRINT_SIZES],[1],[Define to print structure sizes on startup.])
fi

AC_ARG_ENABLE([fullscreen],
    AC_HELP_STRING([--disable-fullscreen],
	[Disable fullscreen item in window menu. @<:@default=enabled@:>@]))
if test x$enable_fullscreen != xno ; then
    AC_DEFINE([WITH_FULLSCREEN],[1],[Define to add fullscreen item to window menu.])
fi

AM_CONDITIONAL([USE_NLS],[test x$enable_nls != xno])

text "x$prefix" = xNONE && prefix="$ac_default_prefix"
LOCDIR="$localedir"
eval "LOCDIR=\"$LOCDIR\""
eval "LOCDIR=\"$LOCDIR\""
eval "LOCDIR=\"$LOCDIR\""
AC_SUBST([LOCDIR])

AC_CONFIG_FILES([version.h
		 Makefile
		 po/Makefile.in
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
		 util/Makefile])
AC_OUTPUT

