#include <v8.h>
#include <node.h>
#include <cstring>
#include <dbus/dbus.h>

#include "dbus.h"
#include "signal.h"
#include "encoder.h"

namespace Signal {

	using namespace node;
	using namespace v8;
	using namespace std;

	Persistent<Function> handler = Persistent<Function>::New(Handle<Function>::Cast(Null()));

	void DispatchSignal(Handle<Value> args[])
	{
		HandleScope scope;

		if (handler->IsNull())
			return;

		MakeCallback(handler, handler, 6, args);
	}

	Handle<Value> SetSignalHandler(const Arguments& args)
	{
		HandleScope scope;

		handler.Dispose();
		handler.Clear();

		handler = Persistent<Function>::New(Handle<Function>::Cast(args[0]));

		return Undefined();
	}

	Handle<Value> EmitSignal(const Arguments& args)
	{
		HandleScope scope;

		if (!args[0]->IsObject()) {
			return ThrowException(Exception::TypeError(
				String::New("first argument must be a object")
			));
		}

		// Object path
		if (!args[1]->IsString()) {
			return ThrowException(Exception::TypeError(
				String::New("Require object path")
			));
		}

		// Interface name
		if (!args[2]->IsString()) {
			return ThrowException(Exception::TypeError(
				String::New("Require interface")
			));
		}

		// Signal name
		if (!args[3]->IsString()) {
			return ThrowException(Exception::TypeError(
				String::New("Require signal name")
			));
		}

		// Arguments
		if (!args[4]->IsArray()) {
			return ThrowException(Exception::TypeError(
				String::New("Require arguments")
			));
		}

		// Signatures
		if (!args[5]->IsArray()) {
			return ThrowException(Exception::TypeError(
				String::New("Require signature")
			));
		}

		NodeDBus::BusObject *bus = static_cast<NodeDBus::BusObject *>(External::Unwrap(args[0]->ToObject()->GetInternalField(0)));
		DBusMessage *message;
		DBusMessageIter iter;

		// Create a signal
		char *path = strdup(*String::Utf8Value(args[1]->ToString()));
		char *interface = strdup(*String::Utf8Value(args[2]->ToString()));
		char *signal = strdup(*String::Utf8Value(args[3]->ToString()));
		message = dbus_message_new_signal(path, interface, signal);

		// Preparing message
		dbus_message_iter_init_append(message, &iter);

		// Preparing arguments
		Local<Array> arguments = Local<Array>::Cast(args[4]);
		Local<Array> signatures = Local<Array>::Cast(args[5]);
		for (unsigned int i = 0; i < arguments->Length(); ++i) {
			Local<Value> arg = arguments->Get(i);

			char *sig = strdup(*String::Utf8Value(signatures->Get(i)->ToString()));

			if (!Encoder::EncodeObject(arg, &iter, sig)) {
				dbus_free(sig);
				printf("Failed to encode arguments of signal\n");
				break;
			}

			dbus_free(sig);
		}

		// Send out message
		dbus_connection_send(bus->connection, message, NULL);
		dbus_connection_flush(bus->connection);
		dbus_message_unref(message);

		dbus_free(path);
		dbus_free(interface);
		dbus_free(signal);

		return Undefined();
	}
}

