#include <expat.h>
#include <nan.h>
#include <node.h>
#include <v8.h>
#include <cstring>

#include "introspect.h"

namespace Introspect {

using namespace node;
using namespace v8;
using namespace std;

static const char* GetAttribute(const char** attrs, const char* name) {
  int i = 0;

  while (attrs[i] != nullptr) {
    if (!strcasecmp(attrs[i], name)) return attrs[i + 1];

    i += 2;
  }

  return "";
}

static void StartElementHandler(void* user_data, const XML_Char* name,
                                const XML_Char** attrs) {
  Nan::HandleScope scope;
  IntrospectObject* introspect_obj = static_cast<IntrospectObject*>(user_data);

  if (strcmp(name, "node") == 0) {
    if (introspect_obj->current_class == INTROSPECT_NONE) {
      introspect_obj->obj.Reset(Nan::New<Object>());
      introspect_obj->current_class = INTROSPECT_ROOT;
    }

  } else if (strcmp(name, "interface") == 0) {
    Local<Object> obj = Nan::New(introspect_obj->obj);

    // Create a new object for interface
    Local<Object> interface = Nan::New<Object>();
    interface->Set(Nan::GetCurrentContext(),
                   Nan::New("method").ToLocalChecked(),
                   Nan::New<Object>());
    interface->Set(Nan::GetCurrentContext(),
                   Nan::New("property").ToLocalChecked(),
                   Nan::New<Object>());
    interface->Set(Nan::GetCurrentContext(),
                   Nan::New("signal").ToLocalChecked(),
                   Nan::New<Object>());

    obj->Set(Nan::GetCurrentContext(),
             Nan::New<String>(GetAttribute(attrs, "name")).ToLocalChecked(),
             interface);

    introspect_obj->current_interface.Reset();
    introspect_obj->current_interface.Reset(interface);

  } else if (strcmp(name, "method") == 0) {
    Local<Object> interface = Nan::New(introspect_obj->current_interface);
    MaybeLocal<Value> maybe = interface->Get(
        Nan::GetCurrentContext(), Nan::New("method").ToLocalChecked());
    Local<Object> method_class = Local<Object>::Cast(maybe.ToLocalChecked());

    // Create a new object for this method
    Local<Object> method = Nan::New<Object>();
    method->Set(Nan::GetCurrentContext(),
                Nan::New("in").ToLocalChecked(),
                Nan::New<Array>());
    method->Set(Nan::GetCurrentContext(),
                Nan::New("out").ToLocalChecked(),
                Nan::New<Array>());

    method_class->Set(
        Nan::GetCurrentContext(),
        Nan::New<String>(GetAttribute(attrs, "name")).ToLocalChecked(),
        method);

    introspect_obj->current_class = INTROSPECT_METHOD;

    introspect_obj->current_method.Reset();
    introspect_obj->current_method.Reset(method);

  } else if (strcmp(name, "property") == 0) {
    Local<Object> interface = Nan::New(introspect_obj->current_interface);
    MaybeLocal<Value> maybe = interface->Get(
        Nan::GetCurrentContext(), Nan::New("property").ToLocalChecked());
    Local<Object> property_class = Local<Object>::Cast(maybe.ToLocalChecked());

    // Create a new object for this property
    Local<Object> method = Nan::New<Object>();
    method->Set(Nan::GetCurrentContext(),
                Nan::New("type").ToLocalChecked(),
                Nan::New<String>(GetAttribute(attrs, "type")).ToLocalChecked());
    method->Set(
        Nan::GetCurrentContext(),
        Nan::New("access").ToLocalChecked(),
        Nan::New<String>(GetAttribute(attrs, "access")).ToLocalChecked());

    property_class->Set(
        Nan::GetCurrentContext(),
        Nan::New<String>(GetAttribute(attrs, "name")).ToLocalChecked(), method);

    introspect_obj->current_class = INTROSPECT_PROPERTY;

  } else if (strcmp(name, "signal") == 0) {
    Local<Object> interface = Nan::New(introspect_obj->current_interface);
    MaybeLocal<Value> maybe = interface->Get(
        Nan::GetCurrentContext(), Nan::New("signal").ToLocalChecked());
    Local<Object> signal_class = Local<Object>::Cast(maybe.ToLocalChecked());

    // Create a new object for this signal
    Local<Array> signal = Nan::New<Array>();

    signal_class->Set(
        Nan::GetCurrentContext(),
        Nan::New<String>(GetAttribute(attrs, "name")).ToLocalChecked(), signal);

    introspect_obj->current_class = INTROSPECT_SIGNAL;

    introspect_obj->current_signal.Reset();
    introspect_obj->current_signal.Reset(signal);

  } else if (strcmp(name, "arg") == 0) {
    switch (introspect_obj->current_class) {
      case INTROSPECT_METHOD: {
        // Argument for method
        Local<Object> method = Nan::New(introspect_obj->current_method);

        if (strcmp(GetAttribute(attrs, "direction"), "in") == 0) {
          MaybeLocal<Value> maybe = method->Get(
              Nan::GetCurrentContext(), Nan::New("in").ToLocalChecked());
          Local<Array> arguments = Local<Array>::Cast(maybe.ToLocalChecked());
          arguments->Set(
              Nan::GetCurrentContext(),
              arguments->Length(),
              Nan::New<String>(GetAttribute(attrs, "type")).ToLocalChecked());
        } else {
          MaybeLocal<Value> maybe = method->Get(
              Nan::GetCurrentContext(), Nan::New("out").ToLocalChecked());
          Local<Array> arguments = Local<Array>::Cast(maybe.ToLocalChecked());
          arguments->Set(
              Nan::GetCurrentContext(),
              arguments->Length(),
              Nan::New<String>(GetAttribute(attrs, "type")).ToLocalChecked());
        }

        break;
      }

      case INTROSPECT_PROPERTY:
        // Do nothing
        break;

      case INTROSPECT_SIGNAL: {
        // Argument for signal
        Local<Array> signal = Nan::New(introspect_obj->current_signal);

        signal->Set(
            Nan::GetCurrentContext(),
            signal->Length(),
            Nan::New<String>(GetAttribute(attrs, "type")).ToLocalChecked());

        break;
      }

      default:
        break;
    }
  }
}

Local<Value> CreateObject(const char* source) {
  Nan::EscapableHandleScope scope;
  IntrospectObject* introspect_obj = new IntrospectObject;
  introspect_obj->current_class = INTROSPECT_NONE;

  // Initializing XML parser
  XML_Parser parser = XML_ParserCreate(nullptr);
  XML_SetUserData(parser, introspect_obj);
  XML_SetElementHandler(parser, StartElementHandler, nullptr);

  // Start to parse source
  if (!XML_Parse(parser, source, strlen(source), true)) {
    introspect_obj->obj.Reset();
    introspect_obj->current_interface.Reset();
    introspect_obj->current_method.Reset();
    introspect_obj->current_signal.Reset();
    delete introspect_obj;

    return Nan::Null();
  }

  XML_ParserFree(parser);

  Local<Object> obj = Nan::New(introspect_obj->obj);

  // Clear
  introspect_obj->obj.Reset();
  introspect_obj->current_interface.Reset();
  introspect_obj->current_method.Reset();
  introspect_obj->current_signal.Reset();
  delete introspect_obj;

  return scope.Escape(obj);
}

}  // namespace Introspect
