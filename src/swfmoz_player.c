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

#include "swfmoz_player.h"
#include "swfmoz_loader.h"

G_DEFINE_TYPE (SwfmozPlayer, swfmoz_player, G_TYPE_OBJECT)

static void
swfmoz_player_dispose (GObject *object)
{
  SwfmozPlayer *player = SWFMOZ_PLAYER (object);

  g_object_unref (player->player);
  if (player->target) {
    cairo_destroy (player->target);
    player->target = NULL;
  }
  if (player->thread) {
    g_main_loop_quit (player->loop);
    if (g_thread_join (player->thread) != player)
      g_assert_not_reached ();
    player->thread = NULL;
  }
  g_main_loop_unref (player->loop);
  g_main_context_unref (player->context);
  g_mutex_free (player->mutex);

  G_OBJECT_CLASS (swfmoz_player_parent_class)->dispose (object);
}

static void
swfmoz_player_class_init (SwfmozPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfmoz_player_dispose;
}

static gpointer
swfmoz_player_run (gpointer playerp)
{
  SwfmozPlayer *player = playerp;

  g_main_loop_run (player->loop);

  return player;
}

static void
swfmoz_player_init (SwfmozPlayer *player)
{
  player->mutex = g_mutex_new ();
  player->player = swfdec_player_new ();
  player->context = g_main_context_new ();
  player->loop = g_main_loop_new (player->context, FALSE);
  player->thread = g_thread_create (swfmoz_player_run, player, TRUE, NULL);
  if (!player->thread) {
    g_critical ("Could not create plugin thread");
  }
}

SwfmozPlayer *
swfmoz_player_new (NPP instance)
{
  SwfmozPlayer *ret;

  ret = g_object_new (SWFMOZ_TYPE_PLAYER, NULL);
  ret->instance = instance;

  return SWFMOZ_PLAYER (ret);
}

SwfdecLoader *
swfmoz_player_add_stream (SwfmozPlayer *player, NPStream *stream)
{
  SwfdecLoader *loader;
  
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), NULL);

  if (player->player_initialized)
    return NULL;
  loader = swfmoz_loader_new (stream);
  g_object_ref (loader);
  swfdec_player_set_loader (player->player, loader);
  player->player_initialized = TRUE;
  return loader;
}

void
swfmoz_player_set_target (SwfmozPlayer *player, cairo_t *cr)
{
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (cr != NULL);

  if (player->target)
    cairo_destroy (player->target);
  player->target = cr;
  if (cr)
    cairo_reference (cr);
}

void
swfmoz_player_render (SwfmozPlayer *player, int x, int y, int width, int height)
{
  cairo_matrix_t matrix;

  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  if (player->target == NULL)
    return;
  /* FIXME: do this without a need to use the matrix */
  cairo_get_matrix (player->target, &matrix);
  g_print ("rendering %d %d  %d %d - offset by %g %g\n", x, y, width, height, matrix.x0, matrix.y0);
  swfdec_player_render (player->player, player->target, 
      x - matrix.x0, y - matrix.y0, width, height);
}

void
swfmoz_player_lock (SwfmozPlayer *player)
{
  g_mutex_lock (player->mutex);
}

void
swfmoz_player_unlock (SwfmozPlayer *player)
{
  g_mutex_unlock (player->mutex);
}
