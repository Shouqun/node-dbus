#ifndef NODE_DBUS_H_
#define NODE_DBUS_H_

#include <v8.h>
#include <string>

namespace NodeDBus {

#define NATIVE_NODE_DEFINE_CONSTANT(target, name, constant)					\
	(target)->Set(v8::String::NewSymbol(name),							\
	v8::Integer::New(constant),											\
	static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

	struct NodeCallback {
		v8::Persistent<v8::Object> Holder;
		v8::Persistent<v8::Function> cb;

		~NodeCallback() {
			Holder.Dispose();
			cb.Dispose();
		}
	};

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
}

#endif
