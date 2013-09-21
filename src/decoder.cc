#include <v8.h>
#include <node.h>
#include <string>
#include <dbus/dbus.h>

#include "decoder.h"

namespace Decoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	Handle<Value> DecodeMessageIter(DBusMessageIter *iter, char *signature)
	{
		HandleScope scope;
		DBusSignatureIter siter;

		// Get type of current value
		switch(dbus_message_iter_get_arg_type(iter)) {
		case DBUS_TYPE_BOOLEAN:
		{
			dbus_bool_t value = false;
			dbus_message_iter_get_basic(iter, &value);
			return scope.Close(Boolean::New(value));
		}

		case DBUS_TYPE_BYTE:
		case DBUS_TYPE_INT16:
		case DBUS_TYPE_UINT16:
		case DBUS_TYPE_INT32:
		case DBUS_TYPE_UINT32:
		case DBUS_TYPE_INT64:
		case DBUS_TYPE_UINT64:
		{
			dbus_uint64_t value = 0;
			dbus_message_iter_get_basic(iter, &value);
			return scope.Close(Number::New(value));
		}

		case DBUS_TYPE_DOUBLE: 
		{
			double value = 0;
			dbus_message_iter_get_basic(iter, &value);
			return scope.Close(Number::New(value));
		}

		case DBUS_TYPE_OBJECT_PATH:
		case DBUS_TYPE_SIGNATURE:
		case DBUS_TYPE_STRING:
		{
			const char *value;
			dbus_message_iter_get_basic(iter, &value); 
			return scope.Close(String::New(value));
		}

		case DBUS_TYPE_STRUCT:
		case DBUS_TYPE_ARRAY:
		{
			bool is_dict = false;
			DBusMessageIter internal_iter;

			// Initializing signature
			dbus_signature_iter_init(&siter, signature);

			if (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRUCT ||
				dbus_signature_iter_get_current_type(&siter) == DBUS_TYPE_STRUCT) {

				DBusMessageIter struct_iter;
				is_dict = true;

				// Open stuct container
				dbus_message_iter_recurse(iter, &struct_iter);
				dbus_message_iter_recurse(&struct_iter, &internal_iter);

			} else {

				dbus_message_iter_recurse(iter, &internal_iter);

				if (dbus_message_iter_get_arg_type(&internal_iter) == DBUS_TYPE_DICT_ENTRY) {
					is_dict = true;
				} else if (dbus_message_iter_get_element_type(iter) == DBUS_TYPE_DICT_ENTRY) {
					is_dict = true;
				} else if (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_ARRAY &&
					dbus_signature_iter_get_current_type(&siter) == DBUS_TYPE_ARRAY) {

					if (dbus_signature_iter_get_element_type(&siter) == DBUS_TYPE_DICT_ENTRY)
						is_dict = true;
				}
			}

			// It's dictionary
			if (is_dict) {

				// Create a object
				Handle<Object> result = Object::New();

				// Make sure it doesn't empty
				if (dbus_message_iter_get_arg_type(&internal_iter) == DBUS_TYPE_INVALID)
					return scope.Close(result);

				do {
					// Getting sub iterator
					DBusMessageIter dict_entry_iter;
					dbus_message_iter_recurse(&internal_iter, &dict_entry_iter);

					// Getting key
					Handle<Value> key = DecodeMessageIter(&dict_entry_iter, dbus_message_iter_get_signature(&dict_entry_iter));
					if (key->IsUndefined())
						continue;

					// Try to get next element
					Handle<Value> value;
					if (dbus_message_iter_next(&dict_entry_iter)) {
						// Getting value
						value = DecodeMessageIter(&dict_entry_iter, dbus_message_iter_get_signature(&dict_entry_iter));
					} else {
						value = Undefined();
					}

					// Append a property
					result->Set(key, value); 

				} while(dbus_message_iter_next(&internal_iter));

				return scope.Close(result);
			}

			// Create an array
			unsigned int count = 0;
			Handle<Array> result = Array::New();
			if (dbus_message_iter_get_arg_type(&internal_iter) == DBUS_TYPE_INVALID)
				return scope.Close(result);

			do {

				// Getting element
				Handle<Value> value = DecodeMessageIter(&internal_iter, dbus_message_iter_get_signature(&internal_iter));
				if (value->IsUndefined())
					continue;

				// Append item
				result->Set(count, value);

				count++;
			} while(dbus_message_iter_next(&internal_iter));

			return scope.Close(result);
		}

		case DBUS_TYPE_VARIANT:
		{
			DBusMessageIter internal_iter;
			dbus_message_iter_recurse(iter, &internal_iter);

			return scope.Close(DecodeMessageIter(&internal_iter, dbus_message_iter_get_signature(&internal_iter)));
		}

		default:
			return Undefined();

		}

		return Undefined();
	}

	Handle<Value> DecodeMessage(DBusMessage *message)
	{
		HandleScope scope;
		DBusMessageIter iter;

		if (!dbus_message_iter_init(message, &iter))
			return Undefined();

		if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR)
			return Undefined();

		if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INVALID)
			return Undefined();

		char *signature = NULL;
		signature = dbus_message_iter_get_signature(&iter);
		Handle<Value> obj = DecodeMessageIter(&iter, signature);
		if (obj->IsUndefined())
			return Undefined();

		return scope.Close(obj);
	}

	Handle<Value> DecodeArguments(DBusMessage *message)
	{
		HandleScope scope;
		DBusMessageIter iter;
		Handle<Array> result = Array::New();

		if (!dbus_message_iter_init(message, &iter))
			return scope.Close(result);

		if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR)
			return scope.Close(result);

		// No argument
		if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INVALID)
			return scope.Close(result);

		unsigned int count = 0;
		char *signature = NULL;

		do {
			signature = dbus_message_iter_get_signature(&iter);
			Handle<Value> value = DecodeMessageIter(&iter, signature);
			if (value->IsUndefined())
				break;

			result->Set(count, value);

			count++;
		} while(dbus_message_iter_next(&iter));

		return scope.Close(result);
	}
}

