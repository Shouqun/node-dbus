#include <v8.h>
#include <node.h>
#include <string>
#include <dbus/dbus.h>
#include <nan.h>

#include "decoder.h"

namespace Decoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	Handle<Value> DecodeMessageIter(DBusMessageIter *iter, const char *signature)
	{
		NanEscapableScope();
		DBusSignatureIter siter;
		int cur_type = dbus_message_iter_get_arg_type(iter);

		// Trying to use signature to handle this iterator if type cannot be recognized
		if (cur_type == DBUS_TYPE_INVALID) {
			dbus_signature_iter_init(&siter, signature);
			cur_type = dbus_signature_iter_get_current_type(&siter);
		}

		// Get type of current value
		switch(cur_type) {
		case DBUS_TYPE_BOOLEAN:
		{
			dbus_bool_t value = false;
			dbus_message_iter_get_basic(iter, &value);
			return NanEscapeScope(NanNew<Boolean>(value));
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
			return NanEscapeScope(NanNew<Number>(value));
		}

		case DBUS_TYPE_DOUBLE: 
		{
			double value = 0;
			dbus_message_iter_get_basic(iter, &value);
			return NanEscapeScope(NanNew<Number>(value));
		}

		case DBUS_TYPE_OBJECT_PATH:
		case DBUS_TYPE_SIGNATURE:
		case DBUS_TYPE_STRING:
		{
			const char *value;
			dbus_message_iter_get_basic(iter, &value); 
			return NanEscapeScope(NanNew<String>(value));
		}

		case DBUS_TYPE_STRUCT:
		{
			char *sig = NULL;

			// Create a object
			Handle<Object> result = NanNew<Object>();

			do {
				// Getting sub iterator
				DBusMessageIter dict_entry_iter;
				dbus_message_iter_recurse(iter, &dict_entry_iter);

				// Getting key
				sig = dbus_message_iter_get_signature(&dict_entry_iter);
				Handle<Value> key = DecodeMessageIter(&dict_entry_iter, sig);
				dbus_free(sig);
				if (key->IsUndefined())
					continue;

				// Try to get next element
				Handle<Value> value;
				if (dbus_message_iter_next(&dict_entry_iter)) {
					// Getting value
					sig = dbus_message_iter_get_signature(&dict_entry_iter);
					value = DecodeMessageIter(&dict_entry_iter, sig);
					dbus_free(sig);
				} else {
					value = NanUndefined();
				}

				// Append a property
				result->Set(key, value); 

			} while(dbus_message_iter_next(iter));

			return NanEscapeScope(result);
		}

		case DBUS_TYPE_ARRAY:
		{
			bool is_dict = false;
			char *sig = NULL;
			DBusMessageIter internal_iter;

			// Initializing signature
			dbus_signature_iter_init(&siter, signature);
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

			// It's dictionary
			if (is_dict) {

				// Create a object
				Handle<Object> result = NanNew<Object>();

				// Make sure it doesn't empty
				if (dbus_message_iter_get_arg_type(&internal_iter) == DBUS_TYPE_INVALID)
					return NanEscapeScope(result);

				do {
					// Getting sub iterator
					DBusMessageIter dict_entry_iter;
					dbus_message_iter_recurse(&internal_iter, &dict_entry_iter);

					// Getting key
					sig = dbus_message_iter_get_signature(&dict_entry_iter);
					Handle<Value> key = DecodeMessageIter(&dict_entry_iter, sig);
					dbus_free(sig);
					if (key->IsUndefined())
						continue;

					// Try to get next element
					Handle<Value> value;
					if (dbus_message_iter_next(&dict_entry_iter)) {
						// Getting value
						sig = dbus_message_iter_get_signature(&dict_entry_iter);
						value = DecodeMessageIter(&dict_entry_iter, sig);
						dbus_free(sig);
					} else {
						value = NanUndefined();
					}

					// Append a property
					result->Set(key, value); 

				} while(dbus_message_iter_next(&internal_iter));

				return NanEscapeScope(result);
			}

			// Create an array
			unsigned int count = 0;
			Handle<Array> result = NanNew<Array>();
			if (dbus_message_iter_get_arg_type(&internal_iter) == DBUS_TYPE_INVALID)
				return NanEscapeScope(result);

			do {

				// Getting element
				sig = dbus_message_iter_get_signature(&internal_iter);
				Handle<Value> value = DecodeMessageIter(&internal_iter, sig);
				dbus_free(sig);
				if (value->IsUndefined())
					continue;

				// Append item
				result->Set(count, value);

				count++;
			} while(dbus_message_iter_next(&internal_iter));

			return NanEscapeScope(result);
		}

		case DBUS_TYPE_VARIANT:
		{
			char *sig = NULL;
			DBusMessageIter internal_iter;
			dbus_message_iter_recurse(iter, &internal_iter);

			sig = dbus_message_iter_get_signature(&internal_iter);
			Handle<Value> value = DecodeMessageIter(&internal_iter, sig);
			dbus_free(sig);

			return NanEscapeScope(value);
		}

		default:
			return NanUndefined();

		}

		return NanUndefined();
	}

	Handle<Value> DecodeMessage(DBusMessage *message)
	{
		NanEscapableScope();
		DBusMessageIter iter;

		if (!dbus_message_iter_init(message, &iter))
			return NanUndefined();

		if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR)
			return NanUndefined();

		if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INVALID)
			return NanUndefined();

		Handle<Array> result = NanNew<Array>();
		unsigned int count = 0;
		char *signature = NULL;
		Handle<Value> value;

		do {
			signature = dbus_message_iter_get_signature(&iter);
			value = DecodeMessageIter(&iter, signature);
			dbus_free(signature);
			if (value->IsUndefined())
				continue;

			result->Set(count, value);

			count++;
		} while(dbus_message_iter_next(&iter));

		if (count == 1) {
			return NanEscapeScope(value);
		}

		return NanEscapeScope(result);
	}

	Handle<Value> DecodeArguments(DBusMessage *message)
	{
		NanEscapableScope();
		DBusMessageIter iter;
		Handle<Array> result = NanNew<Array>();

		if (!dbus_message_iter_init(message, &iter))
			return NanEscapeScope(result);

		if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR)
			return NanEscapeScope(result);

		// No argument
		if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_INVALID)
			return NanEscapeScope(result);

		unsigned int count = 0;
		char *signature = NULL;

		do {
			signature = dbus_message_iter_get_signature(&iter);
			Handle<Value> value = DecodeMessageIter(&iter, signature);
			dbus_free(signature);
			if (value->IsUndefined())
				continue;

			result->Set(count, value);

			count++;
		} while(dbus_message_iter_next(&iter));

		return NanEscapeScope(result);
	}
}

