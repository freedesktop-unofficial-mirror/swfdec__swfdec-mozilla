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

#include <math.h>
#include "swfmoz_player.h"
#include "plugin.h"
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
  if (player->repaint_source) {
    g_source_destroy (player->repaint_source);
    g_source_unref (player->repaint_source);
    player->repaint_source = NULL;
  }

  G_OBJECT_CLASS (swfmoz_player_parent_class)->dispose (object);
}

static void
swfmoz_player_class_init (SwfmozPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfmoz_player_dispose;
}

static gboolean
swfmoz_player_idle_redraw (gpointer playerp)
{
  SwfmozPlayer *player = playerp;

  swfmoz_player_render (player, player->x, player->y, player->width, player->height);
  return TRUE;
}

static void
swfmoz_player_redraw (SwfdecPlayer *swfplayer, double x, double y, double width, double height, SwfmozPlayer *player)
{
  g_print ("invalidating %g %g  %g %g\n", x, y, width, height);
  if (player->windowless) {
    NPRect rect;
    rect.left = floor (x);
    rect.top = floor (y);
    rect.right = ceil (x + width);
    rect.bottom = ceil (y + height);
    plugin_invalidate_rect (player->instance, &rect);
  } else if (player->repaint_source) {
    player->x = MIN (player->x, floor (x));
    player->y = MIN (player->y, floor (y));
    player->width = MAX (player->width, ceil (x + width) - player->x);
    player->height = MAX (player->height, ceil (y + height) - player->y);
  } else {
    GSource *source = g_idle_source_new ();
    player->repaint_source = source;
    /* match GTK */
    g_source_set_priority (source, G_PRIORITY_HIGH_IDLE + 20);
    g_source_set_callback (source, swfmoz_player_idle_redraw, player, NULL);
    g_source_attach (source, player->context);
    player->x = floor (x);
    player->y = floor (y);
    player->width = ceil (x + width) - player->x;
    player->height = ceil (y + height) - player->y;
  }
}

static void
swfmoz_player_init (SwfmozPlayer *player)
{
  player->player = swfdec_player_new ();
  g_signal_connect (player->player, "invalidate", G_CALLBACK (swfmoz_player_redraw), player);
  player->context = NULL;
}

SwfmozPlayer *
swfmoz_player_new (NPP instance, gboolean windowless)
{
  SwfmozPlayer *ret;

  ret = g_object_new (SWFMOZ_TYPE_PLAYER, NULL);
  ret->instance = instance;
  ret->windowless = windowless;

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
  swfdec_player_render (player->player, player->target, 
      x - matrix.x0, y - matrix.y0, width, height);
  if (player->repaint_source) {
    g_source_destroy (player->repaint_source);
    g_source_unref (player->repaint_source);
    player->repaint_source = NULL;
  }
}

