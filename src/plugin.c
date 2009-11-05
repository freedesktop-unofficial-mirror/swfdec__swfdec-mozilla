/* Swfdec Mozilla Plugin
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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
#include "plugin.h"
#include "plugin_x11.h"

/* This is here so we can quickly disable windowless support. For now it's 
 * missing some features (cursor support) and redraws seem to be buggy */
//#define DISABLE_WINDOWLESS

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

gboolean
plugin_get_value (NPP instance, NPNVariable var, gpointer data)
{
  return CallNPN_GetValueProc(mozilla_funcs.getvalue, instance, var, data) == NPERR_NO_ERROR;
}

/*** plugin implementation ***/

char *
NP_GetMIMEDescription (void)
{
  return (char *) "application/x-shockwave-flash:swf:Adobe Flash movie;application/futuresplash:spl:FutureSplash movie";
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
      *val = (char *) "Shockwave Flash";
      break;
    case NPPVpluginDescriptionString:
      /* FIXME: find a way to encode the Swfdec version without breaking stupid JS scripts */
      *val = (char *) "Shockwave Flash 9.0 r999";
      break;
    case NPPVpluginNeedsXEmbed:
      *((PRBool*) val) = PR_FALSE;
      break;
    case NPPVpluginWindowBool:
    case NPPVpluginTransparentBool:
    case NPPVjavaClass:
    case NPPVpluginWindowSize:
    case NPPVpluginTimerInterval:
    case NPPVpluginScriptableInstance:
    case NPPVpluginScriptableIID:
    case NPPVjavascriptPushCallerBool:
    case NPPVpluginKeepLibraryInMemory:
    case NPPVpluginScriptableNPObject:
    case NPPVformValue:
    default:
      return NPERR_INVALID_PARAM;
      break;
  }
  return NPERR_NO_ERROR;
}

G_MODULE_EXPORT gboolean swfdec_mozilla_make_sure_this_thing_stays_in_memory (void);
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

#ifndef DISABLE_WINDOWLESS
/* returns true if this instance can run windowless */
static gboolean
plugin_try_windowless (NPP instance)
{
  PRBool b = PR_FALSE;

  /* Check if the plugin can do windowless.
   * See http://bugzilla.mozilla.org/show_bug.cgi?id=386537 */
  if (CallNPN_GetValueProc(mozilla_funcs.getvalue, instance,
	NPNVSupportsWindowless, (void *) &b) || b != PR_TRUE)
    return FALSE;

  /* Try making us windowless */
  if (CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
  	NPPVpluginWindowBool, (void *) PR_FALSE))
    return FALSE;

  return TRUE;
}
#endif

static NPError
plugin_new (NPMIMEType mime_type, NPP instance,
    uint16_t mode, int16_t argc, char *argn[], char *argv[],
    NPSavedData * saved)
{
  SwfdecPlayer *player;
  int i;
  gboolean windowless = FALSE, opaque = FALSE;
  NPNToolkitType toolkit = 0;

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  /* we must be GTK 2 */
  if (CallNPN_GetValueProc(mozilla_funcs.getvalue, instance,
	NPNVToolkit, (void *) &toolkit) || toolkit != NPNVGtk2)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if (!swfdec_mozilla_make_sure_this_thing_stays_in_memory ()) {
    g_printerr ("Ensuring the plugin stays in memory did not work.\n"
	        "This happens when the plugin was copied from its installed location at " PLUGIN_FILE ".\n"
		"Please use the --with-plugin-dir configure option to install it into a different place.\n");
    return NPERR_INVALID_INSTANCE_ERROR;
  }

  /* Init functioncalling (even g_type_init) gets postponed until we know we
   * won't be unloaded, i.e. NPPVpluginKeepLibraryInMemory was successful */
  swfdec_init ();

#ifndef DISABLE_WINDOWLESS
  /* parse pre-creation properties */
  for (i = 0; i < argc; i++) {
    if (g_ascii_strcasecmp (argn[i], "wmode") == 0) {
      if (g_ascii_strcasecmp (argv[i], "transparent") == 0) {
	windowless = plugin_try_windowless (instance);
	opaque = FALSE;
      } else if (g_ascii_strcasecmp (argv[i], "opaque") == 0) {
	windowless = plugin_try_windowless (instance);
	if (windowless) {
	  CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
	    NPPVpluginTransparentBool, (void *) PR_FALSE);
	  opaque = TRUE;
	}
      }
    }
  }
#endif

  instance->pdata = player = swfmoz_player_new (instance, windowless, opaque);

  /* set the properties we support */
  /* FIXME: figure out how variables override each other */
  for (i = 0; i < argc; i++) {
    if (argn[i] == NULL)
      continue;
    if (g_ascii_strcasecmp (argn[i], "flashvars") == 0) {
      if (argv[i])
	swfdec_player_set_variables (player, argv[i]);
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
      swfdec_player_set_scale_mode (player, scale);
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
      guint j;

      for (j = 0; j < G_N_ELEMENTS (possibilities); j++) {
	if (g_ascii_strcasecmp (argv[i], possibilities[j].name) == 0) {
	  align = possibilities[j].align;
	  break;
	}
      }
      swfdec_player_set_alignment (player, align);
    }
  }

  return NPERR_NO_ERROR;
}

static NPError 
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
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  if (!SWFMOZ_IS_LOADER (stream->notifyData)) {
    if (!swfmoz_player_set_initial_stream (instance->pdata, stream))
      return NPERR_INVALID_URL;
  } else {
    swfmoz_loader_set_stream (stream->notifyData, stream);
  }
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
  swfdec_stream_close (stream->pdata);
  SWFMOZ_LOADER (stream->pdata)->stream = NULL;
  if (SWFMOZ_PLAYER (instance->pdata)->initial == stream->pdata)
    SWFMOZ_PLAYER (instance->pdata)->initial = NULL;
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

  new = swfdec_buffer_new_for_data (g_memdup (buffer, len), len);
  swfmoz_loader_ensure_open (stream->pdata);
  swfdec_stream_push (stream->pdata, new);
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
	window->x, window->y, window->width, window->height,
	((NPSetWindowCallbackStruct *)window->ws_info)->visual);
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

  plugin_x11_handle_event (instance->pdata, eventp);

  return TRUE;
}

static void
plugin_url_notify (NPP instance, const char* url, NPReason reason, void* notifyData)
{
  SwfdecStream *stream;

  // plugin_post_url_notify is used for launch so headers can be sent, but we
  // don't actually care about the notify
  if (notifyData == NULL)
    return;

  stream = SWFDEC_STREAM (notifyData);

  if (reason == NPRES_NETWORK_ERR) {
    swfdec_stream_error (stream, "Network error");
  } else if (reason == NPRES_USER_BREAK) {
    swfdec_stream_error (stream, "User interrupt");
  }
  g_object_unref (stream);
}

NPError
NP_Initialize (NPNetscapeFuncs * moz_funcs, NPPluginFuncs * plugin_funcs)
{
  if (moz_funcs == NULL || plugin_funcs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if ((moz_funcs->version >> 8) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  if (moz_funcs->size < sizeof (NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;
  if (plugin_funcs->size < sizeof (NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  mozilla_funcs = *moz_funcs;

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

