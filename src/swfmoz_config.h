/* Swfdec Mozilla Plugin
 * Copyright (C) 2008 James Bowes <jbowes@dangerouslyinc.com>
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

#ifndef _SWFMOZ_CONFIG_H_
#define _SWFMOZ_CONFIG_H_

#include <glib.h>
#include <swfdec/swfdec.h>

G_BEGIN_DECLS

typedef struct _SwfmozConfig SwfmozConfig;
typedef struct _SwfmozConfigClass SwfmozConfigClass;

#define SWFMOZ_TYPE_CONFIG                    (swfmoz_config_get_type())
#define SWFMOZ_IS_CONFIG(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFMOZ_TYPE_CONFIG))
#define SWFMOZ_IS_CONFIG_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFMOZ_TYPE_CONFIG))
#define SWFMOZ_CONFIG(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFMOZ_TYPE_CONFIG, SwfmozConfig))
#define SWFMOZ_CONFIG_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFMOZ_TYPE_CONFIG, SwfmozConfigClass))
#define SWFMOZ_CONFIG_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFMOZ_TYPE_CONFIG, SwfmozConfigClass))

struct _SwfmozConfig
{
  GObject	parent;
  GKeyFile *	keyfile;	/* Config file contents */
};

struct _SwfmozConfigClass
{
 GObjectClass 	parent;
};

GType		swfmoz_config_get_type			(void);

SwfmozConfig *	swfmoz_config_new			(void);
gboolean	swfmoz_config_read_autoplay		(SwfmozConfig *config,
							 const char *host,
							 gboolean autoplay);
gboolean 	swfmoz_config_should_autoplay 		(SwfmozConfig *config,
							 const SwfdecURL *url);
void		swfmoz_config_set_autoplay 		(SwfmozConfig *config,
							 const SwfdecURL *url,
							 gboolean autoplay);
gboolean	swfmoz_config_has_global_key		(SwfmozConfig *config);
void		swfmoz_config_set_global_autoplay	(SwfmozConfig *config,
							 gboolean autoplay);
void		swfmoz_config_remove_global_autoplay	(SwfmozConfig *config);

G_END_DECLS
#endif
