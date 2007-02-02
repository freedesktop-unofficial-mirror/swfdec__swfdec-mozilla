

# this workaround is needed or autotools don't generate the .deps/*.Plo files correctly
swfdec_playback.c: swfdec_playback_$(AUDIO_TYPE).c Makefile
	cmp -s $(srcdir)/swfdec_playback_$(AUDIO_TYPE).c swfdec_playback.c \
	|| cp $(srcdir)/swfdec_playback_$(AUDIO_TYPE).c swfdec_playback.c

BUILT_SOURCES = swfdec_playback.c
CLEANFILES = swfdec_playback.c

plugindir = $(libdir)/$(MOZILLA_DIR)/plugins
plugin_LTLIBRARIES = libswfdecmozilla.la

libswfdecmozilla_la_CFLAGS = $(GLOBAL_CFLAGS) $(MOZILLA_CFLAGS) $(SWFDEC_CFLAGS) \
	$(AUDIO_CFLAGS) $(CAIRO_CFLAGS)
libswfdecmozilla_la_LDFLAGS = \
	-module -avoid-version -export-symbols $(srcdir)/plugin.symbols \
	$(MOZILLA_LIBS) $(SWFDEC_LIBS) $(AUDIO_LIBS) $(CAIRO_LIBS)

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
	plugin.symbols \
	swfdec_playback_alsa.c \
	swfdec_playback_none.c
