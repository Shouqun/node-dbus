#include <v8.h>
#include <node.h>
#include <cstring>
#include <dbus/dbus.h>
#include <nan.h>

#include "dbus.h"
#include "signal.h"
#include "encoder.h"

namespace Signal {

	using namespace node;
	using namespace v8;
	using namespace std;

	//Persistent<Function> handler = Persistent<Function>::New(Handle<Function>::Cast(NanNull()));
	bool hookSignal = false;
	Persistent<Function> handler;

	void DispatchSignal(Handle<Value> args[])
	{
		NanScope();

		if (!hookSignal)
			return;

//		MakeCallback(handler, handler, 6, args);
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(handler), 6, args);
	}

	NAN_METHOD(SetSignalHandler) {
		NanScope();

//		handler.Dispose();
//		handler.Clear();

		hookSignal = true;
		NanAssignPersistent(handler, args[0].As<Function>());
//		handler = Persistent<Function>::New(Handle<Function>::Cast(args[0]));

		NanReturnUndefined();
	}

	NAN_METHOD(EmitSignal) {
		NanScope();

		if (!args[0]->IsObject()) {
			return NanThrowTypeError("First parameter must be an object");
		}

		// Object path
		if (!args[1]->IsString()) {
			return NanThrowTypeError("Require object path");
		}

		// Interface name
		if (!args[2]->IsString()) {
			return NanThrowTypeError("Require interface");
		}

		// Signal name
		if (!args[3]->IsString()) {
			return NanThrowTypeError("Require signal name");
		}

		// Arguments
		if (!args[4]->IsArray()) {
			return NanThrowTypeError("Require arguments");
		}

		// Signatures
		if (!args[5]->IsArray()) {
			return NanThrowTypeError("Require signature");
		}

		NodeDBus::BusObject *bus = static_cast<NodeDBus::BusObject *>(NanGetInternalFieldPointer(args[0]->ToObject(), 0));
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

		NanReturnUndefined();
	}
}

