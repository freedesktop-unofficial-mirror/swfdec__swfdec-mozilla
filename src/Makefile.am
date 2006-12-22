
plugindir = $(libdir)/$(MOZILLA_DIR)/plugins

plugin_LTLIBRARIES = libswfdecmozilla.la

libswfdecmozilla_la_CFLAGS = $(GLOBAL_CFLAGS) $(MOZILLA_CFLAGS) $(SWFDEC_CFLAGS)
libswfdecmozilla_la_LDFLAGS = \
	-module -avoid-version -export-symbols $(srcdir)/plugin.symbols \
	$(MOZILLA_LIBS) $(SWFDEC_LIBS)

libswfdecmozilla_la_SOURCES = \
	plugin.c \
	plugin_x11.c \
	swfmoz_loader.c \
	swfmoz_player.c

noinst_HEADERS = \
	plugin_x11.h \
	swfmoz_loader.h \
	swfmoz_player.h

EXTRA_DIST = \
	plugin.symbols
