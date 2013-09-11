#include <v8.h>
#include <node.h>
#include <string>
#include <dbus/dbus.h>

#include "dbus.h"
#include "signal.h"
#include "encoder.h"

namespace Signal {

	using namespace node;
	using namespace v8;
	using namespace std;

	struct NodeDBus::NodeCallback *signal_handler = NULL;

	void DispatchSignal(Handle<Value> args[])
	{
		if (!signal_handler)
			return;

		TryCatch try_catch;

		signal_handler->cb->Call(signal_handler->Holder, 6, args);

		if (try_catch.HasCaught()) {
			printf("Ooops, Exception on call the callback\n%s\n", *String::Utf8Value(try_catch.StackTrace()->ToString()));
		}
	}

	void SetHandler(Handle<Object> Holder, Handle<Function> callback)
	{
		if (signal_handler == NULL) {
			signal_handler = new NodeDBus::NodeCallback();
		} else {
			signal_handler->Holder.Dispose();
			signal_handler->cb.Dispose();
		}

		signal_handler->Holder = Persistent<Object>::New(Holder);
		signal_handler->cb = Persistent<Function>::New(callback);
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

		NodeDBus::BusObject *bus = (NodeDBus::BusObject *) External::Unwrap(args[0]->ToObject()->GetInternalField(0));
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
			String::Utf8Value sig(signatures->Get(i)->ToString());
			if (!Encoder::EncodeObject(arg, &iter, *sig)) {
				printf("Failed to encode arguments of signal\n");
				break;
			}
		}

		// Send out message
		dbus_connection_send(bus->connection, message, NULL);
		dbus_connection_flush(bus->connection);
		dbus_message_unref(message);

		delete path;
		delete interface;
		delete signal;

		return Undefined();
	}
}

