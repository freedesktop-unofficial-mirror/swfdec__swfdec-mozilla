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

#include "plugin.h"
#include "plugin_x11.h"
#include "swfmoz_player.h"
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <cairo-xlib.h>

/*** Plugin code ***/

void
plugin_x11_handle_event (SwfmozPlayer *mozplay, XEvent *event)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (mozplay);

  switch (event->type) {
    case VisibilityNotify:
      if (!mozplay->windowless) {
	GdkRectangle rect = { 0, 0, mozplay->target_rect.width, mozplay->target_rect.height };
	GdkRegion *region;
	region = gdk_region_rectangle (&rect);
	swfmoz_player_render (mozplay, NULL, region);
	gdk_region_destroy (region);
      }
      break;
    case GraphicsExpose:
      if (mozplay->windowless && mozplay->target) {
	XGraphicsExposeEvent *expose = (XGraphicsExposeEvent *) event;
	GdkRectangle rect = { expose->x, expose->y, expose->width, expose->height };
	GdkRegion *region = gdk_region_rectangle (&rect);
	cairo_surface_t *surface = cairo_xlib_surface_create (expose->display, 
	    expose->drawable, GDK_VISUAL_XVISUAL (gdk_drawable_get_visual (mozplay->target)),
	    expose->x + expose->width, expose->y + expose->height);
	cairo_t *cr = cairo_create (surface);
	swfmoz_player_render (mozplay, cr, region);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	gdk_region_destroy (region);
      }
      break;
    case Expose:
      if (!mozplay->windowless) {
	XExposeEvent *expose = (XExposeEvent *) event;
	GdkRectangle rect = { expose->x, expose->y, expose->width, expose->height };
	GdkRegion *region;
	region = gdk_region_rectangle (&rect);
	swfmoz_player_render (mozplay, NULL, region);
	gdk_region_destroy (region);
      }
      break;
    case ButtonPress:
      {
	XButtonEvent *button = (XButtonEvent *) event;
	swfmoz_player_mouse_press (mozplay, button->x - mozplay->target_rect.x, 
	    button->y - mozplay->target_rect.y, button->button);
	break;
      }
    case ButtonRelease:
      {
	XButtonEvent *button = (XButtonEvent *) event;
	swfmoz_player_mouse_release (mozplay, button->x - mozplay->target_rect.x, 
	    button->y - mozplay->target_rect.y, button->button);
	break;
      }
    case EnterNotify:
    case LeaveNotify:
      /* FIXME: implement */
      break;
    case MotionNotify:
      {
	if (mozplay->windowless) {
	  XMotionEvent *motion = (XMotionEvent *) event;
	  swfmoz_player_mouse_move (mozplay, motion->x - mozplay->target_rect.x,
	      motion->y - mozplay->target_rect.y);
	} else {
	  int winx, winy;

	  gdk_window_get_pointer (mozplay->target, &winx, &winy, NULL);
	  swfmoz_player_mouse_move (mozplay, winx, winy);
	}
	break;
      }
    case KeyPress:
    case KeyRelease:
      {
	/* try to mirror what the Gtk Widget does */
	guint keyval = 0, keycode = 0;
	XKeyEvent *key = (XKeyEvent *) event;
	gdk_keymap_translate_keyboard_state (gdk_keymap_get_default (), key->keycode,
	    key->state, 0, &keyval, NULL, NULL, NULL);
	if (keyval >= GDK_A && keyval <= GDK_Z)
	  keycode = keyval - GDK_A + SWFDEC_KEY_A;
	if (keyval >= GDK_a && keyval <= GDK_z)
	  keycode = keyval - GDK_a + SWFDEC_KEY_A;
	keycode = swfdec_gtk_keycode_from_hardware_keycode (key->keycode);
	if (keycode != 0) {
	  if (event->type == KeyPress) {
	    swfdec_player_key_press (player, keycode, gdk_keyval_to_unicode (keyval));
	  } else {
	    swfdec_player_key_release (player, keycode, gdk_keyval_to_unicode (keyval));
	  }
	}
	break;
      }
    case ConfigureNotify:
      {
	XConfigureEvent *conf = (XConfigureEvent *) event;

	if (!mozplay->windowless)
	  swfmoz_player_set_target (mozplay, mozplay->target, 0, 0, conf->width, conf->height);
	break;
      }
    default:
      g_printerr ("unhandled event %d\n", event->type);
      break;
  }
}

static GdkFilterReturn
plugin_x11_filter_event (GdkXEvent *gdkxevent, GdkEvent *unused, gpointer playerp)
{
  plugin_x11_handle_event (playerp, gdkxevent);
  return GDK_FILTER_REMOVE;
}

void
plugin_x11_setup_windowed (SwfmozPlayer *player, Window xwindow, 
    int x, int y, int width, int height)
{
  if (player->windowless) {
    if (player->target == NULL) {
      GdkWindow *window;
      if (!plugin_get_value (player->instance, NPNVnetscapeWindow, &xwindow) ||
	  (window = gdk_window_foreign_new (xwindow)) == NULL) {
	g_printerr ("cannot set window given for setup (id %lu)\n", xwindow);
	return;
      }
      swfmoz_player_set_target (player, window, x, y, width, height);
    } else {
      swfmoz_player_set_target (player, player->target, x, y, width, height);
    }
  } else {
    if (player->target == NULL) {
      GdkWindowAttr attr;
      GdkWindow *parent, *window;

      parent = gdk_window_foreign_new (xwindow);
      if (parent == NULL) {
	g_printerr ("invalid window given for setup (id %lu)\n", xwindow);
	return;
      }
      
      attr.event_mask = GDK_VISIBILITY_NOTIFY_MASK | GDK_EXPOSURE_MASK | 
	GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_PRESS_MASK |
	GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | 
	GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_KEY_PRESS_MASK |
	GDK_KEY_RELEASE_MASK;
      attr.x = 0;
      attr.y = 0;
      attr.width = width;
      attr.height = height;
      attr.window_type = GDK_WINDOW_CHILD;
      attr.wclass = GDK_INPUT_OUTPUT;
      window = gdk_window_new (parent, &attr, GDK_WA_X | GDK_WA_Y);
      gdk_window_add_filter (window, plugin_x11_filter_event, player);
      gdk_window_show (window);
      swfmoz_player_set_target (player, window, 0, 0, width, height);
    } else {
      gdk_window_move_resize (player->target, 0, 0, width, height);
    }
  }
}

void
plugin_x11_teardown (SwfmozPlayer *player)
{
  if (player->target) {
    gdk_window_remove_filter (player->target, plugin_x11_filter_event, player);
  }
  swfmoz_player_set_target (player, NULL, 0, 0, 0, 0);
}
