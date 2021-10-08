bin_PROGRAMS = blade-pm-settings

blade_pm_settings_SOURCES = 				\
	blade-pm-dbus-client.h			\
	blpm-settings-main.c					\
	blpm-settings.c						\
	blpm-settings.h						\
	$(top_srcdir)/common/blpm-config.h				\
	$(top_srcdir)/common/blpm-enum.h				\
	$(top_srcdir)/common/blpm-enum-glib.h

blade_pm_settings_CFLAGS =				\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/common					\
	-I$(top_srcdir)/src					\
	-I$(top_srcdir)/data					\
	-I$(top_srcdir)/libdbus                 		\
	-DLOCALEDIR=\"$(localedir)\"				\
	-DG_LOG_DOMAIN=\"blade-pm-settings\" 	\
	-export-dynamic						\
	$(MODULE_FLAGS)						\
	$(GTK_CFLAGS)						\
	$(GLIB_CFLAGS)						\
	$(GTHREAD_CFLAGS)					\
	$(GMODULE_CFLAGS)					\
	$(DBUS_CFLAGS)						\
	$(DBUS_GLIB_CFLAGS)					\
	$(LIBBLADEUI_CFLAGS)					\
	$(LIBBLADEUTIL_CFLAGS)					\
	$(BLCONF_CFLAGS)					\
	$(UPOWER_CFLAGS)

blade_pm_settings_LDADD   =				\
	$(top_builddir)/libdbus/libblpmdbus.la  		\
	$(top_builddir)/common/libblpmcommon.la			\
	$(GTK_LIBS)						\
	$(GLIB_LIBS)						\
	$(GTHREAD_LIBS)						\
	$(GMODULE_LIBS)						\
	$(DBUS_LIBS)						\
	$(DBUS_GLIB_LIBS)					\
	$(LIBBLADEUI_LIBS)					\
	$(LIBBLADEUTIL_LIBS)					\
	$(BLCONF_LIBS)						\
	$(UPOWER_LIBS)


manpagedir = $(mandir)/man1

manpage_DATA = blade-pm-settings.1

if MAINTAINER_MODE

BUILT_SOURCES = 				\
	blade-pm-dbus-client.h

blade-pm-dbus-client.h: $(top_srcdir)/src/org.blade.Power.Manager.xml
	dbus-binding-tool --mode=glib-client --prefix=blpm_manager $< >$@

endif

#
# .desktop file
#
desktop_in_files = blade-pm-settings.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA =	$(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 					\
	blade-pm-dbus-client.h	\
	$(desktop_in_files)			\
	$(manpage_DATA)

DISTCLEANFILES =				\
	$(BUILT_SOURCES)			\
	$(desktop_DATA)
