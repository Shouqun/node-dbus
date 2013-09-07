#include <v8.h>
#include <node.h>
#include <string>
#include <dbus/dbus.h>

#include "dbus.h"
#include "signal.h"

namespace Signal {

	using namespace node;
	using namespace v8;
	using namespace std;

	struct NodeDBus::NodeCallback *signal_handler = NULL;

	void EmitSignal(Handle<Value> args[])
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
}

