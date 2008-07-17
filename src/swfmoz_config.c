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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfmoz_config.h"

G_DEFINE_TYPE (SwfmozConfig, swfmoz_config, G_TYPE_OBJECT)

#define SWFMOZ_CONFIG_FILE "swfdec-mozilla.conf"

static SwfmozConfig *singleton_config = NULL;

static gboolean
swfmoz_config_save_file (SwfmozConfig *config)
{
  gchar *data;
  gsize data_size;
  GError *error = NULL;
  gboolean has_global;

  gchar *keyfile_name = g_build_filename (g_get_user_config_dir (),
					  SWFMOZ_CONFIG_FILE, NULL);

  has_global = g_key_file_has_key (config->keyfile, "global", "autoplay",
				   &error);
  if (error) {
    g_error_free (error);
    error = NULL;
  } else if (!has_global) {
    g_key_file_set_boolean (config->keyfile, "global", "autoplay", FALSE);
  }

  data = g_key_file_to_data (config->keyfile, &data_size, &error);
  if (error) {
    goto fail;
  }

  g_file_set_contents (keyfile_name, data, data_size, &error);
  if (error) {
    goto fail;
  }

  g_free (data);
  g_free (keyfile_name);

  return TRUE;

fail:
  g_printerr ("Unable to write new config file: %s\n", error->message);
  g_error_free (error);
  error = NULL;

  g_free (data);
  g_free (keyfile_name);

  return FALSE;
}

static GKeyFile *
swfmoz_config_read_file (void)
{
  gchar *keyfile_name = g_build_filename (g_get_user_config_dir (),
					  SWFMOZ_CONFIG_FILE, NULL);
  GKeyFile *keyfile;
  GError *error = NULL;

  keyfile = g_key_file_new ();
  if (!g_key_file_load_from_file (keyfile, keyfile_name, G_KEY_FILE_NONE,
				  &error)) {
      g_error_free (error);
      error = NULL;
  }

  g_free (keyfile_name);
  return keyfile;
}

static gboolean
swfmoz_config_read_autoplay (SwfmozConfig *config, const char *host,
			     gboolean autoplay)
{
  GError *error = NULL;
  gboolean tmp_autoplay;

  tmp_autoplay = g_key_file_get_boolean (config->keyfile, host, "autoplay",
					 &error);
  if (error) {
    g_error_free (error);
  } else {
    autoplay = tmp_autoplay;
  }

  return autoplay;
}

gboolean
swfmoz_config_should_autoplay (SwfmozConfig *config, const SwfdecURL *url)
{
  const gchar *host;
  gboolean autoplay = FALSE;

  g_return_val_if_fail (SWFMOZ_IS_CONFIG (config), FALSE);

  host = swfdec_url_get_host (url);
  if (host == NULL)
    host = swfdec_url_get_protocol (url);

  autoplay = swfmoz_config_read_autoplay (config, "global", autoplay);
  autoplay = swfmoz_config_read_autoplay (config, host, autoplay);

  return autoplay;
}

void
swfmoz_config_set_autoplay (SwfmozConfig *config, const SwfdecURL *url,
			    gboolean autoplay)
{
  g_return_if_fail (SWFMOZ_IS_CONFIG (config));

  g_key_file_set_boolean (config->keyfile, swfdec_url_get_host (url),
			  "autoplay", autoplay);

  swfmoz_config_save_file (config);
}

static void
swfmoz_config_dispose (GObject *object)
{
  SwfmozConfig *config = SWFMOZ_CONFIG (object);

  /*Just in case this wasn't initialized via swfmoz_config_new */
  if (config == singleton_config) {
      singleton_config = NULL;
  }

  g_key_file_free (config->keyfile);
  config->keyfile = NULL;

  G_OBJECT_CLASS (swfmoz_config_parent_class)->dispose (object);
}

static void
swfmoz_config_class_init (SwfmozConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfmoz_config_dispose;
}

static void
swfmoz_config_init (SwfmozConfig *config)
{
  config->keyfile = swfmoz_config_read_file ();
}

SwfmozConfig *
swfmoz_config_new (void)
{
  if (!singleton_config) {
    singleton_config = SWFMOZ_CONFIG (g_object_new (SWFMOZ_TYPE_CONFIG, NULL));
  } else {
    g_object_ref (G_OBJECT (singleton_config));
  }

  return singleton_config;
}
