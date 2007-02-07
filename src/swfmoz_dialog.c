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

#include "swfmoz_dialog.h"
#include "plugin.h"
#include "swfdec_playback.h"
#include "swfdec_source.h"
#include "swfmoz_loader.h"

/*** SWFMOZ_DIALOG ***/

G_DEFINE_TYPE (SwfmozDialog, swfmoz_dialog, GTK_TYPE_DIALOG)

static void
swfmoz_dialog_dispose (GObject *object)
{
  SwfmozDialog *dialog = SWFMOZ_DIALOG (object);

  g_object_unref (dialog->player);

  G_OBJECT_CLASS (swfmoz_dialog_parent_class)->dispose (object);
}

static void
swfmoz_dialog_class_init (SwfmozDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfmoz_dialog_dispose;
}

static void
swfmoz_dialog_do_save_media (GtkFileChooser *chooser, gint response, SwfmozLoader *loader)
{
  GtkWidget *dialog;
  GError *error = NULL;

  g_assert (loader->cache_file);
  if (response == GTK_RESPONSE_ACCEPT) {
    GMappedFile *file;
    char *target;
    target = gtk_file_chooser_get_filename (chooser);
    if (target == NULL) {
      g_set_error (&error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	  "No file selected");
      goto error;
    }
    file = g_mapped_file_new (loader->cache_file, FALSE, &error);
    if (file == NULL)
      goto error;
    g_file_set_contents (target, g_mapped_file_get_contents (file),
	g_mapped_file_get_length (file), &error);
    g_mapped_file_free (file);
    if (error)
      goto error;
  }
    
  gtk_widget_destroy (GTK_WIDGET (chooser));
  return;

error:
  dialog = gtk_message_dialog_new (gtk_window_get_transient_for (GTK_WINDOW (chooser)),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, 
      "%s", error->message);
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_window_present (GTK_WINDOW (dialog));
  g_error_free (error);
  gtk_widget_destroy (GTK_WIDGET (chooser));
  return;
}

static void
swfmoz_dialog_save_media (GtkButton *button, SwfmozDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  SwfmozLoader *loader;
  GtkWidget *chooser;
  char *s, *filename;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->media));
  /* FIXME: assert this doesn't happen? */
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;
  
  gtk_tree_model_get (model, &iter, SWFMOZ_LOADER_COLUMN_LOADER, &loader, -1);
  g_object_unref (loader);
  if (loader->cache_file == NULL) {
    /* FIXME: redownload the file */
    g_printerr ("The file \"%s\" is not available locally\n", 
	SWFDEC_LOADER (loader)->url);
    return;
  }
  
  filename = swfdec_loader_get_filename (SWFDEC_LOADER (loader));
  s = g_strdup_printf ("Save \"%s\"", filename);
  chooser = gtk_file_chooser_dialog_new (s, GTK_WINDOW (dialog),
      GTK_FILE_CHOOSER_ACTION_SAVE, 
      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
  g_free (s);
  s = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
  g_free (filename);
  if (s) {
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), s);
    g_free (s);
  }

  gtk_window_set_destroy_with_parent (GTK_WINDOW (chooser), TRUE);
  g_signal_connect (chooser, "response", G_CALLBACK (swfmoz_dialog_do_save_media), loader);
  gtk_window_present (GTK_WINDOW (chooser));
}

static GtkWidget *
swfmoz_dialog_get_media_page (SwfmozDialog *dialog)
{
  GtkWidget *vbox, *align, *widget;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  
  vbox = gtk_vbox_new (FALSE, 3);
  
  dialog->media = widget = gtk_tree_view_new_with_model (dialog->player->loaders);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("URL", renderer,
    "text", SWFMOZ_LOADER_COLUMN_URL, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sort_column_id (column, SWFMOZ_LOADER_COLUMN_URL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Type", renderer,
    "text", SWFMOZ_LOADER_COLUMN_TYPE, NULL);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sort_column_id (column, SWFMOZ_LOADER_COLUMN_URL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);

  align = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (align),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);

  widget = gtk_button_new_with_mnemonic ("_Save media as");
  g_signal_connect (widget, "clicked", G_CALLBACK (swfmoz_dialog_save_media), dialog);
  align = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), widget);
  gtk_box_pack_end (GTK_BOX (vbox), align, FALSE, TRUE, 0);
  
  return vbox;
}

static void
swfmoz_dialog_init (SwfmozDialog *dialog)
{
  GtkDialog *gtkdialog = GTK_DIALOG (dialog);

  gtk_dialog_add_button (gtkdialog, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 400);
}

static void
swfmoz_dialog_set_player (SwfmozDialog *dialog, SwfmozPlayer *player)
{
  GtkWidget *notebook;

  g_object_ref (player);
  dialog->player = player;

  notebook = gtk_notebook_new ();
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
      swfmoz_dialog_get_media_page (dialog),
      gtk_label_new ("Media"));
  gtk_widget_show_all (notebook);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), notebook);
}

void
swfmoz_dialog_show (SwfmozPlayer *player)
{
  SwfmozDialog *dialog;
  GQuark quark = 0;

  g_return_if_fail (SWFMOZ_IS_PLAYER (player));

  if (quark == 0)
    quark = g_quark_from_static_string ("swfmoz-dialog");
  dialog = g_object_get_qdata (G_OBJECT (player), quark);
  if (dialog == NULL) {
    dialog = g_object_new (SWFMOZ_TYPE_DIALOG, NULL);
    g_object_set_qdata (G_OBJECT (player), quark, dialog);
    swfmoz_dialog_set_player (dialog, player);
  }
  gtk_window_present (GTK_WINDOW (dialog));
}
