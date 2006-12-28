
plugindir = $(libdir)/$(MOZILLA_DIR)/plugins

plugin_LTLIBRARIES = libswfdecmozilla.la

libswfdecmozilla_la_CFLAGS = $(GLOBAL_CFLAGS) $(MOZILLA_CFLAGS) $(SWFDEC_CFLAGS)
libswfdecmozilla_la_LDFLAGS = \
	-module -avoid-version -export-symbols $(srcdir)/plugin.symbols \
	$(MOZILLA_LIBS) $(SWFDEC_LIBS)

libswfdecmozilla_la_SOURCES = \
	plugin.c \
	plugin_x11.c \
	swfdec_playback.c \
	swfdec_source.c \
	swfmoz_loader.c \
	swfmoz_player.c

noinst_HEADERS = \
	plugin.h \
	plugin_x11.h \
	swfdec_playback.h \
	swfdec_source.h \
	swfmoz_loader.h \
	swfmoz_player.h

EXTRA_DIST = \
	plugin.symbols
