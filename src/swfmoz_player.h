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

#ifndef _SWFMOZ_PLAYER_H_
#define _SWFMOZ_PLAYER_H_

#include <libswfdec-gtk/swfdec-gtk.h>
#include <npapi.h>

G_BEGIN_DECLS

enum {
  SWFMOZ_LOADER_COLUMN_LOADER,
  SWFMOZ_LOADER_COLUMN_NAME,
  SWFMOZ_LOADER_COLUMN_URL,
  SWFMOZ_LOADER_COLUMN_TYPE,
  SWFMOZ_LOADER_COLUMN_EOF,
  SWFMOZ_LOADER_COLUMN_ERROR,
  SWFMOZ_LOADER_COLUMN_PERCENT_LOADED,
  SWFMOZ_LOADER_N_COLUMNS
};

typedef struct _SwfmozPlayer SwfmozPlayer;
typedef struct _SwfmozPlayerClass SwfmozPlayerClass;

#define SWFMOZ_TYPE_PLAYER                    (swfmoz_player_get_type())
#define SWFMOZ_IS_PLAYER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFMOZ_TYPE_PLAYER))
#define SWFMOZ_IS_PLAYER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFMOZ_TYPE_PLAYER))
#define SWFMOZ_PLAYER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFMOZ_TYPE_PLAYER, SwfmozPlayer))
#define SWFMOZ_PLAYER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFMOZ_TYPE_PLAYER, SwfmozPlayerClass))
#define SWFMOZ_PLAYER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFMOZ_TYPE_PLAYER, SwfmozPlayerClass))

struct _SwfmozPlayer {
  GObject		object;

  NPP			instance;		/* the mozilla plugin */

  GMainContext *	context;		/* the main context run by the thread */

  SwfdecPlayer *	player;			/* the player instance */
  SwfdecLoader *	initial;		/* loader that spawned this player or NULL if none yet */
  char *		variables;		/* variables to pass to initial loader */
  gboolean		windowless;		/* TRUE if player communicates with the windowing system via the browser */
  cairo_t *		target;			/* what we draw to */
  cairo_t *		intermediate;		/* intermediate target to avoid flicker */
  unsigned int		target_width;		/* width of target */
  unsigned int		target_height;		/* height of target */
  gboolean		mouse_down;		/* saved state for the mouse */

  /* for windowed operation */
  GSource *		repaint_source;		/* set when repaint is necessary */
  int			x;			/* area to repaint */
  int			y;
  int			width;
  int			height;

  /* Gtk stuff */
  GtkMenu *		menu;			/* right-click menu */
  GtkTreeModel *	loaders;		/* loaders used in this players */
};

struct _SwfmozPlayerClass {
  GObjectClass		object_class;
};

GType		swfmoz_player_get_type   	(void);

SwfmozPlayer *	swfmoz_player_new	  	(NPP			instance,
						 gboolean		windowless);
void		swfmoz_player_remove		(SwfmozPlayer *		player);

SwfdecLoader *	swfmoz_player_add_stream	(SwfmozPlayer *		player,
						 NPStream *		stream);
void		swfmoz_player_set_target	(SwfmozPlayer *		player,
						 cairo_t *		cr,
						 unsigned int		width,
						 unsigned int		height);
void		swfmoz_player_render		(SwfmozPlayer *		player,
						 int			x,
						 int			y,
						 int			width,
						 int			height);
gboolean	swfmoz_player_mouse_changed	(SwfmozPlayer *		player,
						 int			button,
						 int			x,
						 int			y,
						 gboolean		down);
gboolean	swfmoz_player_mouse_moved	(SwfmozPlayer *		player,
						 int			x,
						 int			y);

char *		swfmoz_player_get_filename	(SwfmozPlayer *		player);
void		swfmoz_player_set_variables	(SwfmozPlayer *		player,
						 const char *		variables);
					 

G_END_DECLS
#endif
