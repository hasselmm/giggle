/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Mathias Hasselmann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "giggle-avatar-image.h"

#include <gio/gio.h>
#include <string.h>

#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GIGGLE_TYPE_AVATAR_IMAGE, GiggleAvatarImagePriv))

enum {
	PROP_NONE,
	PROP_CACHE,
	PROP_PIXBUF,
	PROP_IMAGE_URI,
	PROP_IMAGE_SIZE,
	PROP_GRAVATAR_ID,
};

typedef struct {
	GiggleAvatarCache *cache;
	GdkPixbuf         *pixbuf;
	unsigned           image_size;
	char              *image_uri;
	char              *gravatar_id;
	GCancellable      *cancellable;
} GiggleAvatarImagePriv;

G_DEFINE_TYPE (GiggleAvatarImage,
	       giggle_avatar_image, GTK_TYPE_MISC)

static void
giggle_avatar_image_init (GiggleAvatarImage *image)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);
	priv->image_size = 160;
}

static void
avatar_image_real_set_cache (GiggleAvatarImage *image,
			     GiggleAvatarCache *cache)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);

	if (cache == priv->cache)
		return;

	if (priv->cache) {
		g_object_unref (priv->cache);
		priv->cache = NULL;
	}

	if (cache)
		priv->cache = g_object_ref (cache);
}

static void
avatar_image_real_set_pixbuf (GiggleAvatarImage *image,
			      GdkPixbuf         *pixbuf)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);

	if (pixbuf == priv->pixbuf)
		return;

	if (priv->pixbuf) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (pixbuf)
		priv->pixbuf = g_object_ref (pixbuf);

	gtk_widget_queue_resize (GTK_WIDGET (image));
}

static void
avatar_image_load_cb (GObject      *object,
		      GAsyncResult *result,
		      gpointer      user_data)
{
	GiggleAvatarCache *cache = GIGGLE_AVATAR_CACHE (object);
	GError            *error = NULL;
	GdkPixbuf         *pixbuf;

	pixbuf = giggle_avatar_cache_load_finish (cache, result, &error);

	if (pixbuf) {
		giggle_avatar_image_set_pixbuf (user_data, pixbuf);
	} else if (error) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_warning ("%s: %s", G_STRFUNC, error->message);

		g_error_free (error);
	}
}

static void
avatar_image_real_set_image_uri (GiggleAvatarImage *image,
				 const char        *uri)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);

	if (!g_strcmp0 (uri, priv->image_uri))
		return;

	g_free (priv->image_uri);
	priv->image_uri = g_strdup (uri);

	if (!priv->cache) {
		priv->cache = giggle_avatar_cache_new ();
		g_object_notify (G_OBJECT (image), "cache");
	}

	if (priv->cancellable) {
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
		priv->cancellable = NULL;
	}

	giggle_avatar_image_set_pixbuf (image, NULL);

	if (uri) {
		priv->cancellable = g_cancellable_new ();

		giggle_avatar_cache_load_uri_async
			(priv->cache, uri, G_PRIORITY_DEFAULT,
			 priv->cancellable, avatar_image_load_cb, image);
	}
}

static void
avatar_image_update_gravatar_uri (GiggleAvatarImage *image)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);
	char                  *uri = NULL;

	if (!priv->cache) {
		priv->cache = giggle_avatar_cache_new ();
		g_object_notify (G_OBJECT (image), "cache");
	}

	if (priv->gravatar_id) {
		uri = giggle_avatar_cache_get_gravatar_uri
			(priv->cache, priv->gravatar_id,
			 priv->image_size);
	}

	giggle_avatar_image_set_image_uri (image, uri);

	g_free (uri);
}

static void
avatar_image_real_set_image_size (GiggleAvatarImage *image,
				  unsigned           size)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);

	if (size == priv->image_size)
		return;

	priv->image_size = size;

	if (priv->gravatar_id) {
		avatar_image_update_gravatar_uri (image);
	} else {
		gtk_widget_queue_resize (GTK_WIDGET (image));
	}

}

static void
avatar_image_real_set_gravatar_id (GiggleAvatarImage *image,
				   const char        *id)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (image);

	if (!g_strcmp0 (id, priv->gravatar_id))
		return;

	g_free (priv->gravatar_id);
	priv->gravatar_id = g_strdup (id);

	avatar_image_update_gravatar_uri (image);
}

static void
avatar_image_set_property (GObject      *object,
			   guint         param_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GiggleAvatarImage *image = GIGGLE_AVATAR_IMAGE (object);

	switch (param_id) {
	case PROP_CACHE:
		avatar_image_real_set_cache
			(image, g_value_get_object (value));
		break;

	case PROP_PIXBUF:
		avatar_image_real_set_pixbuf
			(image, g_value_get_object (value));
		break;

	case PROP_IMAGE_URI:
		avatar_image_real_set_image_uri
			(image, g_value_get_string (value));
		break;

	case PROP_IMAGE_SIZE:
		avatar_image_real_set_image_size
			(image, g_value_get_uint (value));
		break;

	case PROP_GRAVATAR_ID:
		avatar_image_real_set_gravatar_id
			(image, g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
avatar_image_get_property (GObject      *object,
			   guint         param_id,
			   GValue       *value,
			   GParamSpec   *pspec)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_CACHE:
		g_value_set_object (value, priv->cache);
		break;

	case PROP_PIXBUF:
		g_value_set_object (value, priv->pixbuf);
		break;

	case PROP_IMAGE_URI:
		g_value_set_string (value, priv->image_uri);
		break;

	case PROP_IMAGE_SIZE:
		g_value_set_uint (value, priv->image_size);
		break;

	case PROP_GRAVATAR_ID:
		g_value_set_string (value, priv->gravatar_id);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
avatar_image_finalize (GObject *object)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (object);

	if (priv->cache)
		g_object_unref (priv->cache);
	if (priv->pixbuf)
		g_object_unref (priv->pixbuf);

	if (priv->cancellable) {
		g_cancellable_cancel (priv->cancellable);
		g_object_unref (priv->cancellable);
	}

	g_free (priv->image_uri);
	g_free (priv->gravatar_id);

	G_OBJECT_CLASS (giggle_avatar_image_parent_class)->finalize (object);
}

static void
rounded_rectangle (cairo_t *cr,
                   double   x,
                   double   y,
                   double   w,
                   double   h,
                   double   r)
{
    /*   A----BQ
     *  H      C
     *  |      |
     *  G      D
     *   F----E
     */

    cairo_move_to  (cr, x + r,     y);          /* Move to A */
    cairo_line_to  (cr, x + w - r, y);          /* Straight line to B */
    cairo_curve_to (cr, x + w,     y,           /* Curve to C, */
                        x + w,     y,           /* control points are both at Q */
                        x + w,     y + r);
    cairo_line_to  (cr, x + w,     y + h - r);  /* Move to D */
    cairo_curve_to (cr, x + w,     y + h,       /* Curve to E */
                        x + w,     y + h,
                        x + w - r, y + h);
    cairo_line_to  (cr, x + r,     y + h);      /* Line to F */
    cairo_curve_to (cr, x,         y + h,       /* Curve to G */
                        x,         y + h,
                        x,         y + h - r);
    cairo_line_to  (cr, x,         y + r);      /* Line to H */
    cairo_curve_to (cr, x,         y,
                        x,         y,
                        x + r,     y);          /* Curve to A */
}

static gboolean
avatar_image_expose_event (GtkWidget      *widget,
			   GdkEventExpose *event)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (widget);
	float                  xalign, yalign;
	double                 x, y;
	int                    w, h;
	cairo_t               *cr;

	cr = gdk_cairo_create (event->window);
	gdk_cairo_region (cr, event->region);
	cairo_clip (cr);

	w = widget->requisition.width;
	h = widget->requisition.height;

	gtk_misc_get_alignment (GTK_MISC (widget), &xalign, &yalign);

	cairo_translate
		(cr, (int) ((widget->allocation.width - w) * xalign),
		 (int) ((widget->allocation.height - h) * yalign));

	rounded_rectangle (cr, 0.5, 0.5, w - 1, h - 1, MIN (w, h) * 0.2);
	gdk_cairo_set_source_color (cr, &widget->style->base[GTK_STATE_NORMAL]);
	cairo_fill_preserve (cr);

	if (priv->pixbuf) {
		x = (w - gdk_pixbuf_get_width (priv->pixbuf)) * 0.5;
		y = (h - gdk_pixbuf_get_height (priv->pixbuf)) * 0.5;

		gdk_cairo_set_source_pixbuf (cr, priv->pixbuf, x, y);
		cairo_fill_preserve (cr);
	}

	gdk_cairo_set_source_color (cr, &widget->style->text[GTK_STATE_NORMAL]);
	cairo_set_line_width (cr, 1);
	cairo_stroke (cr);

	cairo_destroy (cr);

	return TRUE;
}

static void
avatar_image_size_request (GtkWidget      *widget,
			   GtkRequisition *requisition)
{
	GiggleAvatarImagePriv *priv = GET_PRIV (widget);

	if (priv->pixbuf) {
		requisition->width = gdk_pixbuf_get_width (priv->pixbuf);
		requisition->height = gdk_pixbuf_get_height (priv->pixbuf);
	} else {
		requisition->width = requisition->height = priv->image_size;
	}
}

static void
giggle_avatar_image_class_init (GiggleAvatarImageClass *class)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

	object_class->set_property = avatar_image_set_property;
	object_class->get_property = avatar_image_get_property;
	object_class->finalize     = avatar_image_finalize;

	widget_class->expose_event = avatar_image_expose_event;
	widget_class->size_request = avatar_image_size_request;

	g_object_class_install_property
		(object_class, PROP_CACHE,
		 g_param_spec_object
		 	("cache", "Cache",
			 "The avatar cache of this widget",
			 GDK_TYPE_PIXBUF,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_PIXBUF,
		 g_param_spec_object
		 	("pixbuf", "Pixbuf",
			 "The current pixbuf of this widget",
			 GDK_TYPE_PIXBUF,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_IMAGE_SIZE,
		 g_param_spec_uint
		 	("image-size", "Image Size",
			 "The size of this avatar",
			 0, G_MAXINT, 160,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_IMAGE_URI,
		 g_param_spec_string
		 	("image-uri", "Image URI",
			 "The URI for this avatar",
			 NULL,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_object_class_install_property
		(object_class, PROP_GRAVATAR_ID,
		 g_param_spec_string
		 	("gravatar-id", "Gravatar Id",
			 "The Gravatar id for this avatar",
			 NULL,
			 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (class, sizeof (GiggleAvatarImagePriv));
}

GtkWidget *
giggle_avatar_image_new (void)
{
	return g_object_new (GIGGLE_TYPE_AVATAR_IMAGE, NULL);
}

void
giggle_avatar_image_set_cache (GiggleAvatarImage *image,
			       GiggleAvatarCache *cache)
{
	g_return_if_fail (GIGGLE_IS_AVATAR_IMAGE (image));
	g_return_if_fail (GIGGLE_IS_AVATAR_CACHE (cache));
	g_object_set (image, "cache", cache, NULL);
}

GiggleAvatarCache *
giggle_avatar_image_get_cache (GiggleAvatarImage *image)
{
	g_return_val_if_fail (GIGGLE_IS_AVATAR_IMAGE (image), NULL);
	return GET_PRIV (image)->cache;
}

void
giggle_avatar_image_set_pixbuf (GiggleAvatarImage *image,
				GdkPixbuf         *pixbuf)
{
	g_return_if_fail (GIGGLE_IS_AVATAR_IMAGE (image));
	g_return_if_fail (GDK_IS_PIXBUF (pixbuf) || !pixbuf);
	g_object_set (image, "pixbuf", pixbuf, NULL);
}

GdkPixbuf *
giggle_avatar_image_get_pixbuf   (GiggleAvatarImage *image)
{
	g_return_val_if_fail (GIGGLE_IS_AVATAR_IMAGE (image), NULL);
	return GET_PRIV (image)->pixbuf;
}

void
giggle_avatar_image_set_image_size (GiggleAvatarImage *image,
				    unsigned           size)
{
	g_return_if_fail (GIGGLE_IS_AVATAR_IMAGE (image));
	g_object_set (image, "image-size", size, NULL);
}

unsigned
giggle_avatar_image_get_image_size (GiggleAvatarImage *image)
{
	g_return_val_if_fail (GIGGLE_IS_AVATAR_IMAGE (image), 0);
	return GET_PRIV (image)->image_size;
}

void
giggle_avatar_image_set_image_uri (GiggleAvatarImage *image,
				   const char        *uri)
{
	g_return_if_fail (GIGGLE_IS_AVATAR_IMAGE (image));
	g_object_set (image, "image-uri", uri, NULL);
}

G_CONST_RETURN char *
giggle_avatar_image_get_image_uri (GiggleAvatarImage *image)
{
	g_return_val_if_fail (GIGGLE_IS_AVATAR_IMAGE (image), NULL);
	return GET_PRIV (image)->image_uri;
}

void
giggle_avatar_image_set_gravatar_id (GiggleAvatarImage *image,
				     const char        *id)
{
	g_return_if_fail (GIGGLE_IS_AVATAR_IMAGE (image));
	g_object_set (image, "gravatar-id", id, NULL);
}

G_CONST_RETURN char *
giggle_avatar_image_get_gravatar_id (GiggleAvatarImage *image)
{
	g_return_val_if_fail (GIGGLE_IS_AVATAR_IMAGE (image), NULL);
	return GET_PRIV (image)->gravatar_id;
}
