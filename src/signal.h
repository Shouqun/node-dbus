#ifndef SIGNAL_H_
#define SIGNAL_H_

namespace Signal {

	using namespace node;
	using namespace v8;
	using namespace std;

	void DispatchSignal(Handle<Value> args[]);
	Handle<Value> EmitSignal(const Arguments& args);
	Handle<Value> SetSignalHandler(const Arguments& args);
}

#endif
