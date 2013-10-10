#include <v8.h>
#include <node.h>
#include <cstring>
#include <stdlib.h>
#include <dbus/dbus.h>

#include "encoder.h"

namespace Encoder {

	using namespace node;
	using namespace v8;
	using namespace std;

	const char *GetSignatureFromV8Type(Handle<Value>& value)
	{
		if (value->IsTrue() || value->IsFalse() || value->IsBoolean() ) {
			return const_cast<char*>(DBUS_TYPE_BOOLEAN_AS_STRING);
		} else if (value->IsInt32()) {
			return const_cast<char*>(DBUS_TYPE_INT32_AS_STRING);
		} else if (value->IsUint32()) {
			return const_cast<char*>(DBUS_TYPE_UINT32_AS_STRING);
		} else if (value->IsNumber()) {
			return const_cast<char*>(DBUS_TYPE_DOUBLE_AS_STRING);
		} else if (value->IsString()) {
			return const_cast<char*>(DBUS_TYPE_STRING_AS_STRING);
		} else if (value->IsArray()) {
			return const_cast<char*>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_VARIANT_AS_STRING);
		} else if (value->IsObject()) {
			return const_cast<char*>(DBUS_TYPE_ARRAY_AS_STRING
				DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
		} else {
			return NULL;
		}
	}

	bool EncodeObject(Handle<Value> value, DBusMessageIter *iter, const char *signature)
	{
		HandleScope scope;
		DBusSignatureIter siter;
		int type;

		// Get type of current value
		dbus_signature_iter_init(&siter, signature);
		type = dbus_signature_iter_get_current_type(&siter);

		switch(type) {
		case DBUS_TYPE_BOOLEAN:
		{
			dbus_bool_t data = value->BooleanValue();

			if (!dbus_message_iter_append_basic(iter, type, &data)) {
				printf("Failed to encode boolean value\n");
				return false;
			}

			break;
		}

		case DBUS_TYPE_INT16:
		case DBUS_TYPE_INT32:
		case DBUS_TYPE_INT64:
		case DBUS_TYPE_UINT16:
		case DBUS_TYPE_UINT32:
		case DBUS_TYPE_UINT64:
		case DBUS_TYPE_BYTE:
		{
			dbus_uint64_t data = value->IntegerValue();

			if (!dbus_message_iter_append_basic(iter, type, &data)) {
				printf("Failed to encode numeric value\n");
				return false;
			}

			break;
		}

		case DBUS_TYPE_STRING:
		case DBUS_TYPE_OBJECT_PATH:
		case DBUS_TYPE_SIGNATURE:
		{
			char *data = strdup(*String::Utf8Value(value->ToString()));

			if (!dbus_message_iter_append_basic(iter, type, &data)) {
				dbus_free(data);
				printf("Failed to encode string value\n");
				return false;
			}

			dbus_free(data);

			break;
		}

		case DBUS_TYPE_DOUBLE:
		{
			double data = value->NumberValue();

			if (!dbus_message_iter_append_basic(iter, type, &data)) {
				printf("Failed to encode double value\n");
				return false;
			}

			break;
		}

		case DBUS_TYPE_ARRAY:
		{
			if (!value->IsObject()) {
				printf("Failed to encode dictionary\n");
				return false;
			}

			DBusMessageIter subIter;
			DBusSignatureIter arraySiter;
			char *array_sig = NULL;

			// Getting signature of array object
			dbus_signature_iter_recurse(&siter, &arraySiter);
			array_sig = dbus_signature_iter_get_signature(&arraySiter);

			// Open array container to process elements in there
			if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, array_sig, &subIter)) {
				dbus_free(array_sig);
				printf("Can't open container for Array type\n");
				return false; 
			}

			// It's a dictionary
			if (dbus_signature_iter_get_element_type(&siter) == DBUS_TYPE_DICT_ENTRY) {

				dbus_free(array_sig);

				Handle<Object> value_object = value->ToObject();
				DBusSignatureIter dictSubSiter;

				// Getting sub-signature object
				dbus_signature_iter_recurse(&arraySiter, &dictSubSiter);
				dbus_signature_iter_next(&dictSubSiter);
				char *sig = dbus_signature_iter_get_signature(&dictSubSiter);

				// process each elements
				Handle<Array> prop_names = value_object->GetPropertyNames();
				unsigned int len = prop_names->Length();

				bool failed = false;
				for (unsigned int i = 0; i < len; ++i) {
					DBusMessageIter dict_iter;

					// Open dict entry container
					if (!dbus_message_iter_open_container(&subIter, DBUS_TYPE_DICT_ENTRY, NULL, &dict_iter)) {
						printf("Can't open container for DICT-ENTRY\n");
						failed = true;
						break;
					}

					// Getting the key and value
					Handle<Value> prop_key = prop_names->Get(i);
					Handle<Value> prop_value = value_object->Get(prop_key);

					// Append the key
					char *prop_key_str = strdup(*String::Utf8Value(prop_key->ToString()));
					dbus_message_iter_append_basic(&dict_iter, DBUS_TYPE_STRING, &prop_key_str);
					dbus_free(prop_key_str);

					// Append the value
					if (!EncodeObject(prop_value, &dict_iter, sig)) {
						dbus_message_iter_close_container(&subIter, &dict_iter); 
						printf("Failed to encode element of dictionary\n");
						failed = true;
						break;
					}

					dbus_message_iter_close_container(&subIter, &dict_iter); 
				}

				dbus_free(sig);
				dbus_message_iter_close_container(iter, &subIter);

				if (failed) 
					return false;

				break;
			}

			if (!value->IsArray()) {
				printf("Failed to encode array object\n");
				return false;
			}

			// process each elements
			Handle<Array> arrayData = Handle<Array>::Cast(value);
			for (unsigned int i = 0; i < arrayData->Length(); ++i) {
				Local<Value> arrayItem = arrayData->Get(i);
				if (!EncodeObject(arrayItem, &subIter, array_sig))
					break;
			}

			dbus_message_iter_close_container(iter, &subIter);
			dbus_free(array_sig);

			break;
		}

		case DBUS_TYPE_VARIANT:
		{
			DBusMessageIter subIter;

			const char *var_sig = GetSignatureFromV8Type(value);

			if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, var_sig, &subIter)) {
				printf("Can't open contianer for VARIANT type\n");
				return false;
			}

			if (!EncodeObject(value, &subIter, var_sig)) { 
				dbus_message_iter_close_container(iter, &subIter);
				return false;
			}

			dbus_message_iter_close_container(iter, &subIter);

			break;
		}
		case DBUS_TYPE_STRUCT:
		{
			DBusMessageIter subIter;
			DBusSignatureIter structSiter;
			
			// Open array container to process elements in there
			if (!dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, NULL, &subIter)) {
				printf("Can't open container for Struct type\n");
				return false; 
			}

			Handle<Object> value_object = value->ToObject();

			// Getting sub-signature object
			dbus_signature_iter_recurse(&siter, &structSiter);

			// process each elements
			Handle<Array> prop_names = value_object->GetPropertyNames();
			unsigned int len = prop_names->Length();

			for (unsigned int i = 0; i < len; ++i) {

				char *sig = dbus_signature_iter_get_signature(&structSiter);

				Handle<Value> prop_key = prop_names->Get(i);

				if (!EncodeObject(value_object->Get(prop_key), &subIter, sig)) {
					dbus_free(sig);
					printf("Failed to encode element of dictionary\n");
					return false;
				}

				dbus_free(sig);

				if (!dbus_signature_iter_next(&structSiter))
					break;
			}

			dbus_message_iter_close_container(iter, &subIter); 

			break;
		}

		}

		return true;
	}

}

