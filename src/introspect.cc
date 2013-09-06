#include <v8.h>
#include <node.h>
#include <string>
#include <expat.h>
#include <dbus/dbus.h>

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
		IntrospectObject *introspect_obj = reinterpret_cast<IntrospectObject *>(user_data);

		if (strcmp(name, "node") == 0) {

			if (introspect_obj->current_class == INTROSPECT_NONE) {
				introspect_obj->obj = Object::New();
				introspect_obj->current_class = INTROSPECT_ROOT;
			}
		
		} else if (strcmp(name, "interface") == 0) {

			Local<Object> obj = introspect_obj->obj;

			// Create a new object for interface
			Local<Object> interface = Object::New();
			interface->Set(String::NewSymbol("method"), Object::New());
			interface->Set(String::NewSymbol("property"), Object::New());
			interface->Set(String::NewSymbol("signal"), Object::New());

			obj->Set(String::New(GetAttribute(attrs, "name")), interface);

			introspect_obj->current_interface = interface;

		} else if (strcmp(name, "method") == 0) {

			Local<Object> interface = introspect_obj->current_interface;
			Local<Object> method_class = Local<Object>::Cast(interface->Get(String::NewSymbol("method")));

			// Create a new object for this method
			Local<Object> method = Object::New();
			method->Set(String::NewSymbol("in"), Array::New());

			method_class->Set(String::New(GetAttribute(attrs, "name")), method);

			introspect_obj->current_class = INTROSPECT_METHOD;
			introspect_obj->current_method = method;

		} else if (strcmp(name, "property") == 0) {

			Local<Object> interface = introspect_obj->current_interface;
			Local<Object> property_class = Local<Object>::Cast(interface->Get(String::NewSymbol("property")));

			// Create a new object for this property
			Local<Object> method = Object::New();
			method->Set(String::NewSymbol("type"), String::New(GetAttribute(attrs, "type")));
			method->Set(String::NewSymbol("access"), String::New(GetAttribute(attrs, "access")));

			property_class->Set(String::New(GetAttribute(attrs, "name")), method);

			introspect_obj->current_class = INTROSPECT_PROPERTY;

		} else if (strcmp(name, "signal") == 0) {

			Local<Object> interface = introspect_obj->current_interface;
			Local<Object> signal_class = Local<Object>::Cast(interface->Get(String::NewSymbol("signal")));

			// Create a new object for this signal
			Local<Array> signal = Array::New();

			signal_class->Set(String::New(GetAttribute(attrs, "name")), signal);

			introspect_obj->current_class = INTROSPECT_SIGNAL;
			introspect_obj->current_signal = signal;

		} else if (strcmp(name, "arg") == 0) {

			switch(introspect_obj->current_class) {
			case INTROSPECT_METHOD:
			{

				// Argument for method
				Local<Object> method = introspect_obj->current_method;

				if (strcmp(GetAttribute(attrs, "direction"), "in") == 0) {
					Local<Array> arguments = Local<Array>::Cast(method->Get(String::NewSymbol("in")));
					arguments->Set(arguments->Length(), String::New(GetAttribute(attrs, "type")));
				} else {
					method->Set(String::NewSymbol("out"), String::New(GetAttribute(attrs, "type")));
				}

				break;
			}

			case INTROSPECT_PROPERTY:
				break;

			case INTROSPECT_SIGNAL:
			{
				// Argument for signal
				Local<Array> signal = introspect_obj->current_signal;

				signal->Set(signal->Length(), String::New(GetAttribute(attrs, "type")));

				break;
			}

			default:
				break;

			}

		}
	}

	Handle<Value> CreateObject(const char *source)
	{
		IntrospectObject *introspect_obj = new IntrospectObject;
		introspect_obj->current_class = INTROSPECT_NONE;

		// Initializing XML parser
		XML_Parser parser = XML_ParserCreate(NULL);
		XML_SetUserData(parser, introspect_obj);
		XML_SetElementHandler(parser, StartElementHandler, NULL);

		// Start to parse source
		if (!XML_Parse(parser, source, strlen(source), true)) {
			return Undefined();
		}

		Local<Object> obj = introspect_obj->obj;

		delete introspect_obj;

		return obj;
	}

}

