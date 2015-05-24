#include <v8.h>
#include <node.h>
#include <cstring>
#include <expat.h>
#include <dbus/dbus.h>
#include <nan.h>

#include "introspect.h"

namespace Introspect {

	using namespace node;
	using namespace v8;
	using namespace std;

	static const char *GetAttribute(const char **attrs, const char *name)
	{
		int i = 0;

		while(attrs[i] != NULL) {

			if (!strcasecmp(attrs[i], name))
				return attrs[i+1];

			i += 2;
		}

		return "";
	}

	static void StartElementHandler(void *user_data, const XML_Char *name, const XML_Char **attrs)
	{
		NanScope();
		IntrospectObject *introspect_obj = static_cast<IntrospectObject *>(user_data);

		if (strcmp(name, "node") == 0) {

			if (introspect_obj->current_class == INTROSPECT_NONE) {
				NanAssignPersistent(introspect_obj->obj, NanNew<Object>());
				introspect_obj->current_class = INTROSPECT_ROOT;
			}
		
		} else if (strcmp(name, "interface") == 0) {

			Local<Object> obj = NanNew(introspect_obj->obj);

			// Create a new object for interface
			Local<Object> interface = NanNew<Object>();
			interface->Set(NanNew("method"), NanNew<Object>());
			interface->Set(NanNew("property"), NanNew<Object>());
			interface->Set(NanNew("signal"), NanNew<Object>());

			obj->Set(NanNew<String>(GetAttribute(attrs, "name")), interface);

			NanDisposePersistent(introspect_obj->current_interface);
			NanAssignPersistent(introspect_obj->current_interface, interface);

		} else if (strcmp(name, "method") == 0) {

			Local<Object> interface = NanNew(introspect_obj->current_interface);
			Local<Object> method_class = Local<Object>::Cast(interface->Get(NanNew("method")));

			// Create a new object for this method
			Local<Object> method = NanNew<Object>();
			method->Set(NanNew("in"), NanNew<Array>());

			method_class->Set(NanNew<String>(GetAttribute(attrs, "name")), method);

			introspect_obj->current_class = INTROSPECT_METHOD;

			NanDisposePersistent(introspect_obj->current_method);
			NanAssignPersistent(introspect_obj->current_method, method);

		} else if (strcmp(name, "property") == 0) {

			Local<Object> interface = NanNew(introspect_obj->current_interface);
			Local<Object> property_class = Local<Object>::Cast(interface->Get(NanNew("property")));

			// Create a new object for this property
			Local<Object> method = NanNew<Object>();
			method->Set(NanNew("type"), NanNew<String>(GetAttribute(attrs, "type")));
			method->Set(NanNew("access"), NanNew<String>(GetAttribute(attrs, "access")));

			property_class->Set(NanNew<String>(GetAttribute(attrs, "name")), method);

			introspect_obj->current_class = INTROSPECT_PROPERTY;

		} else if (strcmp(name, "signal") == 0) {

			Local<Object> interface = NanNew(introspect_obj->current_interface);
			Local<Object> signal_class = Local<Object>::Cast(interface->Get(NanNew("signal")));

			// Create a new object for this signal
			Local<Array> signal = NanNew<Array>();

			signal_class->Set(NanNew<String>(GetAttribute(attrs, "name")), signal);

			introspect_obj->current_class = INTROSPECT_SIGNAL;

			NanDisposePersistent(introspect_obj->current_signal);
			NanAssignPersistent(introspect_obj->current_signal, signal);

		} else if (strcmp(name, "arg") == 0) {

			switch(introspect_obj->current_class) {
			case INTROSPECT_METHOD:
			{

				// Argument for method
				Local<Object> method = NanNew(introspect_obj->current_method);

				if (strcmp(GetAttribute(attrs, "direction"), "in") == 0) {
					Local<Array> arguments = Local<Array>::Cast(method->Get(NanNew("in")));
					arguments->Set(arguments->Length(), NanNew<String>(GetAttribute(attrs, "type")));
				} else {
					method->Set(NanNew("out"), NanNew<String>(GetAttribute(attrs, "type")));
				}

				break;
			}

			case INTROSPECT_PROPERTY:
				// Do nothing
				break;

			case INTROSPECT_SIGNAL:
			{
				// Argument for signal
				Local<Array> signal = NanNew(introspect_obj->current_signal);

				signal->Set(signal->Length(), NanNew<String>(GetAttribute(attrs, "type")));

				break;
			}

			default:
				break;

			}

		}
	}

	Local<Value> CreateObject(const char *source)
	{
		NanEscapableScope();
		IntrospectObject *introspect_obj = new IntrospectObject;
		introspect_obj->current_class = INTROSPECT_NONE;

		// Initializing XML parser
		XML_Parser parser = XML_ParserCreate(NULL);
		XML_SetUserData(parser, introspect_obj);
		XML_SetElementHandler(parser, StartElementHandler, NULL);

		// Start to parse source
		if (!XML_Parse(parser, source, strlen(source), true)) {
			NanDisposePersistent(introspect_obj->obj);
			NanDisposePersistent(introspect_obj->current_interface);
			NanDisposePersistent(introspect_obj->current_method);
			NanDisposePersistent(introspect_obj->current_signal);
			delete introspect_obj;

			return NanNull();
		}

		XML_ParserFree(parser);

		Local<Object> obj = NanNew(introspect_obj->obj);

		// Clear
		NanDisposePersistent(introspect_obj->obj);
		NanDisposePersistent(introspect_obj->current_interface);
		NanDisposePersistent(introspect_obj->current_method);
		NanDisposePersistent(introspect_obj->current_signal);
		delete introspect_obj;

		return NanEscapeScope(obj);
	}

}

