#ifndef SIGNAL_H_
#define SIGNAL_H_

namespace Signal {

	using namespace node;
	using namespace v8;
	using namespace std;

	void EmitSignal(Handle<Value> args[]);
	void SetHandler(Handle<Object> Holder, Handle<Function> callback);
}

#endif
