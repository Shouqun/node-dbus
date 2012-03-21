#ifndef _DBUS_NODE_H
#define _DBUS_NODE_H

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <cstdlib>

v8::Handle<v8::Value> decode_reply_messages(DBusMessage *message);

bool encode_to_message_with_objects(v8::Local<v8::Value> value, 
                                           DBusMessageIter *iter, 
                                           char* signature);

#endif

