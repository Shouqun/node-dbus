#include <dbus/dbus.h>
#include <nan.h>
#include <node.h>
#include <v8.h>
#include <cstring>

#include "dbus.h"
#include "encoder.h"
#include "signal.h"

namespace Signal {

using namespace node;
using namespace v8;
using namespace std;

// Persistent<Function> handler =
// Nan::Persistent<Function>::New(Handle<Function>::Cast(Nan::Null()));
bool hookSignal = false;
Nan::Persistent<Function> handler;

void DispatchSignal(Local<Value> info[]) {
  Nan::HandleScope scope;

  if (!hookSignal) return;

  //		MakeCallback(handler, handler, 6, info);
  Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(handler), 6,
                    info);
}

NAN_METHOD(SetSignalHandler) {
  //		handler.Dispose();
  //		handler.Clear();

  hookSignal = true;
  handler.Reset(info[0].As<Function>());
  //		handler =
  //Nan::Persistent<Function>::New(Handle<Function>::Cast(info[0]));

  return;
}

NAN_METHOD(EmitSignal) {
  DBusError error;

  if (!info[0]->IsObject()) {
    return Nan::ThrowTypeError("First parameter must be an object");
  }

  // Object path
  if (!info[1]->IsString()) {
    return Nan::ThrowTypeError("Require object path");
  }

  // Interface name
  if (!info[2]->IsString()) {
    return Nan::ThrowTypeError("Require interface");
  }

  // Signal name
  if (!info[3]->IsString()) {
    return Nan::ThrowTypeError("Require signal name");
  }

  // Arguments
  if (!info[4]->IsArray()) {
    return Nan::ThrowTypeError("Require arguments");
  }

  // Signatures
  if (!info[5]->IsArray()) {
    return Nan::ThrowTypeError("Require signature");
  }

  node_dbus::BusObject* bus = static_cast<node_dbus::BusObject*>(
      Nan::GetInternalFieldPointer(info[0]->ToObject(
          Nan::GetCurrentContext()).ToLocalChecked(), 0));
  DBusMessage* message;
  DBusMessageIter iter;

  // Create a signal
  char* path = strdup(*String::Utf8Value(
      v8::Isolate::GetCurrent(),
      info[1]->ToString(Nan::GetCurrentContext()).ToLocalChecked()));
  char* interface = strdup(*String::Utf8Value(
      v8::Isolate::GetCurrent(),
      info[2]->ToString(Nan::GetCurrentContext()).ToLocalChecked()));
  char* signal = strdup(*String::Utf8Value(
      v8::Isolate::GetCurrent(),
      info[3]->ToString(Nan::GetCurrentContext()).ToLocalChecked()));
  message = dbus_message_new_signal(path, interface, signal);

  // Initializing error handler
  dbus_error_init(&error);

  // Preparing message
  dbus_message_iter_init_append(message, &iter);

  // Preparing arguments
  Local<Array> arguments = Local<Array>::Cast(info[4]);
  Local<Array> signatures = Local<Array>::Cast(info[5]);
  for (unsigned int i = 0; i < arguments->Length(); ++i) {
    Local<Value> arg = arguments->Get(
        Nan::GetCurrentContext(), i).ToLocalChecked();
    DBusSignatureIter siter;

    char* sig = strdup(*String::Utf8Value(
        v8::Isolate::GetCurrent(),
        signatures->Get(Nan::GetCurrentContext(), i).ToLocalChecked()->
            ToString(Nan::GetCurrentContext()).ToLocalChecked()));
    if (!dbus_signature_validate(sig, &error)) {
      return Nan::ThrowError(error.message);
    }

    dbus_signature_iter_init(&siter, sig);
    if (!Encoder::EncodeObject(arg, &iter, &siter)) {
      dbus_free(sig);
      printf("Failed to encode arguments of signal\n");
      break;
    }

    dbus_free(sig);
  }

  // Send out message
  dbus_connection_send(bus->connection, message, nullptr);
  dbus_connection_flush(bus->connection);
  dbus_message_unref(message);

  dbus_free(path);
  dbus_free(interface);
  dbus_free(signal);

  return;
}
}  // namespace Signal
