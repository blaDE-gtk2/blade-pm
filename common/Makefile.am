noinst_LTLIBRARIES =        \
        libblpmcommon.la

libblpmcommon_la_SOURCES =  \
	$(BUILT_SOURCES)        \
	blpm-common.c           \
	blpm-common.h           \
	blpm-brightness.c       \
	blpm-brightness.h       \
	blpm-debug.c            \
	blpm-debug.h            \
	blpm-icons.h            \
	blpm-power-common.c     \
	blpm-power-common.h     \
	blpm-enum.h             \
	blpm-enum-glib.h

libblpmcommon_la_CFLAGS =           \
	-I$(top_srcdir)                 \
	-DSBINDIR=\"$(sbindir)\"        \
	$(GTK_CFLAGS)                   \
	$(GLIB_CFLAGS)                  \
	$(LIBBLADEUTIL_CFLAGS)          \
	$(DBUS_GLIB_CFLAGS)             \
	$(UPOWER_CFLAGS)

libblpmcommon_la_LIBADD =           \
	$(GTK_LIBS)                     \
	$(GLIB_LIBS)                    \
	$(XRANDR_LIBS)                  \
	$(X11_LIBS)                     \
	$(DBUS_GLIB_LIBS)               \
	$(UPOWER_LIBS)

blpm_glib_headers =                \
        $(srcdir)/blpm-enum-glib.h

BUILT_SOURCES =       \
	blpm-enum-types.c \
	blpm-enum-types.h

if MAINTAINER_MODE

blpm-enum-types.h: $(blpm_glib_headers)
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef _XFPM_ENUM_TYPES_H\n#define _XFPM_ENUM_TYPES_H\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define XFPM_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* _XFPM_ENUM_TYPES_H__ */" \
		$(blpm_glib_headers) ) >> xgen \
	&& (cmp -s xgen blpm-enum-types.h || cp xgen blpm-enum-types.h ) \
	&& rm -f xgen xgen~

blpm-enum-types.c: blpm-enum-types.h
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include <blpm-enum-types.h>\n" \
		--fhead "#include \"blpm-enum-glib.h\"\n\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {"\
		--vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
		$(blpm_glib_headers) ) > xgen \
	&& cp xgen blpm-enum-types.c  \
	&& rm -f xgen xgen~

endif
