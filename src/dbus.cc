#include <v8.h>
#include <node.h>
#include <uv.h>
#include <string>
#include <dbus/dbus.h>

#include "dbus.h"
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
		DBusAsyncData *data = (DBusAsyncData*)user_data;

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
		TryCatch try_catch;

		// Decode message for arguments
		Handle<Value> result = Decoder::DecodeMessage(reply_message);
		Handle<Value> args[] = {
			result
		};

		// Call
		Handle<Object> holder = data->callback->Holder;
		Local<Function> callback = Local<Function>::New(data->callback->cb);
		callback->Call(holder, 1, args);

		// Release
		dbus_message_unref(reply_message);
		dbus_pending_call_unref(pending);
		
	}

	static void method_free(void *user_data)
	{
		DBusAsyncData *data = (DBusAsyncData *)user_data;
		
		data->callback->Holder.Dispose();
		data->callback->cb.Dispose();

		delete data->callback;
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
		String::Utf8Value service_name(args[1]->ToString());
		String::Utf8Value object_path(args[2]->ToString());
		String::Utf8Value interface_name(args[3]->ToString());
		String::Utf8Value method(args[4]->ToString());
		String::Utf8Value signature(args[5]->ToString());
		Local<Function> callback = Local<Function>::Cast(args[8]);
		int timeout = -1;

		if (args[6]->IsInt32())
			timeout = args[6]->Int32Value();

		// Get bus from internal field
		BusObject *bus = (BusObject *) External::Unwrap(bus_object->GetInternalField(0));

		// Initializing error handler
		dbus_error_init(&error);

		// Create message for method call
		DBusMessage *message = dbus_message_new_method_call(*service_name, *object_path, *interface_name, *method);

		// Preparing method arguments
		if (args[7]->IsObject()) {
			DBusMessageIter iter;
			DBusSignatureIter siter;

			Local<Array> argument_arr = Local<Array>::Cast(args[7]);

			// Initializing augument message
			dbus_message_iter_init_append(message, &iter); 

			// Initializing signature
			if (!dbus_signature_validate(*signature, &error)) {
				printf("Invalid signature: %s\n", error.message);
			}

			// Getting all signatures
			dbus_signature_iter_init(&siter, *signature);
			for (unsigned int i = 0; i < argument_arr->Length(); ++i) {
				char *arg_sig = dbus_signature_iter_get_signature(&siter);
				Local<Value> arg = argument_arr->Get(i);

				if (!Encoder::EncodeObject(arg, &iter, arg_sig)) {
					dbus_free(arg_sig);
					break;
				}

				dbus_free(arg_sig);
				dbus_signature_iter_next(&siter);
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
		data->callback = new NodeCallback();
		data->callback->Holder = Persistent<Object>::New(args.Holder());
		data->callback->cb = Persistent<Function>::New(callback);
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

		return Undefined();
	}

	Handle<Value> ParseIntrospectSource(const Arguments& args)
	{
		HandleScope scope;

		String::Utf8Value source(args[0]->ToString());

		return scope.Close(Introspect::CreateObject(*source));
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

		if (dbus_error_is_set(&error)) {
			printf("Failed to add rule: %s\n", error.message);
		}

		return Undefined();
	}

	Handle<Value> SetObjectHandler(const Arguments& args)
	{
		HandleScope scope;

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
