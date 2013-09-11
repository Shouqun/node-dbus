#ifndef DECODER_H_
#define DECODER_H_

namespace Decoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	Handle<Value> DecodeMessage(DBusMessage *message);
	Handle<Value> DecodeArguments(DBusMessage *message);
}

#endif
