#include <v8.h>
#include <node.h>
#include <cstring>
#include <dbus/dbus.h>

#include "dbus.h"
#include "decoder.h"
#include "encoder.h"
#include "object_handler.h"

namespace ObjectHandler {

	using namespace node;
	using namespace v8;
	using namespace std;

	struct NodeDBus::NodeCallback *object_handler = NULL;

	// Initializing object handler table
	static DBusHandlerResult MessageHandler(DBusConnection *connection, DBusMessage *message, void *user_data)
	{
		// Getting V8 context
		Local<Context> context = Context::GetCurrent();
		Context::Scope ctxScope(context); 
		HandleScope scope;

		const char *sender = dbus_message_get_sender(message);
		const char *object_path = dbus_message_get_path(message);
		const char *interface = dbus_message_get_interface(message);
		const char *member = dbus_message_get_member(message);

		// Create a object template
		Handle<ObjectTemplate> object_template = ObjectTemplate::New();
		object_template->SetInternalFieldCount(2);
		Persistent<ObjectTemplate> object_instance = Persistent<ObjectTemplate>::New(object_template);

		// Store connection and message
		Handle<Object> message_object = object_instance->NewInstance();
		message_object->SetInternalField(0, External::New(connection));
		message_object->SetInternalField(1, External::New(message));

		// Getting arguments
		Handle<Value> arguments = Decoder::DecodeArguments(message);

		Handle<Value> args[] = {
			String::New(dbus_bus_get_unique_name(connection)),
			String::New(sender),
			String::New(object_path),
			String::New(interface),
			String::New(member),
			message_object,
			arguments
		};

		// Keep message live for reply
		dbus_message_ref(message);

		TryCatch try_catch;

		object_handler->cb->Call(object_handler->cb, 7, args);

		if (try_catch.HasCaught()) {
			printf("Ooops, Exception on call the callback\n%s\n", *String::Utf8Value(try_catch.StackTrace()->ToString()));
		}

		return DBUS_HANDLER_RESULT_HANDLED;
	}

	static void UnregisterMessageHandler(DBusConnection *connection, void *user_data)
	{
	}

	static inline DBusObjectPathVTable CreateVTable()
	{
		DBusObjectPathVTable vt;

		vt.message_function = MessageHandler;
		vt.unregister_function = UnregisterMessageHandler;

		return vt;
	}

	static void _SendMessageReply(DBusConnection *connection, DBusMessage *message, Local<Value> reply_value, char *signature)
	{
		HandleScope scope;
		DBusMessageIter iter;
		DBusMessage *reply;
		dbus_uint32_t serial = 0;

		reply = dbus_message_new_method_return(message);

		dbus_message_iter_init_append(reply, &iter);
		if (!Encoder::EncodeObject(reply_value, &iter, signature)) {
			printf("Failed to encode reply value\n");
		}

		// Send reply message
		dbus_connection_send(connection, reply, &serial);
		dbus_connection_flush(connection);

		dbus_message_unref(reply);
	}

	void SetHandler(Handle<Object> Holder, Handle<Function> callback)
	{
		if (object_handler == NULL) {
			object_handler = new NodeDBus::NodeCallback();
		} else {
			object_handler->Holder.Dispose();
			object_handler->cb.Dispose();
		}

		object_handler->Holder = Persistent<Object>::New(Holder);
		object_handler->cb = Persistent<Function>::New(callback);
	}

	DBusObjectPathVTable vtable = CreateVTable();

	Handle<Value> RegisterObjectPath(Arguments const &args)
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

		NodeDBus::BusObject *bus = (NodeDBus::BusObject *) External::Unwrap(args[0]->ToObject()->GetInternalField(0));

		// Register object path
		char *object_path = strdup(*String::Utf8Value(args[1]->ToString()));
		dbus_bool_t ret = dbus_connection_register_object_path(bus->connection,
			object_path,
			&vtable,
			NULL);
		delete object_path;
		if (!ret) {
			return ThrowException(Exception::Error(
				String::New("Out of Memory")
			));
		}

		return Undefined();
	}

	Handle<Value> SendMessageReply(Arguments const &args)
	{
		HandleScope scope;

		if (!args[0]->IsObject()) {
			return ThrowException(Exception::TypeError(
				String::New("first argument must be a object")
			));
		}

		DBusConnection *connection = (DBusConnection *) External::Unwrap(args[0]->ToObject()->GetInternalField(0));
		DBusMessage *message = (DBusMessage *) External::Unwrap(args[0]->ToObject()->GetInternalField(1));

		_SendMessageReply(connection, message, args[1], *String::Utf8Value(args[2]->ToString()));

		return Undefined();
	}
}

