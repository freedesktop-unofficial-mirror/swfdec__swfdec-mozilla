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

#include <swfdec-gtk/swfdec-gtk.h>
#include <npapi.h>
#include "swfmoz_config.h"
#include "swfmoz_loader.h"

G_BEGIN_DECLS

enum {
  SWFMOZ_LOADER_COLUMN_LOADER,
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
  SwfdecGtkPlayer	player;

  NPP			instance;		/* the mozilla plugin */

  GMainContext *	context;		/* the main context run by the thread */

  NPStream *		initial;		/* loader that spawned this player or NULL if none yet */
  gboolean		windowless;		/* TRUE if player communicates with the windowing system via the browser */
  gboolean		opaque;			/* TRUE if the player should not allow translucency */
  GdkWindow *		target;			/* what we draw to */
  GdkRectangle		target_rect;		/* area in target that this plugin occupies */

  GSource *		repaint_source;		/* set when repaint is necessary */
  GdkRegion *		repaint;		/* area to repaint or NULL if none */

  /* Gtk stuff */
  guint			no_release;		/* for disabling release event when closing right-click menu */
  GtkMenu *		menu;			/* right-click menu */
  GtkTreeModel *	loaders;		/* loaders used in this players */
  SwfmozConfig *	config;			/* autoplay configuration */
  GtkWidget *		fullscreen;		/* fullscreen widget */
};

struct _SwfmozPlayerClass {
  SwfdecGtkPlayerClass	player_class;
};

GType		swfmoz_player_get_type   	(void);

SwfdecPlayer *	swfmoz_player_new	  	(NPP			instance,
						 gboolean		windowless,
						 gboolean		opaque);
void		swfmoz_player_remove		(SwfmozPlayer *		player);

gboolean	swfmoz_player_set_initial_stream (SwfmozPlayer *	player,
						 NPStream *		stream);
void		swfmoz_player_add_loader	(SwfmozPlayer *		player,
						 SwfmozLoader *		loader);
void		swfmoz_player_set_target	(SwfmozPlayer *		player,
						 GdkWindow *		target,
						 int			x,
						 int			y,
						 int			width,
						 int			height);
void		swfmoz_player_set_allow_popups	(SwfmozPlayer *		player,
						 gboolean		allow);
void		swfmoz_player_render		(SwfmozPlayer *		player,
						 cairo_t *		cr,
						 GdkRegion *		region);
gboolean	swfmoz_player_mouse_press	(SwfmozPlayer *		player,
						 int			x,
						 int			y,
						 guint			button);
gboolean	swfmoz_player_mouse_release	(SwfmozPlayer *		player,
						 int			x,
						 int			y,
						 guint			button);
gboolean	swfmoz_player_mouse_move	(SwfmozPlayer *		player,
						 int			x,
						 int			y);

char *		swfmoz_player_get_filename	(SwfmozPlayer *		player);
void		swfmoz_player_add_variables	(SwfmozPlayer *		player,
						 const char *		variables);
					 

G_END_DECLS
#endif
