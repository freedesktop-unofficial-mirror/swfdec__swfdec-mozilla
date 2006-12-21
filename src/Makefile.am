
plugindir = $(libdir)/$(MOZILLA_DIR)/plugins

plugin_LTLIBRARIES = libswfdecmozilla.la

libswfdecmozilla_la_CFLAGS = $(GLOBAL_CFLAGS) $(MOZILLA_CFLAGS) $(SWFDEC_CFLAGS)
libswfdecmozilla_la_LDFLAGS = -module -avoid-version $(MOZILLA_LIBS) $(SWFDEC_LIBS)
libswfdecmozilla_la_LIBADD = -lXt

libswfdecmozilla_la_SOURCES = \
	plugin.c \
	swfmoz_player.c

noinst_HEADERS = \
	swfmoz_player.h
