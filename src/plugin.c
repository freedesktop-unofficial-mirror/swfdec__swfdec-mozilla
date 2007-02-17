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

#include <mozilla-config.h>
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
plugin_invalidate_rect (NPP instance, NPRect *rect)
{
  CallNPN_InvalidateRectProc (mozilla_funcs.invalidaterect, instance, rect);
}

/*** plugin implementation ***/

char *
NP_GetMIMEDescription (void)
{
  return "application/x-shockwave-flash:.swf:Adobe Flash movie";
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
      *val = "Shockwave Flash 9.0 (<A HREF=\"http://swfdec.freedesktop.org\">Swfdec</A> " VERSION ")";
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

static NPError
plugin_new (NPMIMEType mime_type, NPP instance,
    uint16_t mode, int16_t argc, char *argn[], char *argv[],
    NPSavedData * saved)
{
  int i;

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  /* if no main loop is running, we're not able to use it, and we need a main loop */
  if (g_main_depth () == 0)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  if (CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
	NPPVpluginKeepLibraryInMemory, (void *) PR_TRUE))
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
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
  for (i = 0; i < argc; i++) {
    if (g_ascii_strcasecmp (argn[i], "flashvars")) {
      swfmoz_player_set_variables (instance->pdata, argv[i]);
    } else {
      g_printerr ("Unsupported movie property %s", argn[i]);
    }
  }

  return NPERR_NO_ERROR;
}

NPError 
plugin_destroy (NPP instance, NPSavedData **save)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  g_object_unref (instance->pdata);
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
plugin_destroy_stream (NPP instance, NPStream* stream, NPReason reason)
{
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;
  if (!SWFMOZ_IS_LOADER (stream->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  SWFMOZ_LOADER (stream->pdata)->stream = NULL;
  swfdec_loader_eof (stream->pdata);
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
  NPSetWindowCallbackStruct *info = window->ws_info;
  
  if (instance == NULL || !SWFMOZ_IS_PLAYER (instance->pdata))
    return NPERR_INVALID_INSTANCE_ERROR;

  if (window) {
    plugin_x11_setup_windowed (instance->pdata, DisplayString (info->display),
	(Window) window->window, window->width, window->height);
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
  /* FIXME: need a way to tell about errors here */
  g_object_unref (notifyData);
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
      NewNPP_DestroyStreamProc (plugin_destroy_stream);
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
  g_printerr ("You should not see this text until you've closed your browser\n");
  return NPERR_NO_ERROR;
}

