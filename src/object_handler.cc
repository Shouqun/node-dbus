#include <v8.h>
#include <node.h>
#include <cstring>
#include <dbus/dbus.h>
#include <nan.h>

#include "dbus.h"
#include "decoder.h"
#include "encoder.h"
#include "object_handler.h"

namespace ObjectHandler {

	using namespace node;
	using namespace v8;
	using namespace std;

	Persistent<Function> handler;

	// Initializing object handler table
	static DBusHandlerResult MessageHandler(DBusConnection *connection, DBusMessage *message, void *user_data)
	{
		NanScope();

		const char *sender = dbus_message_get_sender(message);
		const char *object_path = dbus_message_get_path(message);
		const char *interface = dbus_message_get_interface(message);
		const char *member = dbus_message_get_member(message);

		// Create a object template
		Local<ObjectTemplate> object_template = NanNew<ObjectTemplate>();
		object_template->SetInternalFieldCount(2);

		// Store connection and message
		Local<Object> message_object = object_template->NewInstance();
		NanSetInternalFieldPointer(message_object, 0, connection);
		NanSetInternalFieldPointer(message_object, 1, message);

		// Getting arguments
		Handle<Value> arguments = Decoder::DecodeArguments(message);

		Handle<Value> args[] = {
			NanNew<String>(dbus_bus_get_unique_name(connection)),
			NanNew<String>(sender),
			NanNew<String>(object_path),
			NanNew<String>(interface),
			NanNew<String>(member),
			message_object,
			arguments
		};

		// Keep message live for reply
		if (!dbus_message_get_no_reply(message))
			dbus_message_ref(message);

		// Invoke
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(handler), 7, args);

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
		NanScope();
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

	DBusObjectPathVTable vtable = CreateVTable();

	NAN_METHOD(RegisterObjectPath) {
		NanScope();

		if (!args[0]->IsObject()) {
			return NanThrowTypeError("First parameter must be an object (bus)");
		}

		if (!args[1]->IsString()) {
			return NanThrowTypeError("Second parameter must be a string (object path)");
		}

		NodeDBus::BusObject *bus = static_cast<NodeDBus::BusObject *>(NanGetInternalFieldPointer(args[0]->ToObject(), 0));

		// Register object path
		char *object_path = strdup(*NanUtf8String(args[1]));
		dbus_bool_t ret = dbus_connection_register_object_path(bus->connection,
			object_path,
			&vtable,
			NULL);
		dbus_connection_flush(bus->connection);
		dbus_free(object_path);
		if (!ret) {
			return NanThrowError("Out of Memory");
		}

		NanReturnUndefined();
	}

	NAN_METHOD(SendMessageReply) {
		NanScope();

		if (!args[0]->IsObject()) {
			return NanThrowTypeError("First parameter must be an object");
		}

		DBusConnection *connection = static_cast<DBusConnection *>(NanGetInternalFieldPointer(args[0]->ToObject(), 0));
		DBusMessage *message = static_cast<DBusMessage *>(NanGetInternalFieldPointer(args[0]->ToObject(), 1));

		if (dbus_message_get_no_reply(message)) {
			NanReturnUndefined();
		}

		char *signature = strdup(*NanUtf8String(args[2]));
		_SendMessageReply(connection, message, args[1], signature);
		dbus_free(signature);

		NanReturnUndefined();
	}

	NAN_METHOD(SendErrorMessageReply) {
		NanScope();

		if (!args[0]->IsObject()) {
			return NanThrowTypeError("First parameter must be an object");
		}

		DBusConnection *connection = static_cast<DBusConnection *>(NanGetInternalFieldPointer(args[0]->ToObject(), 0));
		DBusMessage *message = static_cast<DBusMessage *>(NanGetInternalFieldPointer(args[0]->ToObject(), 1));

		// Getting error message from arguments
		char *name = strdup(*NanUtf8String(args[1]));
		char *msg = strdup(*NanUtf8String(args[2]));

		// Initializing error message
		DBusMessage *error_message = dbus_message_new_error(message, name, msg);

		// Send error message
		dbus_connection_send(connection, error_message, NULL);
		dbus_connection_flush(connection);

		// Release
		dbus_message_unref(error_message);
		dbus_free(name);
		dbus_free(msg);

		NanReturnUndefined();
	}

	NAN_METHOD(SetObjectHandler) {
		NanScope();

		if (!args[0]->IsFunction()) {
			return NanThrowTypeError("First parameter must be a function");
		}

		NanDisposePersistent(handler);
		NanAssignPersistent(handler, args[0].As<Function>());

		NanReturnUndefined();
	}
}

