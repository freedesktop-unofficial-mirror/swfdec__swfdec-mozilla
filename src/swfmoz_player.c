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

#include "swfmoz_player.h"
#include <libswfdec/swfdec_player.h>

G_DEFINE_TYPE (SwfmozPlayer, swfmoz_player, G_TYPE_OBJECT)

static void
swfmoz_player_dispose (GObject *object)
{
  SwfmozPlayer *player = SWFMOZ_PLAYER (object);

  g_object_unref (player->player);

  G_OBJECT_CLASS (swfmoz_player_parent_class)->dispose (object);
}

static void
swfmoz_player_class_init (SwfmozPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfmoz_player_dispose;
}

static void
swfmoz_player_init (SwfmozPlayer *player)
{
  player->player = swfdec_player_new ();
}

SwfmozPlayer *
swfmoz_player_new (NPP instance)
{
  SwfmozPlayer *ret;

  ret = g_object_new (SWFMOZ_TYPE_PLAYER, NULL);
  ret->instance = instance;

  return SWFMOZ_PLAYER (ret);
}
