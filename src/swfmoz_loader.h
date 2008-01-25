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

#ifndef _SWFMOZ_LOADER_H_
#define _SWFMOZ_LOADER_H_

#include <npapi.h>
#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _SwfmozLoader SwfmozLoader;
typedef struct _SwfmozLoaderClass SwfmozLoaderClass;

#define SWFMOZ_TYPE_LOADER                    (swfmoz_loader_get_type())
#define SWFMOZ_IS_LOADER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFMOZ_TYPE_LOADER))
#define SWFMOZ_IS_LOADER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFMOZ_TYPE_LOADER))
#define SWFMOZ_LOADER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFMOZ_TYPE_LOADER, SwfmozLoader))
#define SWFMOZ_LOADER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFMOZ_TYPE_LOADER, SwfmozLoaderClass))
#define SWFMOZ_LOADER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFMOZ_TYPE_LOADER, SwfmozLoaderClass))

struct _SwfmozLoader
{
  SwfdecLoader		parent;

  NPP			instance;	/* instance we belong to */
  NPStream *		stream;		/* stream we do or NULL if not created yet */

  char *		cache_file;	/* where the file is cached */
  gboolean		open;		/* TRUE when data has arrived */
};

struct _SwfmozLoaderClass
{
  SwfdecLoaderClass	loader_class;
};

GType		swfmoz_loader_get_type   	(void);

void		swfmoz_loader_set_stream	(SwfmozLoader *	loader,
						 NPP		instance,
						 NPStream *	stream);
void		swfmoz_loader_ensure_open	(SwfmozLoader *	loader);
					 
const char *	swfmoz_loader_get_data_type_string
						(SwfdecLoader *	loader);

G_END_DECLS
#endif
