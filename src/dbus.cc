#include <v8.h>
#include <node.h>
#include <uv.h>
#include <cstring>
#include <dbus/dbus.h>

#include "dbus.h"
#include "callback.h"
#include "connection.h"
#include "signal.h"
#include "decoder.h"
#include "encoder.h"
#include "introspect.h"
#include "object_handler.h"

namespace NodeDBus {

	static void method_callback(DBusPendingCall *pending, void *user_data)
	{
		DBusError error;
		DBusMessage *reply_message;
		DBusAsyncData *data = static_cast<DBusAsyncData *>(user_data);

		dbus_error_init(&error);

		// Getting reply message
		reply_message = dbus_pending_call_steal_reply(pending);
		if (!reply_message) {
			dbus_pending_call_unref(pending);
			return;
		}

		// Get current context from V8
		Local<Context> context = Context::GetCurrent();
		Context::Scope ctxScope(context);
		HandleScope scope;

		// Decode message for arguments
		Handle<Value> result = Decoder::DecodeMessage(reply_message);

		// Release
		dbus_message_unref(reply_message);
		dbus_pending_call_unref(pending);

		// Preparing arguments for callback
		Callback::CallbackData *callback_data = new Callback::CallbackData();
		callback_data->callback = data->callback;
		callback_data->argc = 1;
		callback_data->argv = Persistent<Object>::New(Object::New());
		callback_data->argv->Set(0, result);

		// Invoke callback on the next tick
		Callback::Invoke(callback_data);
	}

	static void method_free(void *user_data)
	{
		DBusAsyncData *data = static_cast<DBusAsyncData *>(user_data);

		data->callback.Clear();
		data->pending = NULL;
		delete data;
	}

	Handle<Value> GetBus(const Arguments& args)
	{
		HandleScope scope;
		DBusConnection *connection = NULL;
		DBusError error;

		dbus_error_init(&error);

		if (!args[0]->IsNumber())
			return ThrowException(Exception::Error(String::New("First parameter is integer")));

		// Create connection
		switch(args[0]->IntegerValue()) {
		case NODE_DBUS_BUS_SYSTEM:
			connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
			break;

		case NODE_DBUS_BUS_SESSION:
			connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
			break;
		}

		if (connection == NULL)
			return ThrowException(Exception::Error(String::New("Failed to get bus object")));

		// Initializing connection object
		Handle<ObjectTemplate> object_template = ObjectTemplate::New();
		object_template->SetInternalFieldCount(1);
		Persistent<ObjectTemplate> object_instance = Persistent<ObjectTemplate>::New(object_template);

		// Create bus object
		BusObject *bus = new BusObject;
		bus->type = (BusType)args[0]->IntegerValue();
		bus->connection = connection;

		// Create a JavaScript object to store bus object
		Local<Object> bus_object = object_instance->NewInstance();
		bus_object->SetInternalField(0, External::New(bus));
		bus_object->Set(String::NewSymbol("uniqueName"), String::New(dbus_bus_get_unique_name(connection)));

		// Initializing connection handler
		Connection::Init(bus);

		return scope.Close(bus_object);
	}

	Handle<Value> CallMethod(const Arguments& args)
	{
		HandleScope scope;
		DBusError error;

		Local<Object> bus_object = args[0]->ToObject();
		char *service_name = strdup(*String::Utf8Value(args[1]->ToString()));
		char *object_path = strdup(*String::Utf8Value(args[2]->ToString()));
		char *interface_name = strdup(*String::Utf8Value(args[3]->ToString()));
		char *method = strdup(*String::Utf8Value(args[4]->ToString()));
		Local<Function> callback = Local<Function>::Cast(args[8]);
		int timeout = -1;

		if (args[6]->IsInt32())
			timeout = args[6]->Int32Value();

		// Get bus from internal field
		BusObject *bus = (BusObject *) External::Unwrap(bus_object->GetInternalField(0));

		// Initializing error handler
		dbus_error_init(&error);

		// Create message for method call
		DBusMessage *message = dbus_message_new_method_call(service_name, object_path, interface_name, method);

		delete service_name;
		delete object_path;
		delete interface_name;
		delete method;

		// Preparing method arguments
		if (args[7]->IsObject()) {
			DBusMessageIter iter;
			DBusSignatureIter siter;

			Local<Array> argument_arr = Local<Array>::Cast(args[7]);
			if (argument_arr->Length() > 0) {

				// Initializing augument message
				dbus_message_iter_init_append(message, &iter); 

				// Initializing signature
				char *sig = strdup(*String::Utf8Value(args[5]->ToString()));
				if (!dbus_signature_validate(sig, &error)) {
					return ThrowException(Exception::Error(String::New(error.message)));
				}

				// Getting all signatures
				dbus_signature_iter_init(&siter, sig);
				for (unsigned int i = 0; i < argument_arr->Length(); ++i) {
					char *arg_sig = dbus_signature_iter_get_signature(&siter);
					Local<Value> arg = argument_arr->Get(i);

					if (!Encoder::EncodeObject(arg, &iter, arg_sig)) {
						dbus_free(arg_sig);
						break;
					}

					dbus_free(arg_sig);

					if (!dbus_signature_iter_next(&siter))
						break;
				}

				dbus_free(sig);
			}
		}

		// Send message and call method
		DBusPendingCall *pending;
		if (!dbus_connection_send_with_reply(bus->connection, message, &pending, timeout)) {
			if (message != NULL)
				dbus_message_unref(message);

			return ThrowException(Exception::Error(String::New("Failed to call method: Out of Memory")));
		}

		// Set callback for waiting
		DBusAsyncData *data = new DBusAsyncData;
		data->pending = pending;
		data->callback = Persistent<Function>::New(callback);
		if (!dbus_pending_call_set_notify(pending, method_callback, data, method_free)) {
			if (message != NULL)
				dbus_message_unref(message);

			return ThrowException(Exception::Error(String::New("Failed to call method: Out of Memory")));
		}

		if (message != NULL)
			dbus_message_unref(message);

		return Undefined();
	}

	Handle<Value> RequestName(Arguments const &args)
	{
		HandleScope scope;

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

		BusObject *bus = (BusObject *) External::Unwrap(args[0]->ToObject()->GetInternalField(0));
		char *service_name = strdup(*String::Utf8Value(args[1]->ToString()));

		// Request bus name
		dbus_bus_request_name(bus->connection, service_name, 0, NULL);

		dbus_free(service_name);

		return Undefined();
	}

	Handle<Value> ParseIntrospectSource(const Arguments& args)
	{
		HandleScope scope;

		if (!args[0]->IsString())
			return Null();

		String::Utf8Value source(args[0]->ToString());
		char *src = strdup(*source);

		Handle<Value> obj = Introspect::CreateObject(src);

		delete src;

		return scope.Close(obj);
	}

	Handle<Value> SetSignalHandler(const Arguments& args)
	{
		HandleScope scope;

		Signal::SetHandler(args.Holder(), Handle<Function>::Cast(args[0]));

		return Undefined();
	}

	Handle<Value> AddSignalFilter(const Arguments& args)
	{
		HandleScope scope;
		DBusError error;

		Local<Object> bus_object = args[0]->ToObject();
		String::Utf8Value rule(args[1]->ToString());
		char *rule_str = strdup(*rule);

		BusObject *bus = (BusObject *) External::Unwrap(bus_object->GetInternalField(0));

		dbus_error_init(&error);
		dbus_bus_add_match(bus->connection, rule_str, &error);
		dbus_connection_flush(bus->connection);

		delete rule_str;

		if (dbus_error_is_set(&error)) {
			printf("Failed to add rule: %s\n", error.message);
		}

		return Undefined();
	}

	Handle<Value> SetObjectHandler(const Arguments& args)
	{
		HandleScope scope;

		if (!args[0]->IsFunction()) {
			return ThrowException(Exception::TypeError(
				String::New("first argument must be a function")
			));
		}

		ObjectHandler::SetHandler(args.Holder(), Handle<Function>::Cast(args[0]));

		return Undefined();
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "getBus", GetBus);
		NODE_SET_METHOD(target, "callMethod", CallMethod);
		NODE_SET_METHOD(target, "requestName", RequestName);
		NODE_SET_METHOD(target, "registerObjectPath", ObjectHandler::RegisterObjectPath);
		NODE_SET_METHOD(target, "sendMessageReply", ObjectHandler::SendMessageReply);
		NODE_SET_METHOD(target, "setObjectHandler", SetObjectHandler);
		NODE_SET_METHOD(target, "parseIntrospectSource", ParseIntrospectSource);
		NODE_SET_METHOD(target, "setSignalHandler", SetSignalHandler);
		NODE_SET_METHOD(target, "addSignalFilter", AddSignalFilter);
		NODE_SET_METHOD(target, "emitSignal", Signal::EmitSignal);
	}

	NODE_MODULE(dbus, init);
}
