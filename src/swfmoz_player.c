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
#include "swfdec_playback.h"
#include "swfdec_source.h"
#include "swfmoz_loader.h"

G_DEFINE_TYPE (SwfmozPlayer, swfmoz_player, G_TYPE_OBJECT)

static gboolean
swfmoz_player_idle_redraw (gpointer playerp)
{
  SwfmozPlayer *player = playerp;

  swfmoz_player_render (player, player->x, player->y, player->width, player->height);
  return TRUE;
}

static void
swfmoz_player_redraw (SwfdecPlayer *swfplayer, double x, double y, 
    double width, double height, SwfmozPlayer *player)
{
  int xi, yi, wi, hi;

  xi = MAX ((int) floor (x), 0);
  yi = MAX ((int) floor (y), 0);
  wi = (int) ceil (x + width) - xi;
  hi = (int) ceil (y + height) - yi;
  if (wi <= 0 || hi <= 0)
    return;

  if (player->windowless) {
    NPRect rect;
    rect.left = xi;
    rect.top = yi;
    rect.right = xi + wi;
    rect.bottom = yi + hi;
    plugin_invalidate_rect (player->instance, &rect);
  } else if (player->repaint_source) {
    player->width = MAX (player->x + player->width, xi + wi);
    player->height = MAX (player->y + player->height, yi + hi);
    player->x = MIN (player->x, xi);
    player->y = MIN (player->y, yi);
    player->width -= player->x;
    player->height -= player->y;
  } else {
    GSource *source = g_idle_source_new ();
    player->repaint_source = source;
    g_source_set_priority (source, G_PRIORITY_HIGH_IDLE + 20); /* match GTK */
    g_source_set_callback (source, swfmoz_player_idle_redraw, player, NULL);
    g_source_attach (source, player->context);
    player->x = xi;
    player->y = yi;
    player->width = wi;
    player->height = hi;
  }
}

static void
swfmoz_player_launch (SwfdecPlayer *swfplayer, const char *url, const char *target,
    SwfmozPlayer *player)
{
  plugin_get_url (player->instance, url, target);
}

static void
swfmoz_player_dispose (GObject *object)
{
  SwfmozPlayer *player = SWFMOZ_PLAYER (object);

  swfmoz_player_set_paused (player, TRUE);

  /* the player might be refed elsewhere */
  g_signal_handlers_disconnect_by_func (player->player, swfmoz_player_redraw, player);
  g_signal_handlers_disconnect_by_func (player->player, swfmoz_player_launch, player);
  g_object_unref (player->player);
  if (player->target) {
    cairo_destroy (player->target);
    cairo_destroy (player->intermediate);
    player->target = NULL;
  }
  if (player->repaint_source) {
    g_source_destroy (player->repaint_source);
    g_source_unref (player->repaint_source);
    player->repaint_source = NULL;
  }

  /* sanity checks */
  g_assert (player->audio == NULL);

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
  g_signal_connect (player->player, "invalidate", G_CALLBACK (swfmoz_player_redraw), player);
  g_signal_connect (player->player, "launch", G_CALLBACK (swfmoz_player_launch), player);
  player->context = g_main_context_default ();
  player->paused = TRUE;
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

  if (stream->notifyData) {
    loader = SWFDEC_LOADER (stream->notifyData);
    swfmoz_loader_set_stream (SWFMOZ_LOADER (loader), player->instance, stream);
  } else {
    if (player->player_initialized)
      return NULL;
    loader = swfmoz_loader_new (player->instance, stream);
    swfdec_player_set_loader (player->player, loader);
    player->player_initialized = TRUE;
  }
  return loader;
}

void
swfmoz_player_set_target (SwfmozPlayer *player, cairo_t *cr, unsigned int width, unsigned int height)
{
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (cr != NULL);

  if (player->target) {
    cairo_destroy (player->target);
    cairo_destroy (player->intermediate);
  }
  player->target = cr;
  player->target_width = width;
  player->target_height = height;
  if (cr) {
    cairo_surface_t *surface;
    cairo_reference (cr);
    surface = cairo_surface_create_similar (cairo_get_target (cr), 
	CAIRO_CONTENT_COLOR_ALPHA, width, height);
    player->intermediate = cairo_create (surface);
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_surface_destroy (surface);
  }
}

void
swfmoz_player_render (SwfmozPlayer *player, int x, int y, int width, int height)
{
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  if (player->target == NULL)
    return;
  swfdec_player_render (player->player, player->intermediate, 
      x, y, width, height);
  if (player->paused) {
    cairo_t *cr = player->intermediate;
    int w = player->target_width;
    int h = player->target_height;
    int len = MIN (w, h) * 4 / 5;
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
    cairo_rectangle (cr, (w - len) / 2, (h - len) / 2,
	len / 3, len);
    cairo_rectangle (cr, (w - len) / 2 + 2 * len / 3, (h - len) / 2,
	len / 3, len);
    cairo_fill_preserve (cr);
    cairo_rectangle (cr, 0, 0, w, h);
    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.4);
    cairo_fill (cr);
  }

  cairo_rectangle (player->target, x, y, width, height);
  cairo_fill (player->target);
  if (player->repaint_source) {
    g_source_destroy (player->repaint_source);
    g_source_unref (player->repaint_source);
    player->repaint_source = NULL;
  }
}

gboolean
swfmoz_player_mouse_changed (SwfmozPlayer *player, int button, int x, int y, gboolean down)
{
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), FALSE);

  switch (button) {
    case 1:
      if (swfmoz_player_get_paused (player)) {
	if (!down)
	  return FALSE;
	swfmoz_player_set_paused (player, FALSE);
      } else {
	swfdec_player_handle_mouse (player->player, x, y, down ? 1 : 0);
      }
      return TRUE;
    case 3:
      if (down) {
	swfmoz_player_set_paused (player, !swfmoz_player_get_paused (player));
	return TRUE;
      }
    default:
      break;
  }
  return FALSE;
}

gboolean
swfmoz_player_get_paused (SwfmozPlayer *player)
{
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), TRUE);

  return player->paused;
}

static void
swfmoz_player_invalidate (SwfmozPlayer *player)
{
  swfmoz_player_redraw (player->player, 0.0, 0.0, player->target_width, 
      player->target_height, player);
}

static void
swfmoz_player_update_audio (SwfmozPlayer *player)
{
  gboolean should_play = !player->paused;

  if (should_play && player->audio == NULL) {
    player->audio = swfdec_playback_open (player->player, player->context);
  } else if (!should_play && player->audio != NULL) {
    swfdec_playback_close (player->audio);
    player->audio = NULL;
  }
}

void
swfmoz_player_set_paused (SwfmozPlayer *player, gboolean paused)
{
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));

  if (player->paused == paused)
    return;

  player->paused = paused;
  swfmoz_player_update_audio (player);
  if (paused) {
    g_source_destroy (player->iterate_source);
    g_source_unref (player->iterate_source);
    player->iterate_source = NULL;
  } else {
    GSource *source = swfdec_iterate_source_new (player->player, 1.0);
    g_source_attach (source, player->context);
    player->iterate_source = source;
  }
  swfmoz_player_invalidate (player);
}

