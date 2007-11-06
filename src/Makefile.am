plugindir = $(PLUGIN_DIR)
plugin_LTLIBRARIES = libswfdecmozilla.la

libswfdecmozilla_la_CFLAGS = $(GLOBAL_CFLAGS) $(MOZILLA_CFLAGS) $(SWFDEC_CFLAGS) \
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"

libswfdecmozilla_la_LDFLAGS = \
	-module -avoid-version -export-symbols $(srcdir)/plugin.symbols \
	$(SWFDEC_LIBS)

libswfdecmozilla_la_SOURCES = \
	plugin.c \
	plugin_x11.c \
	swfmoz_dialog.c \
	swfmoz_loader.c \
	swfmoz_player.c

noinst_HEADERS = \
	plugin.h \
	plugin_x11.h \
	swfmoz_dialog.h \
	swfmoz_loader.h \
	swfmoz_player.h

EXTRA_DIST = \
	plugin.symbols
