/* Swfdec Mozilla Plugin
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfmoz_loader.h"
#include "plugin.h"

/* Note about refcounting:
 * Refcounts to a SwfdecLoader are held by:
 * - The SwfdecPlayer (until he releases it which can be at any point in time)
 * - The NPStream (released in NPP_DestroyStream)
 * - if the stream was created using NPN_Get/PostURLNotify, the notifyData 
 *   holds a reference. This reference is released by NPP_URLNotify
 * It's necessary to keep this many references since there's a lot of possible
 * orders in which these events happen.
 */

G_DEFINE_TYPE (SwfmozLoader, swfmoz_loader, SWFDEC_TYPE_LOADER)

static void
swfmoz_loader_dispose (GObject *object)
{
  //SwfmozLoader *loader = SWFMOZ_LOADER (object);

  G_OBJECT_CLASS (swfmoz_loader_parent_class)->dispose (object);
}

static SwfdecLoader *
swfmoz_loader_load (SwfdecLoader *loader, const char *url)
{
  SwfmozLoader *moz = SWFMOZ_LOADER (loader);
  SwfdecLoader *new;

  /* FIXME: add guards so not every URL is loaded */
  new = g_object_new (SWFMOZ_TYPE_LOADER, NULL);
  g_object_ref (new);
  plugin_get_url_notify (moz->instance, url, NULL, new);
  return new;
}

static void
swfmoz_loader_class_init (SwfmozLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecLoaderClass *loader_class = SWFDEC_LOADER_CLASS (klass);

  object_class->dispose = swfmoz_loader_dispose;

  loader_class->load = swfmoz_loader_load;
}

static void
swfmoz_loader_init (SwfmozLoader *slow_loader)
{
}

void
swfmoz_loader_set_stream (SwfmozLoader *loader, NPP instance, NPStream *stream)
{
  g_return_if_fail (SWFMOZ_IS_LOADER (loader));
  g_return_if_fail (loader->stream == NULL);
  g_return_if_fail (instance != NULL);
  g_return_if_fail (stream != NULL);

  g_printerr ("Loading stream: %s\n", stream->url);
  SWFDEC_LOADER (loader)->url = g_strdup (stream->url);
  loader->instance = instance;
  loader->stream = stream;
}

SwfdecLoader *
swfmoz_loader_new (NPP instance, NPStream *stream)
{
  SwfmozLoader *ret;

  g_return_val_if_fail (stream != NULL, NULL);

  ret = g_object_new (SWFMOZ_TYPE_LOADER, NULL);
  swfmoz_loader_set_stream (ret, instance, stream);

  return SWFDEC_LOADER (ret);
}
