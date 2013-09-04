#include <v8.h>
#include <node.h>
#include <string>
#include <dbus/dbus.h>

#include "decoder.h"

namespace Decoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	Handle<Value> DecodeMessage(DBusMessage *message)
	{

		return Undefined();
	}
}

