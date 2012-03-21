#ifndef _DBUS_REGISTER_H
#define _DBUS_REGISTER_H

#include <dbus/dbus.h>
#include <v8.h>
#include <node.h>

bool EncodeReplyValue(v8::Local<v8::Value> value, DBusMessageIter *iter);

static DBusHandlerResult MessageHandler(DBusConnection *connection, DBusMessage *message, void *user_data);
static void UnregisterMessageHandler(DBusConnection *connection, void *user_data);
static inline DBusObjectPathVTable CreateVTable();

v8::Handle<v8::Value> RequestName(v8::Arguments const &args);
v8::Handle<v8::Value> RegisterObjectPath(v8::Arguments const &args);

#endif

