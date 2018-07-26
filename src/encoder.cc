#include <v8.h>
#include <node.h>
#include <cstring>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <iostream>
#include <nan.h>

#include "encoder.h"

namespace Encoder {

    using namespace node;
    using namespace v8;
    using namespace std;

    bool IsByte(Local<Value> &value, const char *sig = nullptr) {
        if (value->IsUint32()) {
            int number = value->Int32Value();
            if (number >= 0 && number <= 255) {
                return true;
            }
        }
        return false;
    }

    bool IsBoolean(Local<Value> &value, const char *sig = nullptr) {
        return value->IsTrue() || value->IsFalse() || value->IsBoolean();
    }

    bool IsNullOrUndefined(Local<Value> &value, const char *sig = nullptr) {
        return value->IsNull() || value->IsUndefined();
    }

    bool IsUint32(Local<Value> &value, const char *sig = nullptr) {
        return value->IsUint32();
    }

    bool IsInt32(Local<Value> &value, const char *sig = nullptr) {
        return value->IsInt32();
    }

    bool IsNumber(Local<Value> &value, const char *sig = nullptr) {
        return value->IsNumber();
    }

    bool IsString(Local<Value> &value, const char *sig = nullptr) {
        return value->IsString();
    }

    bool IsArray(Local<Value> &value, const char *sig = nullptr) {
        return value->IsArray();
    }

    bool IsObject(Local<Value> &value, const char *sig = nullptr) {
        return value->IsObject();
    }

    bool HasSameSig(Local<Value> &value, const char *sig = nullptr) {
        return (sig) && (strlen(sig)) &&
               (0 == GetSignatureFromV8Type(value).compare(sig));
    }

    typedef bool (*CheckTypeCallback)(Local<Value> &value, const char *sig);

    bool CheckArrayItems(Local<Array> &array, CheckTypeCallback checkType, const char *sig = nullptr) {
        for (unsigned int i = 0; i < array->Length(); ++i) {
            Local<Value> arrayItem = array->Get(i);
            if (!checkType(arrayItem, sig))
                return false;
        }
        return true;
    }

    string GetSignatureFromV8Type(Local<Value> &value) {
        if (IsNullOrUndefined(value)) {
            return "";
        }
        if (IsBoolean(value)) {
            return const_cast<char *>(DBUS_TYPE_BOOLEAN_AS_STRING);
        }
        if (IsByte(value)) {
            return const_cast<char *>(DBUS_TYPE_BYTE_AS_STRING);
        }
        if (IsUint32(value)) {
            return const_cast<char *>(DBUS_TYPE_UINT32_AS_STRING);
        }
        if (IsInt32(value)) {
            return const_cast<char *>(DBUS_TYPE_INT32_AS_STRING);
        }
        if (IsNumber(value)) {
            return const_cast<char *>(DBUS_TYPE_DOUBLE_AS_STRING);
        }
        if (IsString(value)) {
            return const_cast<char *>(DBUS_TYPE_STRING_AS_STRING);
        }
        if (IsArray(value)) {

            Local<Array> arrayData = Local<Array>::Cast(value);
            size_t arrayDataLength = arrayData->Length();

            if (0 == arrayDataLength) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_VARIANT_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsBoolean)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BOOLEAN_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsByte)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsUint32)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_UINT32_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsInt32)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_INT32_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsNumber)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_DOUBLE_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsString)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsArray)) {

                Local<Value> lastArrayItem = arrayData->Get(static_cast<uint32_t>(arrayDataLength - 1));
                string lastArrayItemSig = GetSignatureFromV8Type(lastArrayItem);

                if (CheckArrayItems(arrayData, HasSameSig, lastArrayItemSig.c_str())) {
                    return string(const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING)).append(lastArrayItemSig);
                }
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_ARRAY_AS_STRING
                                          DBUS_TYPE_VARIANT_AS_STRING);
            }
            if (CheckArrayItems(arrayData, IsObject)) {
                return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_ARRAY_AS_STRING
                                          DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                          DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
                                          DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
            }
            return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_VARIANT_AS_STRING);
        }
        if (IsObject(value)) {
            return const_cast<char *>(DBUS_TYPE_ARRAY_AS_STRING
                                      DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
                                      DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING
                                      DBUS_DICT_ENTRY_END_CHAR_AS_STRING);
        }
        return "";
    }

    bool EncodeObject(Local<Value> value, DBusMessageIter *iter,
                      DBusSignatureIter *siter,
                      DBusSignatureIter *concreteSiter) {

        Nan::HandleScope scope;
        int type;

        if (concreteSiter == nullptr) {
            // this is safe because we never modify siter
            concreteSiter = siter;
        }

        if (IsNullOrUndefined(value)) {
            return false;
        }

        // Get type of current value
        type = dbus_signature_iter_get_current_type(siter);

        if (type != DBUS_TYPE_ARRAY && value->IsArray()) {
            Local<Array> arrayData = Local<Array>::Cast(value);
            for (unsigned int i = 0; i < arrayData->Length(); ++i) {
                Local<Value> arrayItem = arrayData->Get(i);
                if (!EncodeObject(arrayItem, iter, siter, concreteSiter))
                    return false;
                dbus_signature_iter_next(siter);
            }
            return true;
        }

        switch (type) {
            case DBUS_TYPE_INVALID: {
                printf("Invalid type\n");
                break;
            }
            case DBUS_TYPE_BOOLEAN: {
                auto data = static_cast<dbus_bool_t>(value->BooleanValue());

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode boolean value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_BYTE: {
                auto data = static_cast<uint8_t>(value->IntegerValue());

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode numeric value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_INT16: {
                auto data = static_cast<dbus_int16_t>(value->IntegerValue());

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode numeric value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_UINT16: {
                auto data = static_cast<dbus_uint16_t>(value->IntegerValue());

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode numeric value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_INT32: {
                dbus_int32_t data = value->Int32Value();

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode numeric value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_UNIX_FD:
            case DBUS_TYPE_UINT32: {
                dbus_uint32_t data = value->Uint32Value();

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode numeric value\n");
                    return false;
                }

                break;
            }


            case DBUS_TYPE_UINT64: {
                auto data = static_cast<dbus_uint64_t>(value->IntegerValue());

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode numeric value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_DOUBLE: {
                double data = value->NumberValue();

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    printf("Failed to encode double value\n");
                    return false;
                }

                break;
            }

            case DBUS_TYPE_STRING:
            case DBUS_TYPE_OBJECT_PATH:
            case DBUS_TYPE_SIGNATURE: {
                char *data = strdup(*String::Utf8Value(value->ToString()));

                if (!dbus_message_iter_append_basic(iter, type, &data)) {
                    dbus_free(data);
                    printf("Failed to encode string value\n");
                    return false;
                }

                dbus_free(data);

                break;
            }

            case DBUS_TYPE_ARRAY: {
                if (!value->IsObject()) {
                    printf("Failed to encode dictionary\n");
                    return false;
                }

                DBusMessageIter subIter;
                DBusSignatureIter arraySiter, arrayConcreteSiter;
                char *array_sig = nullptr;

                // Getting signature of array object
                dbus_signature_iter_recurse(siter, &arraySiter);
                dbus_signature_iter_recurse(concreteSiter, &arrayConcreteSiter);
                array_sig = dbus_signature_iter_get_signature(&arraySiter);

                // Open array container to process elements in there
                if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, array_sig, &subIter)) {
                    dbus_free(array_sig);
                    printf("Can't open container for Array type\n");
                    return false;
                }
                dbus_free(array_sig);

                // It's a dictionary
                if (dbus_signature_iter_get_element_type(siter) == DBUS_TYPE_DICT_ENTRY) {
                    Local<Object> value_object = value->ToObject();

                    // process each elements
                    Local<Array> prop_names = value_object->GetPropertyNames();
                    unsigned int len = prop_names->Length();

                    bool failed = false;
                    for (unsigned int i = 0; i < len; ++i) {
                        DBusSignatureIter dictSubSiter, dictSubConcreteSiter;
                        DBusMessageIter dict_iter;

                        // Getting the key and value
                        Local<Value> prop_key = prop_names->Get(i);
                        Local<Value> prop_value = value_object->Get(prop_key);

                        if (IsNullOrUndefined(prop_value) ||
                            IsNullOrUndefined(prop_key)) {
                            continue;
                        }

                        // Getting sub-signature object
                        dbus_signature_iter_recurse(&arraySiter, &dictSubSiter);
                        dbus_signature_iter_recurse(&arrayConcreteSiter, &dictSubConcreteSiter);

                        // Open dict entry container
                        if (!dbus_message_iter_open_container(&subIter, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_iter)) {
                            printf("Can't open container for DICT-ENTRY\n");
                            failed = true;
                            break;
                        }

                        // Append the key
                        if (!EncodeObject(prop_key, &dict_iter, &dictSubSiter, &dictSubConcreteSiter)) {
                            dbus_message_iter_close_container(&subIter, &dict_iter);
                            printf("Failed to encode element of dictionary\n");
                            failed = true;
                            break;
                        }

                        // Append the value
                        dbus_signature_iter_next(&dictSubSiter);
                        dbus_signature_iter_next(&dictSubConcreteSiter);
                        if (!EncodeObject(prop_value, &dict_iter, &dictSubSiter, &dictSubConcreteSiter)) {
                            dbus_message_iter_close_container(&subIter, &dict_iter);
                            printf("Failed to encode element of dictionary\n");
                            failed = true;
                            break;
                        }

                        dbus_message_iter_close_container(&subIter, &dict_iter);
                    }

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
                Local<Array> arrayData = Local<Array>::Cast(value);
                for (unsigned int i = 0; i < arrayData->Length(); ++i) {
                    Local<Value> arrayItem = arrayData->Get(i);
                    if (!EncodeObject(arrayItem, &subIter, &arraySiter, &arrayConcreteSiter))
                        break;
                }

                dbus_message_iter_close_container(iter, &subIter);

                break;
            }

            case DBUS_TYPE_VARIANT: {
                DBusMessageIter subIter;
                DBusSignatureIter subSiter;

                char *var_sig;
                if (dbus_signature_iter_get_current_type(concreteSiter) == DBUS_TYPE_VARIANT) {
                    string str_sig = GetSignatureFromV8Type(value);
                    var_sig = strdup(str_sig.c_str());
                } else {
                    var_sig = dbus_signature_iter_get_signature(concreteSiter);
                }

                if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, var_sig, &subIter)) {
                    dbus_free(var_sig);
                    printf("Can't open container for VARIANT type\n");
                    return false;
                }

                dbus_signature_iter_init(&subSiter, var_sig);
                if (!EncodeObject(value, &subIter, &subSiter, &subSiter)) {
                    dbus_message_iter_close_container(iter, &subIter);
                    dbus_free(var_sig);
                    return false;
                }

                dbus_message_iter_close_container(iter, &subIter);
                dbus_free(var_sig);

                break;
            }
            case DBUS_TYPE_STRUCT: {
                DBusMessageIter subIter;
                DBusSignatureIter structSiter, structConcreteSiter;

                Local<Object> value_object = value->ToObject();

                // Open array container to process elements in there
                if (!dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, nullptr, &subIter)) {
                    printf("Can't open container for Struct type\n");
                    return false;
                }

                // Getting sub-signature object
                dbus_signature_iter_recurse(siter, &structSiter);
                dbus_signature_iter_recurse(concreteSiter, &structConcreteSiter);

                // process each elements
                Local<Array> prop_names = value_object->GetPropertyNames();
                unsigned int len = prop_names->Length();

                for (unsigned int i = 0; i < len; ++i) {

                    Local<Value> prop_key = prop_names->Get(i);

                    if (!EncodeObject(value_object->Get(prop_key), &subIter, &structSiter, &structConcreteSiter)) {
                        printf("Failed to encode element of dictionary\n");
                        return false;
                    }

                    if (!dbus_signature_iter_next(&structSiter))
                        break;

                    if (!dbus_signature_iter_next(&structConcreteSiter))
                        break;
                }

                dbus_message_iter_close_container(iter, &subIter);

                break;
            }
            default: {
                printf("Type not implemented: %i\n", type);
            }

        }
        return true;
    }


}

