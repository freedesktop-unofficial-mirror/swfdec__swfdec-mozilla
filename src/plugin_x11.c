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

/*** Plugin code ***/

static GdkFilterReturn
plugin_x11_handle_event (GdkXEvent *gdkxevent, GdkEvent *unused, gpointer playerp)
{
  SwfmozPlayer *player = playerp;
  XEvent *event = gdkxevent;

  switch (event->type) {
    case VisibilityNotify:
      swfmoz_player_render (player, 0, 0, player->target_rect.width, player->target_rect.height);
      break;
    case Expose:
      {
	XExposeEvent *expose = (XExposeEvent *) event;
	swfmoz_player_render (player, expose->x, expose->y, 
	    expose->width, expose->height);
	break;
      }
    case ButtonPress:
    case ButtonRelease:
      {
	XButtonEvent *button = (XButtonEvent *) event;
	swfmoz_player_mouse_changed (player, button->button, button->x, 
	    button->y, event->type == ButtonPress);
	break;
      }
    case EnterNotify:
    case LeaveNotify:
      /* FIXME: implement */
      break;
    case MotionNotify:
      {
	int winx, winy;

	gdk_window_get_pointer (player->target, &winx, &winy, NULL);
	swfmoz_player_mouse_moved (player, winx, winy);
	break;
      }
    default:
      g_printerr ("unhandled event %d\n", event->type);
      break;
  }
  return GDK_FILTER_REMOVE;
}

static void
plugin_x11_notify_cb (SwfdecPlayer *player, GParamSpec *pspec, GdkWindow *window)
{
  GdkColor color;
  guint c;

  c = swfdec_player_get_background_color (player);
  color.red = ((c & 0xFF0000) >> 16) * 0x101;
  color.green = ((c & 0xFF00) >> 8) * 0x101;
  color.blue = (c & 0xFF) * 0x101;
  gdk_window_set_background (window, &color);
}

void
plugin_x11_setup_windowed (SwfmozPlayer *player, Window xwindow, 
    int x, int y, int width, int height)
{
  if (player->target == NULL) {
    GdkWindowAttr attr;
    GdkWindow *parent, *window;
    
    parent = gdk_window_foreign_new (xwindow);
    if (parent == NULL) {
      g_printerr ("invalid window given for setup (id %lu)\n", xwindow);
      return;
    }
    
    attr.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | 
      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK |
      GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | 
      GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
    attr.x = 0;
    attr.y = 0;
    attr.width = width;
    attr.height = height;
    attr.window_type = GDK_WINDOW_CHILD;
    attr.wclass = GDK_INPUT_OUTPUT;
    window = gdk_window_new (parent, &attr, GDK_WA_X | GDK_WA_Y);
    gdk_window_add_filter (window, plugin_x11_handle_event, player);
    gdk_window_show (window);
    swfmoz_player_set_target (player, window, 0, 0, width, height);
    plugin_x11_notify_cb (player->player, NULL, window);
    g_signal_connect (player->player, "notify::background-color", 
	G_CALLBACK (plugin_x11_notify_cb), window);
  } else {
    gdk_window_move_resize (player->target, 0, 0, width, height);
  }
}

void
plugin_x11_teardown (SwfmozPlayer *player)
{
  if (player->target) {
    gdk_window_remove_filter (player->target, plugin_x11_handle_event, player);
    g_signal_handlers_disconnect_by_func (player->player, 
	plugin_x11_notify_cb, player->target);
  }
  swfmoz_player_set_target (player, NULL, 0, 0, 0, 0);
}
