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

static DBusHandlerResult
MessageHandler(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	HandleScope scope;
	Persistent<Function> *f(node::cb_unwrap(user_data));
	Handle<Value> args_value = Undefined();
	Local<Value> return_value;

	args_value = decode_reply_messages(message);

	int argc = 3;
	Local<Value> argv[3] = {
		Local<Value>::New(String::New(dbus_message_get_interface(message))),
		Local<Value>::New(String::New(dbus_message_get_member(message))),
		Local<Value>::New(args_value)
	};

	/* 
	 * NOTE: Using JavaScript to handle message is easier for development.
	 * If it has performance concern, we can integrate it into C++ code.
	 */
	return_value = (*f)->Call(*f, argc, argv);

	SendMessageReply(connection, message, return_value);

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
