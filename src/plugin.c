/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#include <npapi.h>
#include <npupp.h>

#include "swfmoz_player.h"

NPNetscapeFuncs mozilla_funcs;


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
      *val = "Swfdec Flash Player " VERSION;
      break;
    case NPPVpluginDescriptionString:
      *val = "A plugin to play Flash files using the Swfdec library, see "
	"<A HREF=\"http://swfdec.freedesktop.org\">http://swfdec.freedesktop.org</A>";
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
  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  g_print ("make plugin stay in memory\n");
  if (CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
	NPPVpluginKeepLibraryInMemory, (void *) PR_TRUE))
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  g_print ("we wanna be windowed\n");
  if (CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
	NPPVpluginWindowBool, (void *) PR_TRUE))
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  g_print ("ok, done\n");
  /* this is not lethal, we ignore a failure here */
  CallNPN_SetValueProc (mozilla_funcs.setvalue, instance,
      NPPVpluginTransparentBool, (void *) PR_TRUE);

  /* Init functioncalling (even g_type_init) gets postponed until we know we
   * won't be unloaded, i.e. NPPVpluginKeepLibraryInMemory was successful */
  swfdec_init ();

  //instance->pdata = swfmoz_player_new (instance);
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

NPError
NP_Initialize (NPNetscapeFuncs * moz_funcs, NPPluginFuncs * plugin_funcs)
{
  PRBool b = PR_FALSE;
  NPNToolkitType toolkit = 0;

  g_print ("Hi!\n");
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
  g_print ("checking toolkit\n");
  if (CallNPN_GetValueProc(mozilla_funcs.getvalue, NULL,
	NPNVToolkit, (void *) &toolkit) || toolkit != NPNVGtk2)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  /* we want XEmbed embedding */
  g_print ("checking XEmbed\n");
  if (CallNPN_GetValueProc(mozilla_funcs.getvalue, NULL,
	NPNVSupportsXEmbedBool, (void *) &b) || !b)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;

  g_print ("initialization requirements met\n");
  plugin_funcs->size = sizeof (NPPluginFuncs);
  plugin_funcs->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
  plugin_funcs->newp = NewNPP_NewProc (plugin_new);
  plugin_funcs->destroy = NewNPP_DestroyProc (plugin_destroy);
#if 0
  plugin_funcs->setwindow = NewNPP_SetWindowProc (plugin_set_window);
  plugin_funcs->newstream = NewNPP_NewStreamProc (plugin_new_stream);
  plugin_funcs->destroystream =
      NewNPP_DestroyStreamProc (plugin_destroy_stream);
  plugin_funcs->asfile = NewNPP_StreamAsFileProc (plugin_stream_as_file);
  plugin_funcs->writeready = NewNPP_WriteReadyProc (plugin_write_ready);
  plugin_funcs->write = NewNPP_WriteProc (plugin_write);
  plugin_funcs->print = NULL;
  plugin_funcs->event = NewNPP_HandleEventProc (plugin_event);
  plugin_funcs->urlnotify = NULL;
  plugin_funcs->javaClass = NULL;
  plugin_funcs->getvalue = NULL;
  plugin_funcs->setvalue = NewNPP_SetValueProc (plugin_set_value);
#endif

  return NPERR_NO_ERROR;
}

NPError
NP_Shutdown (void)
{
  return NPERR_NO_ERROR;
}

