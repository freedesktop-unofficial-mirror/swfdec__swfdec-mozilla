/* Swfdec Mozilla Plugin
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFMOZ_DIALOG_H_
#define _SWFMOZ_DIALOG_H_

#include <gtk/gtk.h>
#include "swfmoz_player.h"

G_BEGIN_DECLS


typedef struct _SwfmozDialog SwfmozDialog;
typedef struct _SwfmozDialogClass SwfmozDialogClass;

#define SWFMOZ_TYPE_DIALOG                    (swfmoz_dialog_get_type())
#define SWFMOZ_IS_DIALOG(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFMOZ_TYPE_DIALOG))
#define SWFMOZ_IS_DIALOG_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFMOZ_TYPE_DIALOG))
#define SWFMOZ_DIALOG(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFMOZ_TYPE_DIALOG, SwfmozDialog))
#define SWFMOZ_DIALOG_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFMOZ_TYPE_DIALOG, SwfmozDialogClass))
#define SWFMOZ_DIALOG_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFMOZ_TYPE_DIALOG, SwfmozDialogClass))

struct _SwfmozDialog {
  GtkDialog		dialog;

  SwfmozPlayer *	player;		/* the player we show info about */

  GtkWidget *		media;		/* GtkTreeView containing the media */
};

struct _SwfmozDialogClass {
  GtkDialogClass	dialog_class;
};

GType		swfmoz_dialog_get_type   	(void);

void		swfmoz_dialog_show	  	(SwfmozPlayer *		player);
void		swfmoz_dialog_remove		(SwfmozPlayer *		player);
					 

G_END_DECLS
#endif
