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

#include <cairo-xlib.h>

#include "swfmoz_player.h"

/*** GSource for Display handling ***/

/* This code is based upon GDKs implementation of a source for X11
 * Display handling */

typedef gboolean (* DisplaySourceHandler) (gpointer user_data, XEvent *event);
typedef struct {
  GSource source;

  Display *display;
  GPollFD event_poll_fd;
} DisplaySource;

static gboolean
display_source_prepare (GSource *source, gint *timeout)
{
  Display *disp = ((DisplaySource*) source)->display;

  *timeout = -1;
  return XPending (disp) > 0;
}

static gboolean
display_source_check (GSource *source)
{
  Display *disp = ((DisplaySource*) source)->display;

  return XPending (disp) > 0;
}

static gboolean
display_source_dispatch (GSource *source, GSourceFunc callback, gpointer user_data)
{
  Display *disp = ((DisplaySource*) source)->display;

  if (XPending (disp)) {
    XEvent event;
    DisplaySourceHandler handler = (DisplaySourceHandler) callback;
    XNextEvent (disp, &event);
    handler (user_data, &event);
  }
  return TRUE;
}

static GSourceFuncs display_source_funcs = {
  display_source_prepare,
  display_source_check,
  display_source_dispatch,
  NULL
};

static GSource *
display_source_new (Display *display)
{
  GSource *ret;
  DisplaySource *source;

  g_return_val_if_fail (display != NULL, NULL);

  ret = g_source_new (&display_source_funcs, sizeof (DisplaySource));
  source = (DisplaySource *) ret;
  source->display = display;
  source->event_poll_fd.fd = ConnectionNumber (display);
  source->event_poll_fd.events = G_IO_IN;
  
  g_source_add_poll (ret, &source->event_poll_fd);
  g_source_set_can_recurse (ret, TRUE);
  return ret;
}

/*** Plugin code ***/

typedef struct {
  Display *	display;
  GSource *	source;
  Window	window;
} SwfmozX11Data;

static void
swfmoz_x11_data_free (SwfmozX11Data *data)
{
  if (data == NULL)
    return;

  g_source_destroy (data->source);
  g_source_unref (data->source);
  XDestroyWindow (data->display, data->window);
  XCloseDisplay (data->display);
  g_free (data);
}

static void
swfmoz_x11_data_set (SwfmozPlayer *player, SwfmozX11Data *data)
{
  static GQuark quark = 0;

  if (quark == 0)
    quark = g_quark_from_static_string ("swfmoz-x11-data");

  g_object_set_qdata_full (G_OBJECT (player), quark, data,
      (GDestroyNotify) swfmoz_x11_data_free);
}

gboolean
plugin_x11_handle_event (SwfmozPlayer *player, XEvent *event)
{
  switch (event->type) {
    case Expose:
      {
	XExposeEvent *expose = (XExposeEvent *) event;
	swfmoz_player_render (player, expose->x, expose->y, expose->width, expose->height);
	break;
      }
    default:
      g_print ("unhandled event %d\n", event->type);
      break;
  }
  return TRUE;
}

void
plugin_x11_setup_windowed (SwfmozPlayer *player, const char *display_name,
    Window window, int width, int height)
{
  SwfmozX11Data *data = g_new0 (SwfmozX11Data, 1);
  cairo_t *cr;
  cairo_surface_t *surface;
  XWindowAttributes attr;

  swfmoz_x11_data_set (player, data);
  data->display = XOpenDisplay (display_name);
  data->source = display_source_new (data->display);
  g_source_set_callback (data->source, (GSourceFunc) plugin_x11_handle_event,
    player, NULL);
  g_source_attach (data->source, player->context);
  data->window = XCreateSimpleWindow (data->display, window, 0, 0, width, height,
      0, 0, 0);
  XMapWindow (data->display, data->window);
  XSelectInput (data->display, data->window, ExposureMask);

  XGetWindowAttributes (data->display, data->window, &attr);
  surface = cairo_xlib_surface_create (data->display, data->window,
      attr.visual, width, height);
  cr = cairo_create (surface);
  swfmoz_player_set_target (player, cr);
  cairo_surface_destroy (surface);
}

void
plugin_x11_teardown (SwfmozPlayer *player)
{
  swfmoz_x11_data_set (player, NULL);
  swfmoz_player_set_target (player, NULL);
}
