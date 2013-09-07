#include <v8.h>
#include <node.h>
#include <string>
#include <dbus/dbus.h>

#include "decoder.h"

namespace Decoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	Handle<Value> DecodeMessageIter(DBusMessageIter *iter)
	{
		switch(dbus_message_iter_get_arg_type(iter)) {
		case DBUS_TYPE_BOOLEAN:
		{
			dbus_bool_t value = false;
			dbus_message_iter_get_basic(iter, &value);
			return Boolean::New(value);
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
			return Number::New(value);
		}

		case DBUS_TYPE_DOUBLE: 
		{
			double value = 0;
			dbus_message_iter_get_basic(iter, &value);
			return Number::New(value);
		}

		case DBUS_TYPE_OBJECT_PATH:
		case DBUS_TYPE_SIGNATURE:
		case DBUS_TYPE_STRING:
		{
			const char *value;
			dbus_message_iter_get_basic(iter, &value); 
			return String::New(value);
		}

		case DBUS_TYPE_ARRAY:
		case DBUS_TYPE_STRUCT:
		{
			DBusMessageIter internal_iter;
			unsigned int count = 0;

			dbus_message_iter_recurse(iter, &internal_iter);

			// It's dictionary
			if (dbus_message_iter_get_arg_type(&internal_iter) == DBUS_TYPE_DICT_ENTRY) {

				// Create a object
				Local<Object> result = Object::New();
				do {

					// Getting sub iterator
					DBusMessageIter dict_entry_iter;
					dbus_message_iter_recurse(&internal_iter, &dict_entry_iter);

					// Getting key and value
					Handle<Value> key = DecodeMessageIter(&dict_entry_iter);
					Handle<Value> value = DecodeMessageIter(&dict_entry_iter);
					if (key->IsUndefined())
						continue;

					// Append a property
					result->Set(key, value); 

				} while(dbus_message_iter_next(&internal_iter));

				return result;
			}

			// Create an array
			Local<Array> result = Array::New(0);
			do {

				// Getting element
				Handle<Value> value = DecodeMessageIter(&internal_iter);
				if (value->IsUndefined())
					continue;

				// Append item
				result->Set(count, value);

				count++;
			} while(dbus_message_iter_next(&internal_iter));

			// No element in this array
			if (count == 0)
				return Array::New(0);

			return result;
		}

		case DBUS_TYPE_VARIANT:
		{
			DBusMessageIter internal_iter;
			dbus_message_iter_recurse(iter, &internal_iter);

			return DecodeMessageIter(&internal_iter);
		}

		}

		return Undefined();
	}

	Handle<Value> DecodeMessage(DBusMessage *message)
	{
		DBusMessageIter iter;

		dbus_message_iter_init(message, &iter);

		return DecodeMessageIter(&iter);
	}
}

