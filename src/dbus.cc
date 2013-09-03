#include <v8.h>
#include <node.h>
#include <uv.h>
#include <string>
#include <dbus/dbus.h>

#include "dbus.h"

namespace NodeDBus {
 
	using namespace node;
	using namespace v8;
	using namespace std;

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

		return scope.Close(bus_object);
	}

	Handle<Value> CallMethod(const Arguments& args)
	{
		HandleScope scope;

		Local<Object> bus_object = args[0]->ToObject();
		String::Utf8Value service_name(args[1]->ToString());
		String::Utf8Value object_path(args[2]->ToString());
		String::Utf8Value interface_name(args[3]->ToString());
		String::Utf8Value method(args[4]->ToString());
		String::Utf8Value signature(args[5]->ToString());

		// Get bus from internal field
		BusObject *bus = (DBusGConnection*) External::Unwrap(bus_object->GetInternalField(0));

		// Create message for method call
		DBusMessage *message = dbus_message_new_method_call(*service_name, *object_path, *interface_name, method);

		// Preparing method arguments
		if (args[6]->IsObject()) {
			DBusMessageIter iter;
			DBusSignatureIter siter;
			DBusError error;

			Local<Object> argumentObject = args[6]->ToObject();

			// Initializing augument message
			dbus_message_iter_init_append(message, &iter); 

			dbus_error_init(&error);

			// Initializing signature
			if (!dbus_signature_validate(signature, &error)) {
				ERROR("Invalid signature: %s\n", error.message);
			}

			dbus_signature_iter_init(&siter, signature);
			// TODO: encode arguments to message
		}

		// TODO: Process message returned
	}

	static void init(Handle<Object> target) {
		HandleScope scope;

		NODE_SET_METHOD(target, "getBus", GetBus);
		NODE_SET_METHOD(target, "callMethod", CallMethod);
	}

	NODE_MODULE(dbus, init);
}
