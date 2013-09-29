#ifndef NODE_DBUS_H_
#define NODE_DBUS_H_

#include <v8.h>
#include <string>

namespace NodeDBus {

	using namespace node;
	using namespace v8;
	using namespace std;

#define NATIVE_NODE_DEFINE_CONSTANT(target, name, constant)					\
	(target)->Set(v8::String::NewSymbol(name),							\
	v8::Integer::New(constant),											\
	static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

	typedef struct DBusAsyncData {
		Persistent<Function> callback;
		DBusPendingCall *pending;
	} DBusAsyncData;

	typedef enum {
		NODE_DBUS_BUS_SYSTEM,
		NODE_DBUS_BUS_SESSION
	} BusType;

	typedef struct {
		BusType type;
		DBusConnection *connection;
	} BusObject;

	typedef struct {
		BusObject *bus;
		std::string service;
		std::string object_path;
		std::string interface;
	} InterfaceObject;

	void EmitSignal(Handle<Value> args);
}

#endif
