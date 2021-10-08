interfaces =					\
	blpm-settings.ui			\
	acme.ui

interfaces_h =					\
	$(interfaces:.ui=_ui.h)

if MAINTAINER_MODE

BUILT_SOURCES = $(interfaces_h)

blpm-settings_ui.h: blpm-settings.ui
	blxo-csource --static --strip-comments --strip-content --name=blpm_settings_ui $< > $@

acme_ui.h: acme.ui
	blxo-csource --static --strip-comments --strip-content --name=acme_ui $< > $@

endif

CLEANFILES =					\
	$(BUILT_SOURCES)

EXTRA_DIST =					\
	$(interfaces)				\
	$(interfaces_h)
