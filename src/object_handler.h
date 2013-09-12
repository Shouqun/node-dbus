#ifndef OBJECT_HANDLER_H_
#define OBJECT_HANDLER_H_

namespace ObjectHandler {

	using namespace node;
	using namespace v8;
	using namespace std;

	void SetHandler(Handle<Object> Holder, Handle<Function> callback);
	Handle<Value> RegisterObjectPath(Arguments const &args);
	Handle<Value> SendMessageReply(Arguments const &args);
}

#endif
