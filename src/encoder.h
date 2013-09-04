#ifndef ENCODER_H_
#define ENCODER_H_

namespace Encoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	bool EncodeObject(Local<Value> value, DBusMessageIter *iter, char *signature);
}

#endif
