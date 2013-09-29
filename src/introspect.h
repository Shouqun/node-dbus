#ifndef INTROSPECT_H_
#define INTROSPECT_H_

namespace Introspect {

	using namespace node;
	using namespace v8;
	using namespace std;

	typedef enum {
		INTROSPECT_NONE,
		INTROSPECT_ROOT,
		INTROSPECT_METHOD,
		INTROSPECT_PROPERTY,
		INTROSPECT_SIGNAL
	} IntrospectClass;

	typedef struct {
		Persistent<Object> obj;
		Persistent<Object> current_interface;
		Persistent<Object> current_method;
		Persistent<Array> current_signal;
		IntrospectClass current_class;
	} IntrospectObject;

	Handle<Value> CreateObject(const char *source);
}

#endif
