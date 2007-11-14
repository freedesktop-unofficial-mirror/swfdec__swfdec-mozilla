/* Swfdec Mozilla Plugin
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <cairo-xlib.h>

#include <npapi.h>
#include <npupp.h>

#include "swfmoz_loader.h"
#include "swfmoz_player.h"
#include "plugin_x11.h"

NPNetscapeFuncs mozilla_funcs;


/*** forward declarations for plugin API ***/

void
plugin_get_url (NPP instance, const char *url, const char *target)
{
  CallNPN_GetURLProc (mozilla_funcs.geturl, instance, url, target);
}

void
plugin_get_url_notify (NPP instance, const char *url,
    const char *target, void *data)
{
  CallNPN_GetURLNotifyProc (mozilla_funcs.geturlnotify, instance, 
      url, target, data);
}

void
plugin_post_url (NPP instance, const char *url,
    const char *target, const char *data, guint data_len)
{
  CallNPN_PostURLProc (mozilla_funcs.posturl, instance, 
      url, target, data_len, data ? data : "", FALSE);
}

void
plugin_post_url_notify (NPP instance, const char *url,
    const char *target, const char *data, guint data_len, void *user_data)
{
  CallNPN_PostURLNotifyProc (mozilla_funcs.posturlnotify, instance, 
      url, target, data_len, data ? data : "", FALSE, user_data);
}

void
plugin_destroy_stream (NPP instance, NPStream *stream)
{
  CallNPN_DestroyStreamProc (mozilla_funcs.destroystream, instance, stream, NPRES_DONE);
}

void
plugin_invalidate_rect (NPP instance, NPRect *rect)
{
  CallNPN_InvalidateRectProc (mozilla_funcs.invalidaterect, instance, rect);
}

gboolean
plugin_push_allow_popups (NPP instance, gboolean allow)
{
  return CallNPN_PushPopupsEnabledStateProc (mozilla_funcs.pushpopupsenabledstate, instance, allow);
}

gboolean
plugin_pop_allow_popups (NPP instance)
{
  return CallNPN_PopPopupsEnabledStateProc (mozilla_funcs.poppopupsenabledstate, instance);
}

/*** plugin implementation ***/

char *
NP_GetMIMEDescription (void)
{
  return "application/x-shockwave-flash:swf:Adobe Flash movie;application/futuresplash:spl:FutureSplash movie";
}

NPError 
NP_GetValue (void* reserved, NPPVariable var, void* out)
{
  char **val;

  if (out == NULL)
    return NPERR_INVALID_PARAM;
	          
  val = (char**)out;
		      
  switch (var) {
    case NPPVpluginNameString:
      *val = "Shockwave Flash";
      break;
    case NPPVpluginDescriptionString:
      /* FIXME: find a way to encode the Swfdec version without breaking stupid JS scripts */
      *val = "Shockwave Flash 9.0 r100";
      break;
    case NPPVpluginNeedsXEmbed:
      *((PRBool*) val) = PR_TRUE;
      break;
    default:
      return NPERR_INVALID_PARAM;
      break;
  }
  return NPERR_NO_ERROR;
}

/* This mess is unfortunately necessary */
#define PLUGIN_FILE PLUGIN_DIR G_DIR_SEPARATOR_S "libswfdecmozilla." G_MODULE_SUFFIX
G_MODULE_EXPORT gboolean
swfdec_mozilla_make_sure_this_thing_stays_in_memory (void)
{
  static gboolean inited = FALSE;
  GModule *module;
  gpointer check;
    
  if (inited)
    return TRUE;
  if (!g_module_supported ())
    return FALSE;
  module = g_module_open (PLUGIN_FILE, 0);
  if (module == NULL)
    return FALSE;
  /* now load this function name to be sure it we've loaded ourselves */
  if (!g_module_symbol (module, "swfdec_mozilla_make_sure_this_thing_stays_in_memory", &check) ||
      check != swfdec_mozilla_make_sure_this_thing_stays_in_memory) {
    g_module_close (module);
    return FALSE;
  }
  g_module_make_resident (module);
  g_module_close (module);
  inited = TRUE;
  return TRUE;
}

static NPError
plugin_new (NPMIMEType mime_type, NPP instance,
    uint16_t mode, int16_t argc, char *argn[], char *argv[],
    NPSavedData * saved)
{
  int i;

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  if (!swfdec_mozilla_make_sure_this_thing_stays_in_memory ()) {
    g_printerr ("Ensuring the plugin stays in memory did not work.\n"
	        "This happens when the plugin was copied from its installed location at " PLUGIN_FILE ".\n"
		"Please use the --with-plugin-dir configure option to install it into a different place.\n");
    return NPERR_INVALID_INSTANCE_ERROR;
  }
#if 0
  /* see https://bugzilla.mozilla.org/show_bug.cgi?id=137189 for why this doesn't work
   * probably needs user agent sniffing to make this work correctly (iff gecko 
   * implements it
   */
  if (CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
  	NPPVpluginWindowBool, (void *) PR_FALSE))
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  if (CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
	NPPVpluginTransparentBool, (void *) PR_TRUE))
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
#endif

  /* Init functioncalling (even g_type_init) gets postponed until we know we
   * won't be unloaded, i.e. NPPVpluginKeepLibraryInMemory was successful */
  swfdec_init ();
  instance->pdata = swfmoz_player_new (instance, FALSE);

  /* set the properties we support */
  /* FIXME: figure out how variables override each other */
  for (i = 0; i < argc; i++) {
    if (argn[i] == NULL)
      continue;
    if (g_ascii_strcasecmp (argn[i], "flashvars") == 0) {
      if (argv[i])
	swfmoz_player_add_variables (instance->pdata, argv[i]);
    } else if (g_ascii_strcasecmp (argn[i], "bgcolor") == 0) {
      GdkColor color;
      if (gdk_color_parse (argv[i], &color)) {
	swfdec_player_set_background_color (SWFMOZ_PLAYER (instance->pdata)->player, 
	    0xFF000000 | (color.red / 0x101 << 16) | 
	    (color.green / 0x101 << 8) | (color.blue / 0x101));
      }
    } else if (g_ascii_strcasecmp (argn[i], "src") == 0) {
    } else if (g_ascii_strcasecmp (argn[i], "type") == 0) {
    } else if (g_ascii_strcasecmp (argn[i], "width") == 0) {
    } else if (g_ascii_strcasecmp (argn[i], "height") == 0) {
    } else if (g_ascii_strcasecmp (argn[i], "scale") == 0) {
      SwfdecScaleMode scale;
      if (g_ascii_strcasecmp (argv[i], "noborder") == 0) {
	scale = SWFDEC_SCALE_NO_BORDER;
      } else if (g_ascii_strcasecmp (argv[i], "exactfit") == 0) {
	scale = SWFDEC_SCALE_EXACT_FIT;
      } else if (g_ascii_strcasecmp (argv[i], "noscale") == 0) {
	scale = SWFDEC_SCALE_NONE;
      } else {
	scale = SWFDEC_SCALE_SHOW_ALL;
      }
      swfdec_player_set_scale_mode (SWFMOZ_PLAYER (instance->pdata)->player, scale);
    } else if (g_ascii_strcasecmp (argn[i], "salign") == 0) {
      struct {
	const char *	name;
	SwfdecAlignment	align;
      } possibilities[] = {
	{ "t", SWFDEC_ALIGNMENT_TOP },
	{ "l", SWFDEC_ALIGNMENT_LEFT },
	{ "r", SWFDEC_ALIGNMENT_RIGHT },
	{ "b", SWFDEC_ALIGNMENT_BOTTOM },
	{ "tl", SWFDEC_ALIGNMENT_TOP_LEFT },
	{ "tr", SWFDEC_ALIGNMENT_TOP_RIGHT },
	{ "bl", SWFDEC_ALIGNMENT_BOTTOM_LEFT },
	{ "br", SWFDEC_ALIGNMENT_BOTTOM_RIGHT }
      };
      SwfdecAlignment align = SWFDEC_ALIGNMENT_CENTER;
      guint i;

      for (i = 0; i < G_N_ELEMENTS (possibilities); i++) {
	if (g_ascii_strcasecmp (argv[i], possibilities[i].name) == 0) {
	  align = possibilities[i].align;
	  break;
	}
      }
      swfdec_player_set_alignment (SWFMOZ_PLAYER (instance->pdata)->player, align);
    } else {
      g_printerr ("Unsupported movie property %s with value \"%s\"\n", 
	  argn[i], argv[i] ? argv[i] : "(null)");
    }
  }

  return NPERR_NO_ERROR;
}

NPError 
plugin_destroy (NPP instance, NPSavedData **save)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  plugin_x11_teardown (instance->pdata);
  swfmoz_player_remove (instance->pdata);
  instance->pdata = NULL;
  return NPERR_NO_ERROR;
}

static NPError 
plugin_new_stream (NPP instance, NPMIMEType type, NPStream* stream, 
    NPBool seekable, uint16* stype)
{
  SwfdecLoader *loader;

  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  loader = swfmoz_player_add_stream (instance->pdata, stream);
  if (loader == NULL)
    return NPERR_INVALID_URL;
  g_object_ref (loader);
  stream->pdata = loader;
  if (stype)
    *stype = NP_ASFILE;
  return NPERR_NO_ERROR;
}

static NPError 
plugin_destroy_stream_cb (NPP instance, NPStream* stream, NPReason reason)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;
  if (!SWFMOZ_IS_LOADER (stream->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  swfmoz_loader_ensure_open (stream->pdata);
  swfdec_loader_eof (stream->pdata);
  SWFMOZ_LOADER (stream->pdata)->stream = NULL;
  g_object_unref (stream->pdata);
  return NPERR_NO_ERROR;
}

static int32
plugin_write_ready (NPP instance, NPStream* stream)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return -1;
  if (!SWFMOZ_IS_LOADER (stream->pdata))
    return -1;

  return G_MAXINT32;
}

static int32
plugin_write (NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
  SwfdecBuffer *new;
  
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return -1;
  if (!SWFMOZ_IS_LOADER (stream->pdata))
    return -1;

  new = swfdec_buffer_new ();
  new->length = len;
  new->data = g_memdup (buffer, len);
  swfmoz_loader_ensure_open (stream->pdata);
  swfdec_loader_push (stream->pdata, new);
  return len;
}

static void
plugin_stream_as_file (NPP instance, NPStream *stream, const char *filename)
{
  SwfmozLoader *loader;

  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return;
  if (!SWFMOZ_IS_LOADER (stream->pdata))
    return;

  loader = stream->pdata;
  g_free (loader->cache_file);
  loader->cache_file = g_strdup (filename);
}

static NPError 
plugin_set_window (NPP instance, NPWindow *window)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  if (window) {
    plugin_x11_setup_windowed (instance->pdata, (Window) window->window, 
	window->x, window->y, window->width, window->height);
  } else {
    plugin_x11_teardown (instance->pdata);
  }
  return NPERR_NO_ERROR;
}

static int16
plugin_handle_event (NPP instance, void *eventp)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return FALSE;

  /* FIXME: implement */
  return FALSE;
}

static void
plugin_url_notify (NPP instance, const char* url, NPReason reason, void* notifyData)
{
  SwfdecLoader *loader = SWFDEC_LOADER (notifyData);

  if (reason == NPRES_NETWORK_ERR) {
    swfdec_loader_error (loader, "Network error");
  } else if (reason == NPRES_USER_BREAK) {
    swfdec_loader_error (loader, "User interrupt");
  }
  g_object_unref (loader);
}

NPError
NP_Initialize (NPNetscapeFuncs * moz_funcs, NPPluginFuncs * plugin_funcs)
{
  PRBool b = PR_FALSE;
  NPNToolkitType toolkit = 0;

  if (moz_funcs == NULL || plugin_funcs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if ((moz_funcs->version >> 8) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  if (moz_funcs->size < sizeof (NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;
  if (plugin_funcs->size < sizeof (NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  mozilla_funcs = *moz_funcs;

  /* we must be GTK 2 */
  if (CallNPN_GetValueProc(mozilla_funcs.getvalue, NULL,
	NPNVToolkit, (void *) &toolkit) || toolkit != NPNVGtk2)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  /* we want XEmbed embedding */
  if (CallNPN_GetValueProc(mozilla_funcs.getvalue, NULL,
	NPNVSupportsXEmbedBool, (void *) &b) || !b)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  memset (plugin_funcs, 0, sizeof (NPPluginFuncs));
  plugin_funcs->size = sizeof (NPPluginFuncs);
  plugin_funcs->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
  plugin_funcs->newp = NewNPP_NewProc (plugin_new);
  plugin_funcs->destroy = NewNPP_DestroyProc (plugin_destroy);

  plugin_funcs->newstream = NewNPP_NewStreamProc (plugin_new_stream);
  plugin_funcs->destroystream =
      NewNPP_DestroyStreamProc (plugin_destroy_stream_cb);
  plugin_funcs->writeready = NewNPP_WriteReadyProc (plugin_write_ready);
  plugin_funcs->write = NewNPP_WriteProc (plugin_write);
  plugin_funcs->asfile = NewNPP_StreamAsFileProc (plugin_stream_as_file);
  plugin_funcs->setwindow = NewNPP_SetWindowProc (plugin_set_window);
  plugin_funcs->event = NewNPP_HandleEventProc (plugin_handle_event);
  plugin_funcs->urlnotify = NewNPP_URLNotifyProc (plugin_url_notify);
#if 0
  plugin_funcs->print = NULL;
  plugin_funcs->javaClass = NULL;
  plugin_funcs->getvalue = NULL;
  plugin_funcs->setvalue = NewNPP_SetValueProc (plugin_set_value);
#endif

  return NPERR_NO_ERROR;
}

NPError
NP_Shutdown (void)
{
  /* Haha, we stay in memory anyway, no way to get rid of us! */
  return NPERR_NO_ERROR;
}

