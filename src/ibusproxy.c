/* vim:set et sts=4: */
/* ibus - The Input Bus
 * Copyright (C) 2008-2009 Huang Peng <shawn.p.huang@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "ibusproxy.h"
#include "ibusinternal.h"

#define IBUS_PROXY_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), IBUS_TYPE_PROXY, IBusProxyPrivate))

enum {
    DBUS_SIGNAL,
    LAST_SIGNAL,
};

enum {
    PROP_0,
    PROP_NAME,
    PROP_PATH,
    PROP_CONNECTION,
};


/* IBusProxyPriv */
struct _IBusProxyPrivate {
    gchar *name;
    gchar *path;
    IBusConnection *connection;
};
typedef struct _IBusProxyPrivate IBusProxyPrivate;

static guint            _signals[LAST_SIGNAL] = { 0 };

/* functions prototype */
static void      ibus_proxy_class_init  (IBusProxyClass *klass);
static void      ibus_proxy_init        (IBusProxy      *proxy);
static GObject  *ibus_proxy_constructor (GType           type,
                                         guint           n_construct_params,
                                         GObjectConstructParam *construct_params);
static void      ibus_proxy_destroy     (IBusProxy       *proxy);
static void      ibus_proxy_set_property(IBusProxy       *proxy,
                                         guint           prop_id,
                                         const GValue    *value,
                                         GParamSpec      *pspec);
static void      ibus_proxy_get_property(IBusProxy       *proxy,
                                         guint           prop_id,
                                         GValue          *value,
                                         GParamSpec      *pspec);

static gboolean  ibus_proxy_dbus_signal (IBusProxy       *proxy,
                                         DBusMessage     *message);

static IBusObjectClass  *_parent_class = NULL;

GType
ibus_proxy_get_type (void)
{
    static GType type = 0;

    static const GTypeInfo type_info = {
        sizeof (IBusProxyClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)    ibus_proxy_class_init,
        NULL,               /* class finalize */
        NULL,               /* class data */
        sizeof (IBusProxy),
        0,
        (GInstanceInitFunc) ibus_proxy_init,
    };

    if (type == 0) {
        type = g_type_register_static (IBUS_TYPE_OBJECT,
                    "IBusProxy",
                    &type_info,
                    (GTypeFlags)0);
    }
    return type;
}

IBusProxy *
ibus_proxy_new (const gchar     *name,
                const gchar     *path,
                IBusConnection  *connection)
{
    g_assert (name != NULL);
    g_assert (path != NULL);
    g_assert (IBUS_IS_CONNECTION (connection));

    IBusProxy *proxy;
    IBusProxyPrivate *priv;

    proxy = IBUS_PROXY (g_object_new (IBUS_TYPE_PROXY,
                            "name", name,
                            "path", path,
                            "connection", connection,
                            NULL));

    return proxy;
}

static void
ibus_proxy_class_init (IBusProxyClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);

    _parent_class = (IBusObjectClass *) g_type_class_peek_parent (klass);

    g_type_class_add_private (klass, sizeof (IBusProxyPrivate));
    
    gobject_class->constructor = ibus_proxy_constructor;
    gobject_class->set_property = (GObjectSetPropertyFunc) ibus_proxy_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) ibus_proxy_get_property;
    
    ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_proxy_destroy;

    klass->dbus_signal = ibus_proxy_dbus_signal;
    
    /* install properties */
    g_object_class_install_property (gobject_class,
                    PROP_NAME,
                    g_param_spec_string ("name",
                        "service name",
                        "The service name of proxy object",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
    g_object_class_install_property (gobject_class,
                    PROP_PATH,
                    g_param_spec_string ("path",
                        "object path",
                        "The path of proxy object",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    
    g_object_class_install_property (gobject_class,
                    PROP_CONNECTION,
                    g_param_spec_object ("connection",
                        "object path",
                        "The path of proxy object",
                        IBUS_TYPE_CONNECTION,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    /* install signals */
    _signals[DBUS_SIGNAL] =
        g_signal_new (I_("dbus-signal"),
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            G_STRUCT_OFFSET (IBusProxyClass, dbus_signal),
            NULL, NULL,
            ibus_marshal_BOOLEAN__POINTER,
            G_TYPE_BOOLEAN,
            1, G_TYPE_POINTER);

}

static GObject *
ibus_proxy_constructor (GType           type,
                        guint           n_construct_params,
                        GObjectConstructParam *construct_params)
{
    GObject *obj;

    g_debug ("%s", __FUNCTION__);
    obj = G_OBJECT_CLASS (_parent_class)->constructor (type, n_construct_params, construct_params);
    g_debug ("%s", __FUNCTION__);
    return obj;
}

static void
ibus_proxy_init (IBusProxy *proxy)
{
    g_debug ("%s", __FUNCTION__);
    IBusProxyPrivate *priv;
    priv = IBUS_PROXY_GET_PRIVATE (proxy);

    priv->name = NULL;
    priv->path = NULL;
    priv->connection = NULL;
}

static void
ibus_proxy_destroy (IBusProxy *proxy)
{
    IBusProxyPrivate *priv;
    priv = IBUS_PROXY_GET_PRIVATE (proxy);
    
    g_free (priv->name);
    g_free (priv->path);
    g_object_unref (priv->connection);
    
    priv->name = NULL;
    priv->path = NULL;
    priv->connection = NULL;
    
    IBUS_OBJECT_CLASS(_parent_class)->destroy (IBUS_OBJECT (proxy));
}

static void
ibus_proxy_set_property (IBusProxy      *proxy,
                         guint           prop_id,
                         const GValue   *value,
                         GParamSpec     *pspec)
{
    g_debug ("%s", __FUNCTION__);
    IBusProxyPrivate *priv;
    priv = IBUS_PROXY_GET_PRIVATE (proxy);
    
    switch (prop_id) {
    case PROP_NAME:
        g_assert (priv->name == NULL);
        priv->name = g_strdup (g_value_get_string (value));
        break;
    case PROP_PATH:
        g_assert (priv->path == NULL);
        priv->path = g_strdup (g_value_get_string (value));
        break;
    case PROP_CONNECTION:
        g_assert (priv->connection == NULL);
        priv->connection = IBUS_CONNECTION (g_value_get_object (value));
        g_object_ref (priv->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (proxy, prop_id, pspec);
    }
}

static void
ibus_proxy_get_property (IBusProxy      *proxy,
                         guint           prop_id,
                         GValue         *value,
                         GParamSpec     *pspec)
{
    IBusProxyPrivate *priv;
    priv = IBUS_PROXY_GET_PRIVATE (proxy);
    
    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string (value, priv->name);
        break;
    case PROP_PATH:
        g_value_set_string (value, priv->path);
        break;
    case PROP_CONNECTION:
        g_value_set_object (value, priv->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (proxy, prop_id, pspec);
    }
}


gboolean
ibus_proxy_handle_signal (IBusProxy     *proxy,
                          DBusMessage   *message)
{
    gboolean retval = FALSE;
    g_return_val_if_fail (message != NULL, FALSE);
    
    g_signal_emit (proxy, _signals[DBUS_SIGNAL], 0, message, &retval);
    return retval ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
ibus_proxy_dbus_signal (IBusProxy   *proxy,
                        DBusMessage *message)
{
    return FALSE;
}