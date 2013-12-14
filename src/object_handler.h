#ifndef OBJECT_HANDLER_H_
#define OBJECT_HANDLER_H_

namespace ObjectHandler {

	using namespace node;
	using namespace v8;
	using namespace std;

	Handle<Value> RegisterObjectPath(Arguments const &args);
	Handle<Value> SendMessageReply(Arguments const &args);
	Handle<Value> SendErrorMessageReply(Arguments const &args);
	Handle<Value> SetObjectHandler(const Arguments& args);
}

#endif
