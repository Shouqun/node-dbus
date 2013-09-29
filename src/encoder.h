#ifndef ENCODER_H_
#define ENCODER_H_

namespace Encoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	const char *GetSignatureFromV8Type(Local<Value>& value);
	bool EncodeObject(Handle<Value> value, DBusMessageIter *iter, const char *signature);
}

#endif
