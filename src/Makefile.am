bin_PROGRAMS = blade-pm

blade_pm_SOURCES =                   \
	$(BUILT_SOURCES)			\
	blpm-main.c				\
	blpm-manager.c				\
	blpm-manager.h				\
	blpm-power.c				\
	blpm-power.h				\
	blpm-battery.c				\
	blpm-battery.h				\
	blpm-blconf.c				\
	blpm-blconf.h				\
	blpm-console-kit.c			\
	blpm-console-kit.h			\
	blpm-systemd.c				\
	blpm-systemd.h				\
	egg-idletime.c				\
	egg-idletime.h				\
	blpm-backlight.c			\
	blpm-backlight.h			\
	blpm-kbd-backlight.c			\
	blpm-kbd-backlight.h			\
	blpm-dpms.c				\
	blpm-dpms.h				\
	blpm-button.c				\
	blpm-button.h				\
	blpm-network-manager.c			\
	blpm-network-manager.h			\
	blpm-inhibit.c				\
	blpm-inhibit.h				\
	blpm-notify.c				\
	blpm-notify.h				\
	blpm-polkit.c				\
	blpm-polkit.h				\
	blpm-errors.c				\
	blpm-errors.h				\
	blpm-suspend.c				\
	blpm-suspend.h				\
	../bar-plugins/power-manager-plugin/power-manager-button.c	\
	../bar-plugins/power-manager-plugin/power-manager-button.h	\
	../bar-plugins/power-manager-plugin/scalemenuitem.c			\
	../bar-plugins/power-manager-plugin/scalemenuitem.h

blade_pm_CFLAGS =                    \
	-I$(top_srcdir)                         \
	-I$(top_srcdir)/common                  \
	-I$(top_srcdir)/libdbus                 \
	-DLOCALEDIR=\"$(localedir)\"            \
	-DG_LOG_DOMAIN=\"blade-pm\"  \
	-DSYSCONFDIR=\"$(sysconfdir)\"          \
	-DSBINDIR=\"$(sbindir)\"                \
	-DUPOWER_ENABLE_DEPRECATED              \
	-DXFPM_SUSPEND_HELPER_CMD=\"$(prefix)/bin/xfce4-pm-helper\" \
	-DXFPM_SYSTRAY							\
	$(GOBJECT_CFLAGS)                       \
	$(GTHREAD_CFLAGS)                       \
	$(DBUS_GLIB_CFLAGS)                     \
	$(LIBBLADEUI_CFLAGS)                    \
	$(BLCONF_CFLAGS)                        \
	$(LIBNOTIFY_CFLAGS)                     \
	$(UPOWER_CFLAGS)                        \
	$(XRANDR_CFLAGS)			\
	$(DPMS_CFLAGS)           		\
	$(PLATFORM_CPPFLAGS)			\
	$(PLATFORM_CFLAGS)

blade_pm_LDFLAGS =			\
	-no-undefined				\
	$(PLATFORM_LDFLAGS)

blade_pm_LDADD =                     \
	$(top_builddir)/common/libblpmcommon.la \
	$(top_builddir)/libdbus/libblpmdbus.la  \
	$(GOBJECT_LIBS)                         \
	$(GTHREAD_LIBS)                         \
	$(DBUS_GLIB_LIBS)                       \
	$(LIBBLADEUI_LIBS)                      \
	$(BLCONF_LIBS)                          \
	$(LIBNOTIFY_LIBS)                       \
	$(UPOWER_LIBS)                          \
	$(XRANDR_LIBS)				\
	$(DPMS_LIBS)

if ENABLE_POLKIT

sbin_PROGRAMS = blpm-power-backlight-helper     \
	   xfce4-pm-helper

blpm_power_backlight_helper_SOURCES =           \
       blpm-backlight-helper.c

blpm_power_backlight_helper_LDADD =             \
       $(GLIB_LIBS)                             \
       -lm

blpm_power_backlight_helper_CFLAGS =            \
        $(GLIB_CFLAGS)                          \
	$(PLATFORM_CPPFLAGS)			\
	$(PLATFORM_CFLAGS)

xfce4_pm_helper_SOURCES =  \
	blpm-pm-helper.c

xfce4_pm_helper_CFLAGS =   \
	-I$(top_srcdir)        \
	$(GLIB_CFLAGS)         \
	$(PLATFORM_CPPFLAGS)   \
	$(PLATFORM_CFLAGS)     \
	$(LIBBLADEUTIL_CFLAGS)

xfce4_pm_helper_LDFLAGS = \
	$(LIBBLADEUTIL_LIBS)  \
	$(GLIB_LIBS)

polkit_policydir = $(datadir)/polkit-1/actions
polkit_policy_DATA =					\
	org.blade.power.policy

# You will need a recent intltool or the patch from this bug
# http://bugzilla.gnome.org/show_bug.cgi?id=462312
@INTLTOOL_POLICY_RULE@

.in2.in:
	sed "s|[@]sbindir@|${sbindir}|" $< > $@

endif

manpagedir = $(mandir)/man1

manpage_DATA = blade-pm.1

BUILT_SOURCES =					\
	blade-pm-dbus-server.h	\
	blade-pm-dbus-client.h	\
	blpm-marshal.c                          \
        blpm-marshal.h				\
	org.freedesktop.PowerManagement.h       \
	org.freedesktop.PowerManagement.Inhibit.h

if MAINTAINER_MODE

blpm-marshal.c: blpm-marshal.list
	echo "#include \"blpm-marshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=_blpm_marshal --body >> $@

blpm-marshal.h: blpm-marshal.list
	glib-genmarshal $< --prefix=_blpm_marshal --header > $@

blade-pm-dbus-server.h: $(srcdir)/org.blade.Power.Manager.xml
	dbus-binding-tool --mode=glib-server --prefix=blpm_manager $< >$@

blade-pm-dbus-client.h: $(srcdir)/org.blade.Power.Manager.xml
	dbus-binding-tool --mode=glib-client --prefix=blpm_manager $< >$@

org.freedesktop.PowerManagement.h: $(srcdir)/org.freedesktop.PowerManagement.xml
	dbus-binding-tool --mode=glib-server --prefix=blpm_power $< >$@

org.freedesktop.PowerManagement.Inhibit.h: $(srcdir)/org.freedesktop.PowerManagement.Inhibit.xml
	dbus-binding-tool --mode=glib-server --prefix=blpm_inhibit $< >$@

endif

@INTLTOOL_DESKTOP_RULE@
autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = blade-pm.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

EXTRA_DIST =                                    \
	blpm-marshal.list			\
        $(autostart_in_files)                   \
        org.blade.power.policy.in2              \
        $(manpage_DATA)

DISTCLEANFILES =                                \
        $(BUILT_SOURCES)                        \
        blade-pm.desktop             \
        org.blade.power.policy                  \
        org.blade.power.policy.in
