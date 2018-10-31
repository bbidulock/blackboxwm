# Makefile.am for Blackbox - an X11 Window manager

ACLOCAL_AMFLAGS		= -I m4

SUBDIRS			= data po doc lib src util

EXTRA_DIST = INSTALL COPYING AUTHORS NEWS README README.md README.md.in THANKS TODO ChangeLog RELEASE RELEASE.md COMPLIANCE

dist_noinst_SCRIPTS = gennews.sh autogen.sh configure.sh rebuild.sh dist.sh

DISTCLEANFILES = ChangeLog AUTHORS NEWS README README.md README.html README.txt RELEASE RELEASE.html RELEASE.txt

dist-hook:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		chmod u+w $(distdir)/{ChangeLog,AUTHORS,NEWS} ; \
		( cd $(srcdir) ; \
		  echo -e "# created with git log --stat=76 -M -C -500|fmt -sct -w80\n" ; \
		  git log --stat=76 -M -C -500|fmt -sct -w80 \
		)|head -10000>$(distdir)/ChangeLog ; \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- authors file.  $(DATE)" ; \
		  echo "" ; \
		  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}' \
		)>$(distdir)/AUTHORS ; \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- history of user-visible changes.  $(DATE)" ; \
		  PACKAGE="$(PACKAGE)" $(srcdir)/gennews.sh \
		)>$(distdir)/NEWS ; \
	fi ; \
	chmod u+w $(distdir)/{README.md,README} ; \
	sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g;s:\@BRANCH\@:$(BRANCH):g' $(distdir)/README.md.in >$(distdir)/README.md ; \
	cat $(distdir)/README.md | fmt -sct -w80 -g72 >$(distdir)/README

ChangeLog:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		( cd $(srcdir) ; \
		  echo -e "# created with git log --stat=76 -M -C -500|fmt -sct -w80\n" ; \
		  git log --stat=76 -M -C -500|fmt -sct -w80 \
		)|head -10000>$@ ; \
	else touch $@ ; fi

AUTHORS:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- authors file.  $(DATE)" ; \
		  echo "" ; \
		  git log|grep '^Author:'|awk '{if(!authors[$$0]){print$$0;authors[$$0]=1;}}' \
		)>$@ ; \
	else touch $@ ; fi

NEWS:
	$(AM_V_GEN)if test -x "`which git 2>/dev/null`" -a -d "$(srcdir)/.git" ; then \
		( cd $(srcdir) ; \
		  echo "$(PACKAGE) -- history of user-visible changes.  $(DATE)" ; \
		  PACKAGE="$(PACKAGE)" $(srcdir)/gennews.sh \
		)>$@ ; \
	else touch $@ ; fi

README.md: README.md.in
	$(AM_V_GEN)sed -r -e 's:\@PACKAGE\@:$(PACKAGE):g;s:\@VERSION\@:$(VERSION):g;s:\@DATE\@:$(DATE):g;s:\@BRANCH\@:$(BRANCH):g' $< >$@

README: README.md
	$(AM_V_GEN)cat $< | fmt -sct -w80 -g72 >$@

README.html: README.md
	$(AM_V_GEN)$(MARKDOWN) $< >$@

README.txt: README.html
	$(AM_V_GEN)$(LINKS) -dump -width 80 -nolist $< | sed -r -e 's,^[[:space:]]*,,' >$@

RELEASE: RELEASE.md
	$(AM_V_GEN)cat $< | fmt -sct -w80 -g72 >$@

RELEASE.html: RELEASE.md
	$(AM_V_GEN)$(MARKDOWN) $< >$@

RELEASE.txt: RELEASE.html
	$(AM_V_GEN)$(LINKS) -dump -width 80 -nolist $< | sed -r -e 's,^[[:space:]]*,,' >$@

EXTRA_DIST += gettext.h

