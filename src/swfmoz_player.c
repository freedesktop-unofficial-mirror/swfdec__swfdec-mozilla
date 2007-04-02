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
#include "plugin.h"
#include "swfmoz_player.h"
#include "swfmoz_loader.h"

#include "swfmoz_dialog.h"

/*** menu ***/

static void
swfmoz_player_menu_playing_toggled (GtkCheckMenuItem *item, SwfdecGtkPlayer* player)
{
  if (swfdec_gtk_player_get_playing (player) != gtk_check_menu_item_get_active (item))
    swfdec_gtk_player_set_playing (player, gtk_check_menu_item_get_active (item));
}

static void
swfmoz_player_menu_notify_playing (SwfdecGtkPlayer *player, GParamSpec *pspec,
    GtkCheckMenuItem *item)
{
  if (swfdec_gtk_player_get_playing (player) != gtk_check_menu_item_get_active (item))
    gtk_check_menu_item_set_active (item, swfdec_gtk_player_get_playing (player));
}

static void
swfmoz_player_menu_audio_toggled (GtkCheckMenuItem *item, SwfdecGtkPlayer* player)
{
  if (swfdec_gtk_player_get_audio_enabled (player) != gtk_check_menu_item_get_active (item))
    swfdec_gtk_player_set_audio_enabled (player, gtk_check_menu_item_get_active (item));
}

static void
swfmoz_player_menu_notify_audio (SwfdecGtkPlayer *player, GParamSpec *pspec,
    GtkCheckMenuItem *item)
{
  if (swfdec_gtk_player_get_audio_enabled (player) != gtk_check_menu_item_get_active (item))
    gtk_check_menu_item_set_active (item, swfdec_gtk_player_get_audio_enabled (player));
}

static void
swfmoz_player_menu_about (GtkMenuItem *item, SwfmozPlayer *player)
{
  static const char *authors[] = {
    "Benjamin Otte <otte@gnome.org>",
    "David Schleef <ds@schleef.org>",
    "Eric Anholt <eric@anholt.net>",
    NULL,
  };
  gtk_show_about_dialog (NULL, "authors", authors,
      "comments", "Play Flash content in your browser",
      "name", "Swfdec Mozilla Plugin",
      "version", VERSION,
      "website", "http://swfdec.freedesktop.org/",
      NULL);
}

static void
swfmoz_player_popup_menu (SwfmozPlayer *player)
{
  if (player->menu == NULL) {
    GtkWidget *item;

    player->menu = GTK_MENU (gtk_menu_new ());
    g_object_ref_sink (player->menu);
    
    item = gtk_check_menu_item_new_with_mnemonic ("Playing");
    g_signal_connect (item, "toggled", 
	G_CALLBACK (swfmoz_player_menu_playing_toggled), player->player);
    g_signal_connect (player->player, "notify::playing",
	G_CALLBACK (swfmoz_player_menu_notify_playing), item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), 
	swfdec_gtk_player_get_playing (SWFDEC_GTK_PLAYER (player->player)));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (player->menu), item);

    item = gtk_check_menu_item_new_with_mnemonic ("Enable Audio");
    g_signal_connect (item, "toggled", 
	G_CALLBACK (swfmoz_player_menu_audio_toggled), player->player);
    g_signal_connect (player->player, "notify::audio-enabled",
	G_CALLBACK (swfmoz_player_menu_notify_audio), item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), 
	swfdec_gtk_player_get_audio_enabled (SWFDEC_GTK_PLAYER (player->player)));
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (player->menu), item);

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (player->menu), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PROPERTIES, NULL);
    g_signal_connect_swapped (item, "activate", 
	G_CALLBACK (swfmoz_dialog_show), player);
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (player->menu), item);

    item = gtk_separator_menu_item_new ();
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (player->menu), item);

    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
    g_signal_connect (item, "activate", 
	G_CALLBACK (swfmoz_player_menu_about), player);
    gtk_widget_show (item);
    gtk_menu_shell_append (GTK_MENU_SHELL (player->menu), item);
  }
  gtk_menu_popup (player->menu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time());
}

/*** SWFMOZ_PLAYER ***/

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

  xi += player->target_rect.x;
  yi += player->target_rect.y;
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
    g_source_set_priority (source, GDK_PRIORITY_REDRAW);
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
swfmoz_player_invalidate (SwfmozPlayer *player)
{
  swfmoz_player_redraw (player->player, 0.0, 0.0, player->target_rect.width, 
      player->target_rect.height, player);
}

static void
swfmoz_player_notify_playing (SwfdecGtkPlayer *gtkplayer, GParamSpec *pspec, SwfmozPlayer *player)
{
  swfmoz_player_invalidate (player);
}

static void
swfmoz_player_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  //SwfmozPlayer *player = SWFMOZ_PLAYER (object);
  
  switch (param_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfmoz_player_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  //SwfmozPlayer *player = SWFMOZ_PLAYER (object);

  switch (param_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfmoz_player_dispose (GObject *object)
{
  SwfmozPlayer *player = SWFMOZ_PLAYER (object);

  if (player->menu) {
    g_signal_handlers_disconnect_matched (player->player, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, 
	swfmoz_player_menu_notify_playing, NULL);
    g_signal_handlers_disconnect_matched (player->player, G_SIGNAL_MATCH_FUNC, 0, 0, NULL, 
	swfmoz_player_menu_notify_audio, NULL);
    gtk_widget_destroy (GTK_WIDGET (player->menu));
    player->menu = NULL;
  }
  /* the player might be refed elsewhere */
  g_signal_handlers_disconnect_by_func (player->player, swfmoz_player_redraw, player);
  g_signal_handlers_disconnect_by_func (player->player, swfmoz_player_launch, player);
  g_signal_handlers_disconnect_by_func (player->player, swfmoz_player_notify_playing, player);
  g_object_unref (player->player);
  if (player->target) {
    g_object_unref (player->target);
    player->target = NULL;
  }
  if (player->repaint_source) {
    g_source_destroy (player->repaint_source);
    g_source_unref (player->repaint_source);
    player->repaint_source = NULL;
  }
  if (player->initial) {
    g_object_unref (player->initial);
    player->initial = NULL;
  }
  if (player->loaders) {
    g_object_unref (player->loaders);
    player->loaders = NULL;
  }
  g_free (player->variables);

  G_OBJECT_CLASS (swfmoz_player_parent_class)->dispose (object);
}

static void
swfmoz_player_class_init (SwfmozPlayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfmoz_player_dispose;
  object_class->get_property = swfmoz_player_get_property;
  object_class->set_property = swfmoz_player_set_property;
}

static void
swfmoz_player_init (SwfmozPlayer *player)
{
  player->player = swfdec_gtk_player_new ();
  g_signal_connect (player->player, "invalidate", G_CALLBACK (swfmoz_player_redraw), player);
  g_signal_connect (player->player, "launch", G_CALLBACK (swfmoz_player_launch), player);
  g_signal_connect (player->player, "notify::playing", G_CALLBACK (swfmoz_player_notify_playing), player);
  player->context = g_main_context_default ();

  player->loaders = GTK_TREE_MODEL (gtk_list_store_new (SWFMOZ_LOADER_N_COLUMNS,
      SWFMOZ_TYPE_LOADER, G_TYPE_STRING, G_TYPE_STRING, 
      G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_UINT));
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

static void
swfmoz_player_loaders_update (GtkListStore *store, GtkTreeIter *iter, SwfdecLoader *loader)
{
  char *filename = swfdec_loader_get_filename (loader);
  guint percent;

  percent = swfdec_loader_get_size (loader);
  if (percent) {
    percent = 100 * swfdec_loader_get_loaded (loader) / percent;
  } else {
    percent = loader->eof ? 100 : 50;
  }
  gtk_list_store_set (store, iter,
    SWFMOZ_LOADER_COLUMN_LOADER, loader,
    SWFMOZ_LOADER_COLUMN_NAME, filename,
    SWFMOZ_LOADER_COLUMN_URL, loader->url,
    SWFMOZ_LOADER_COLUMN_EOF, loader->eof,
    SWFMOZ_LOADER_COLUMN_ERROR, loader->error != NULL,
    SWFMOZ_LOADER_COLUMN_TYPE, swfmoz_loader_get_data_type_string (loader),
    SWFMOZ_LOADER_COLUMN_PERCENT_LOADED, percent,
    -1);
  g_free (filename);
}

static gboolean
swfmoz_player_find_loader (GtkListStore *store, SwfdecLoader *loader, GtkTreeIter *iter)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);

  if (!gtk_tree_model_get_iter_first (model, iter))
    return FALSE;
  do {
    SwfdecLoader *comp;
    gtk_tree_model_get (model, iter, SWFMOZ_LOADER_COLUMN_LOADER, &comp, -1);
    g_object_unref (comp);
    if (comp == loader)
      return TRUE;
  } while (gtk_tree_model_iter_next (model, iter));
  return FALSE;
}

static void
swfmoz_player_loader_notify_cb (SwfdecLoader *loader, GParamSpec *pspec, GtkListStore *store)
{
  GtkTreeIter iter;

  if (!swfmoz_player_find_loader (store, loader, &iter)) {
    g_assert_not_reached ();
  }
  swfmoz_player_loaders_update (store, &iter, loader);
}

SwfdecLoader *
swfmoz_player_add_stream (SwfmozPlayer *player, NPStream *stream)
{
  GtkTreeIter iter;
  SwfdecLoader *loader;
  
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), NULL);

  if (stream->notifyData) {
    loader = SWFDEC_LOADER (stream->notifyData);
    swfmoz_loader_set_stream (SWFMOZ_LOADER (loader), player->instance, stream);
  } else {
    if (player->initial)
      return NULL;
    loader = swfmoz_loader_new (player->instance, stream);
    swfdec_player_set_loader_with_variables (player->player, loader, player->variables);
    player->initial = loader;
    g_object_ref (loader);
  }
  /* add loader to the list of loaders */
  g_signal_connect (loader, "notify", G_CALLBACK (swfmoz_player_loader_notify_cb),
      GTK_LIST_STORE (player->loaders));
  gtk_list_store_append (GTK_LIST_STORE (player->loaders), &iter);
  swfmoz_player_loaders_update (GTK_LIST_STORE (player->loaders), &iter, loader);
  return loader;
}

void
swfmoz_player_set_target (SwfmozPlayer *player, GdkWindow *target, 
    int x, int y, int width, int height)
{
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (GDK_IS_WINDOW (target));

  if (player->target) {
    g_object_unref (player->target);
  }
  player->target = target;
  player->target_rect.x = x;
  player->target_rect.y = y;
  player->target_rect.width = width;
  player->target_rect.height = height;
  if (target) {
    g_object_ref (target);
  }
}

void
swfmoz_player_render (SwfmozPlayer *player, int x, int y, int width, int height)
{
  int player_width, player_height;
  GdkRectangle rect;
  cairo_t *cr;

  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (x >= 0);
  g_return_if_fail (y >= 0);
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  /* first, remove the idle source */
  if (player->repaint_source && x <= player->x && y <= player->y &&
      x + width >= player->x + player->width &&
      y + height >= player->y + player->height) {
    g_source_destroy (player->repaint_source);
    g_source_unref (player->repaint_source);
    player->repaint_source = NULL;
  }

  /* second, check if we have anything to draw */
  if (player->target == NULL)
    return;
  rect.x = x + player->target_rect.x;
  rect.y = y + player->target_rect.y;
  rect.width = width;
  rect.height = height;
  if (!gdk_rectangle_intersect (&rect, &player->target_rect, &rect))
    return;

  swfdec_player_get_image_size (player->player, &player_width, &player_height);
  rect.width = MIN (width, player_width);
  rect.height = MIN (height, player_height);

  gdk_window_begin_paint_rect (player->target, &rect);
  cr = gdk_cairo_create (player->target);
  cairo_translate (cr, player->target_rect.x, player->target_rect.y);
  if (rect.width > 0 && rect.height > 0) {
    swfdec_player_render (player->player, cr, x, y, rect.width, rect.height);
  }
  if (!swfdec_gtk_player_get_playing (SWFDEC_GTK_PLAYER (player->player))) {
    int w = player->target_rect.width;
    int h = player->target_rect.height;
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
  cairo_destroy (cr);
  gdk_window_end_paint (player->target);
}

gboolean
swfmoz_player_mouse_changed (SwfmozPlayer *player, int button, int x, int y, gboolean down)
{
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), FALSE);

  switch (button) {
    case 1:
      if (!swfdec_gtk_player_get_playing (SWFDEC_GTK_PLAYER (player->player))) {
	if (!down)
	  return FALSE;
	swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (player->player), TRUE);
      } else {
	player->mouse_down = down;
	swfdec_player_handle_mouse (player->player, x, y, down ? 1 : 0);
      }
      return TRUE;
    case 3:
      if (!down) {
	swfmoz_player_popup_menu (player);
	return TRUE;
      }
    default:
      break;
  }
  return FALSE;
}

gboolean
swfmoz_player_mouse_moved (SwfmozPlayer *player, int x, int y)
{
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), FALSE);

  if (swfdec_gtk_player_get_playing (SWFDEC_GTK_PLAYER (player->player))) {
    swfdec_player_handle_mouse (player->player, x, y, player->mouse_down ? 1 : 0);
  }
  return FALSE;
}

char *
swfmoz_player_get_filename (SwfmozPlayer *player)
{
  g_return_val_if_fail (SWFMOZ_IS_PLAYER (player), NULL);

  if (player->initial == NULL)
    return g_strdup ("unknown.swf");

  return swfdec_loader_get_filename (player->initial);
}

void
swfmoz_player_set_variables (SwfmozPlayer *player, const char *variables)
{
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));
  g_return_if_fail (player->initial == NULL);
  g_return_if_fail (variables != NULL);

  g_free (player->variables);
  player->variables = g_strdup (variables);
}

void
swfmoz_player_remove (SwfmozPlayer *player)
{
  /* This function is called when the player is removed from screen.
   * It may still be necessary to keep it around if dialogs are open.
   */
  g_return_if_fail (SWFMOZ_IS_PLAYER (player));

  swfdec_gtk_player_set_playing (SWFDEC_GTK_PLAYER (player->player), FALSE);
  swfdec_gtk_player_set_audio_enabled (SWFDEC_GTK_PLAYER (player->player), FALSE);
  swfmoz_dialog_remove (player);
  g_object_unref (player);
}

