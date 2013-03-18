#include <v8.h>
#include <node.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string>
#include <list>
#include <map>
#include <iostream>

#include "common.h"
#include "dbus.h"
#include "dbus_introspect.h"
#include "dbus_register.h"

using namespace v8;
using namespace dbus_library;

DBusObjectPathVTable vtable = CreateVTable();
char *currentName = NULL;

bool
AppendDictValue(DBusMessageIter *iter, Local<Value> value)
{
	DBusMessageIter value_iter;
	const char *signature;

	if (value->IsBoolean()) {
		signature = DBUS_TYPE_BOOLEAN_AS_STRING;
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, signature, &value_iter);

	} else if (value->IsString()) {
		signature = DBUS_TYPE_STRING_AS_STRING;
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, signature, &value_iter);

	} else if (value->IsInt32()) {
		signature = DBUS_TYPE_INT32_AS_STRING;
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, signature, &value_iter);

	} else if (value->IsUint32()) {
		signature = DBUS_TYPE_UINT32_AS_STRING;
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, signature, &value_iter);

	} else if (value->IsNumber()) {
		signature = DBUS_TYPE_DOUBLE_AS_STRING;
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, signature, &value_iter);

	} else if (value->IsArray()) {
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
			DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_VARIANT_AS_STRING,
			&value_iter);

	} else if (value->IsObject()) {
		dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
			DBUS_TYPE_ARRAY_AS_STRING
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
			&value_iter);
	}

	EncodeReplyValue(value, &value_iter);

	dbus_message_iter_close_container(iter, &value_iter); 

	return TRUE;
}

bool
AppendDictEntry(DBusMessageIter *iter, Local<Value> key, Local<Value> value)
{

	DBusMessageIter dict_iter;

	/* Get key */
	String::Utf8Value key_name_val(key->ToString());
	char *key_str = *key_name_val;

	dbus_message_iter_open_container(iter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter);

	/* Key */
	dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &key_str);

	/* Value */
	AppendDictValue(&dict_iter, value);

	dbus_message_iter_close_container(iter, &dict_iter); 

	return TRUE;
}

bool
EncodeReplyValue(Local<Value> value, DBusMessageIter *iter)
{
	if (value->IsBoolean()) {
		dbus_bool_t data = value->BooleanValue();  

		if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &data)) {
			ERROR("Error append boolean\n");
			return false;
		}

	} else if (value->IsString()) {
		String::Utf8Value data_val(value->ToString());
		char *data = *data_val;

		if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &data)) {
			ERROR("Error append string\n");
			return false;
		}

	} else if (value->IsInt32()) {
		dbus_uint64_t data = value->IntegerValue();

		if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &data)) {
			ERROR("Error append numeric\n");
			return false;
		}

	} else if (value->IsUint32()) {
		dbus_uint64_t data = value->IntegerValue();

		if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &data)) {
			ERROR("Error append numeric\n");
			return false;
		}

	} else if (value->IsNumber()) {
		double data = value->NumberValue();

		if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &data)) {
			ERROR("Error append double\n");
			return false;
		}

	} else if (value->IsArray()) {
        DBusMessageIter subIter;
      
		/* Create a new container */
		dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, DBUS_TYPE_VARIANT_AS_STRING, &subIter);
        
        Local<Array> arrayData = Local<Array>::Cast(value);
		for (unsigned int i = 0; i < arrayData->Length(); ++i) {
			Local<Value> arrayItem = arrayData->Get(i);
			AppendDictValue(&subIter, arrayItem);
		}

        dbus_message_iter_close_container(iter, &subIter);

	} else if (value->IsObject()) {
        Local<Object> value_object = value->ToObject();
        DBusMessageIter subIter;

		/* Create a new container */
		dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
			DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
			DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
			DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
			&subIter);
        
        Local<Array> prop_names = value_object->GetOwnPropertyNames();
        int len = prop_names->Length();

		for (int i = 0; i < len; ++i) {
			Local<Value> prop_name = prop_names->Get(i);
			Local<Value> valueItem = value_object->Get(prop_name);

			AppendDictEntry(&subIter, prop_name, valueItem);
		}

		dbus_message_iter_close_container(iter, &subIter);
	}

	return true;
}

static void
SendMessageReply(DBusConnection *connection, DBusMessage *message, Local<Value> reply_value)
{
	DBusMessageIter iter;
	DBusMessage *reply;
	dbus_uint32_t serial = 0;

	reply = dbus_message_new_method_return(message);

	dbus_message_iter_init_append(reply, &iter);

	if (!EncodeReplyValue(reply_value, &iter)) {
		ERROR("Failed to encode reply value\n");
	}

	/* Send the reply */
	dbus_connection_send(connection, reply, &serial);
	dbus_connection_flush(connection);

	/* Release */
	dbus_message_unref(reply);
}

Handle<Value>
_SendMessageReply(Arguments const &args)
{
	Persistent<ObjectTemplate> message_template;
	DBusConnection *connection;
	DBusMessage *message;

	if (!args[0]->IsObject()) {
		return ThrowException(Exception::TypeError(
			String::New("first argument must be a object (message object)")
		));
	}

	connection = (DBusConnection*) External::Unwrap(args[0]->ToObject()->GetInternalField(0));
	message = (DBusMessage*) External::Unwrap(args[0]->ToObject()->GetInternalField(1));

	SendMessageReply(connection, message, args[1]);

	dbus_message_unref(message);

	return Undefined();
}

static DBusHandlerResult
MessageHandler(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	HandleScope scope;
	Persistent<Function> *f(node::cb_unwrap(user_data));
	Handle<Value> args_value = Undefined();
	Local<Value> return_value;

	args_value = decode_reply_messages(message);

	Handle<ObjectTemplate> object_template = ObjectTemplate::New();
	object_template->SetInternalFieldCount(2);
	Persistent<ObjectTemplate> message_template = Persistent<ObjectTemplate>::New(object_template);

	/* package connection and message */
	Local<Object> message_object = message_template->NewInstance();
	message_object->SetInternalField(0, External::New(connection));
	message_object->SetInternalField(1, External::New(message));

	/* 
	 * NOTE: Using JavaScript to handle message is easier for development.
	 * If it has performance concern, we can integrate it into C++ code.
	 */

	int argc = 4;
	Local<Value> argv[4] = {
		Local<Value>::New(String::New(dbus_message_get_interface(message))),
		Local<Value>::New(String::New(dbus_message_get_member(message))),
		message_object,
		Local<Value>::New(args_value)
	};

	return_value = (*f)->Call(*f, argc, argv);

	if (!return_value->IsNull()) {
		message_template.Dispose();
		SendMessageReply(connection, message, return_value);
	} else {
		dbus_message_ref(message);
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static void
UnregisterMessageHandler(DBusConnection *connection, void *user_data)
{
	node::cb_destroy(node::cb_unwrap(user_data));
}

static inline DBusObjectPathVTable
CreateVTable()
{
	DBusObjectPathVTable vt;

	vt.message_function = MessageHandler;
	vt.unregister_function = UnregisterMessageHandler;

	return vt;
}

Handle<Value>
RequestName(Arguments const &args)
{
	DBusGConnection *connection;

	if (!args[0]->IsObject()) {
		return ThrowException(Exception::TypeError(
			String::New("first argument must be a object (bus)")
		));
	}

	if (!args[1]->IsString()) {
		return ThrowException(Exception::TypeError(
			String::New("first argument must be a string (Bus Name)")
		));
	}

	connection = (DBusGConnection*) External::Unwrap(args[0]->ToObject()->GetInternalField(0));

	/* Name */
	free(currentName);
	currentName = g_strdup(*String::Utf8Value(args[1]->ToString()));

	/* Request bus name */
	dbus_bus_request_name(dbus_g_connection_get_connection(connection),
		currentName, 0, NULL);
//		*String::Utf8Value(args[1]->ToString()), 0, NULL);
	/* TODO: accept flags */
	/* TODO: check last parameter for error */

	return Undefined();
}

Handle<Value>
RegisterObjectPath(Arguments const &args)
{
	HandleScope scope;

	if (!args[0]->IsObject()) {
		return ThrowException(Exception::TypeError(
			String::New("first argument must be a object (bus)")
		));
	}

	if (!args[1]->IsString()) {
		return ThrowException(Exception::TypeError(
			String::New("second argument must be a string (object path)")
		));
	}

	if (!args[2]->IsFunction()) {
		return ThrowException(Exception::TypeError(
			String::New("third argument must be a function (message handler)")
		));
	}

	DBusGConnection *connection = (DBusGConnection*) External::Unwrap(args[0]->ToObject()->GetInternalField(0));

	/* Register message handler for object */
	dbus_bool_t ret = dbus_connection_register_object_path(dbus_g_connection_get_connection(connection),
		*String::Utf8Value(args[1]->ToString()),
		&vtable,
		node::cb_persist(args[2]));
	if (!ret)
		return ThrowException(Exception::Error(
			String::New("Out of Memory")
		));

	return Undefined();
}

Handle<Value> EmitSignal(Arguments const &args)
{
  HandleScope scope;

  if (args.Length() < 4) {
    return ThrowException(Exception::RangeError(
        String::New("Arguments not enough for emitSignal: "
                    "  (bus_object, path, interface, signal [,arguments]")));
  }

  if (!args[0]->IsObject()) {
    return ThrowException(Exception::TypeError(
        String::New("first argument MUST be an object(bus)")));
  }

  if (!args[1]->IsString()) {
    return ThrowException(Exception::TypeError(
        String::New("second argument MUST be an string(object path)")));
  }

  if (!args[2]->IsString()) {
    return ThrowException(Exception::TypeError(
        String::New("third argument MUST be an string(interface)")));
  }

  if (!args[3]->IsString()) {
    return ThrowException(Exception::TypeError(
        String::New("third argument MUST be an string(signal)")));
  }


  DBusGConnection *connection = (DBusGConnection*)External::Unwrap(
                                  args[0]->ToObject()->GetInternalField(0));

  if (!connection) {
    return ThrowException(Exception::TypeError(
        String::New("Not a bus object")));
  }

  String::Utf8Value path(args[1]->ToString());
  String::Utf8Value interface(args[2]->ToString());
  String::Utf8Value signal(args[3]->ToString());

  DBusMessage *signalMsg;
	DBusMessageIter iter;

  signalMsg = dbus_message_new_signal(*path, *interface, *signal);

  //check if signal has arguments
  if (args.Length() == 5) {
    //prepare signal arguments
    dbus_message_iter_init_append(signalMsg, &iter);

    if (!EncodeReplyValue(args[4], &iter)) {
      ERROR("Failed to encode signal arguments\n");
    }
  }

  //send the signal
  dbus_connection_send(dbus_g_connection_get_connection(connection),
                       signalMsg, NULL);

  dbus_connection_flush(dbus_g_connection_get_connection(connection));

  dbus_message_unref(signalMsg);

  return Undefined();
}

