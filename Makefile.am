# Makefile.am for Blackbox - an X11 Window manager

ACLOCAL_AMFLAGS		= -I m4

SUBDIRS			= data \
			  po \
			  doc \
			  lib \
			  src \
			  util

EXTRA_DIST		= gettext.h \
			  INSTALL \
			  COPYING \
			  AUTHORS \
			  AUTHORS.in \
			  NEWS \
			  NEWS.in \
			  README \
			  README.in \
			  THANKS \
			  TODO \
			  ChangeLog

dist_noinst_SCRIPTS	= autogen.sh

ChangeLog:
	( \
	  echo -e "# created with git log --stat=76 -M -C|fmt -sct -w80\n" ; \
	  git log --stat=76 -M -C|fmt -sct -w80 ; \
	)>$@

AUTHORS.in:
	( \
	  echo "\@PACKAGE\@ -- authors file.  \@DATE\@" ; \
	  echo "" ; \
	  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}'|tac \
	)>$@

README: README.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

NEWS: NEWS.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

AUTHORS: AUTHORS.in
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g' $< >$@

