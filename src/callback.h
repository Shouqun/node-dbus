#ifndef CALLBACK_H_
#define CALLBACK_H_

namespace Callback {

	using namespace node;
	using namespace v8;
	using namespace std;

	struct CallbackData {
		Persistent<Function> callback;
		Persistent<Object> argv;
		unsigned int argc;
	};

	void Invoke(CallbackData *callback_data);
}

#endif
