noinst_LTLIBRARIES = 			\
	libblpmdbus.la

libblpmdbus_la_SOURCES =		\
	blpm-dbus.c			\
	blpm-dbus.h			\
	blpm-dbus-monitor.c		\
	blpm-dbus-monitor.h		\
	blpm-unique.c			\
	blpm-unique.h			\
	blpm-dbus-marshal.c		\
	blpm-dbus-marshal.h		\
	org.blade.unique.h
	
libblpmdbus_la_CFLAGS =			\
	$(GLIB_CFLAGS)			\
	$(LIBBLADEUTIL_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)

libblpmdbus_la_LIBADD =			\
	$(GLIB_LIBS)			\
	$(DBUS_GLIB_LIBS)		\
	$(DBUS_LIBS)

if MAINTAINER_MODE

BUILT_SOURCES =				\
	blpm-dbus-marshal.c		\
	blpm-dbus-marshal.h		\
	org.blade.unique.h

blpm-dbus-marshal.c: blpm-dbus-marshal.list
	echo "#include \"blpm-dbus-marshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=_blpm_dbus_marshal --body >> $@

blpm-dbus-marshal.h: blpm-dbus-marshal.list
	glib-genmarshal $< --prefix=_blpm_dbus_marshal --header > $@

org.blade.unique.h: org.blade.unique.xml
	dbus-binding-tool --mode=glib-server --prefix=xfce_unique $< >$@

endif

EXTRA_DIST =				\
	blpm-dbus-marshal.list

DISTCLEANFILES =			\
	$(BUILT_SOURCES)