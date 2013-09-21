#include <v8.h>
#include <node.h>
#include <uv.h>
#include <cstring>

#include "callback.h"

namespace Callback {

	using namespace node;
	using namespace v8;
	using namespace std;

	static void InvokeCallback(uv_work_t* req)
	{
	}

	static void ReleaseCallback(uv_work_t* req)
	{
		HandleScope scope;
		CallbackData *data = static_cast<CallbackData *>(req->data);

		TryCatch try_catch;

		// Preparing arguments
		Handle<Value> args[data->argc];
		for (unsigned int i = 0; i < data->argc; ++i) {
			args[i] = data->argv->Get(i);
		}

		// Invoke
		data->callback->Call(data->callback, data->argc, args);

		if (try_catch.HasCaught()) {
			printf("Ooops, Exception on call the callback\n%s\n", *String::Utf8Value(try_catch.StackTrace()->ToString()));
		}

		// Release
		data->argv.Dispose();
		data->argv.Clear();
		data->callback.Dispose();
		data->callback.Clear();
		delete data;

		req->data = NULL;
	}

	void Invoke(CallbackData *callback_data)
	{
		// Asynchronus
		uv_work_t *req = new uv_work_t();
		req->data = callback_data;
		uv_queue_work(uv_default_loop(), req, InvokeCallback, (uv_after_work_cb)ReleaseCallback);
	}
}

