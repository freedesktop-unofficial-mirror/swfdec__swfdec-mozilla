/* Swfdec
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
  //SwfmozLoader *moz = SWFMOZ_LOADER (loader);
  //SwfdecLoader *new;

  /* FIXME! */
  g_assert_not_reached ();
  return NULL;
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

SwfdecLoader *
swfmoz_loader_new (NPStream *stream)
{
  SwfmozLoader *ret;

  g_return_val_if_fail (stream != NULL, NULL);

  ret = g_object_new (SWFMOZ_TYPE_LOADER, NULL);
  ret->stream = stream;

  return SWFDEC_LOADER (ret);
}
