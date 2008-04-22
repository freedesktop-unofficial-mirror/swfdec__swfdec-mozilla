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

#ifndef _SWFMOZ_PLUGIN_H_
#define _SWFMOZ_PLUGIN_H_

#include <glib.h>
#include <npapi.h>

G_BEGIN_DECLS


gboolean	plugin_get_value		(NPP		instance,
						 NPNVariable	var,
						 gpointer	data);

void		plugin_get_url			(NPP		instance,
						 const char *	url,
						 const char *	target);
void		plugin_get_url_notify		(NPP		instance,
						 const char *	url,
						 const char *	target,
						 void *		data);
void		plugin_post_url			(NPP		instance,
						 const char *	url,
						 const char *	target,
						 const char *	data,
						 guint		data_len);
void		plugin_post_url_notify		(NPP		instance,
						 const char *	url,
						 const char *	target,
						 const char *	data,
						 guint		data_len,
						 void *		user_data);

void		plugin_destroy_stream		(NPP		instance,
						 NPStream *	stream);

void		plugin_invalidate_rect		(NPP		instance,
						 NPRect *	rect);

gboolean	plugin_push_allow_popups	(NPP		instance,
						 gboolean	allow);
gboolean	plugin_pop_allow_popups		(NPP		instance);

G_END_DECLS
#endif
