#include <v8.h>
#include <node.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string>
#include <list>
#include <map>
#include <iostream>

#include "common.h"
#include "dbus.h"
#include "dbus_introspect.h"

using namespace v8;
using namespace dbus_library;

class DBusMethodContainer {
public:
  DBusGConnection *connection;
  std::string  service;
  std::string  path;
  std::string  interface;
  std::string  method;
  std::string  signature;
};

class DBusSignalContainer {
public:
  DBusGConnection *connection;
  std::string service;
  std::string path;
  std::string interface;
  std::string signal;
  std::string match;
};

static DBusGConnection* GetBusFromType(DBusBusType type) {
  DBusGConnection *connection;
  GError *error;

  error = NULL;
  connection = NULL;
  connection = dbus_g_bus_get(type, &error);
  if (connection == NULL)
  {
    ERROR("Failed to open connection to bus: %s\n",error->message);
    g_error_free (error);
    return connection;
  }
  return connection;
}

static Persistent<ObjectTemplate> g_connetion_template_;
static bool g_is_signal_filter_attached_ = false;

//the signal map stores v8 Persistent object by the signal string
typedef std::map<std::string, Handle<Object> > SignalMap;
SignalMap g_signal_object_map;

std::string GetSignalMatchRule(DBusSignalContainer *container) {
  std::string result;
  
  result = container->interface + "." + container->signal;
  
  return result;
}

std::string GetSignalMatchRuleByString(std::string interface,
                                       std::string signal) {
  std::string result;
  
  result = interface + "." + signal;
  
  return result;
}

Handle<Value> GetSignalObject(DBusSignalContainer *signal) {
  std::string match_rule = GetSignalMatchRule(signal);
 
  SignalMap::iterator ite = g_signal_object_map.find(match_rule);
  if (ite == g_signal_object_map.end() ) {
    return Undefined();
  }
   
  return ite->second;
}

Handle<Value> GetSignalObjectByString(std::string match_rule) {
  //std::cout<<"GetSignalObjectByString:"<<match_rule; 
  SignalMap::iterator ite = g_signal_object_map.find(match_rule);
  if (ite == g_signal_object_map.end() ) {
    return Undefined();
  }
  
  //std::cout<<"Find the match"; 
  return ite->second;
}

void AddSignalObject(DBusSignalContainer *signal,
                           Handle<Object> signal_obj) {
  std::string match_rule = GetSignalMatchRule(signal);
  
  SignalMap::iterator ite = g_signal_object_map.find(match_rule);
  if (ite == g_signal_object_map.end() ) {
    LOG("We are to add the signal object\n");
    g_signal_object_map.insert( make_pair(match_rule,
          signal_obj));
  }
}

void RemoveSignalObject(DBusSignalContainer *signal) {
  std::string match_rule = GetSignalMatchRule(signal);
  // remove the matching item from signal object map
  LOG("We are going to remove the object map item");
  g_signal_object_map.erase(match_rule);
}


/// dbus_signal_weak_callback: MakeWeak callback for signal Persistent
///    object,
static void dbus_signal_weak_callback(Persistent<Value> value, 
                                      void* parameter) {
  LOG("dbus_signal_weak_callback\n");

  DBusSignalContainer *container = (DBusSignalContainer*) parameter;
  if (container != NULL) {
    //Remove the matching objet map item from the map
    RemoveSignalObject(container); 
    delete container;
  }

  value.Dispose();
  value.Clear();
}                                       



/// dbus_method_weak_callback: MakeWeak callback for method Persistent
///   obect
static void dbus_method_weak_callback(Persistent<Value> value,
                                      void* parameter) {
  LOG("dbus_method_weak_callback");
  DBusMethodContainer *container = (DBusMethodContainer*) parameter;
  if (container != NULL)
    delete container;

  value.Dispose();
  value.Clear();
}


/// dbus_messages_iter_size: get the size of DBusMessageIter
///   iterate the iter from the begin to end and count the size
static int dbus_messages_iter_size(DBusMessageIter *iter) {
  int msg_count = 0;
  
  while( dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_INVALID) {
    msg_count++;
    dbus_message_iter_next(iter);
  }
  return msg_count;
}


/// dbus_message_size: get the size of DBusMessage struct
///   use DBusMessageIter to iterate on the message and count the 
///   size of the message
static int dbus_messages_size(DBusMessage *message) {
  DBusMessageIter iter;
  int msg_count = 0;
  
  dbus_message_iter_init(message, &iter);

  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
    const char *error_name = dbus_message_get_error_name(message);
    if (error_name != NULL) {
      ERROR("Error message:%s\n ",error_name);
    }
    return 0;
  }

  while ((dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
    msg_count++;
    //go the next
    dbus_message_iter_next (&iter);
  }

  return msg_count; 
}



/// decode_reply_message_by_iter: decode each message to v8 Value/Object
///   check the type of "iter", and then create the v8 Value/Object 
///   according to the type
static Handle<Value> decode_reply_message_by_iter(
                                                DBusMessageIter *iter) {

  switch (dbus_message_iter_get_arg_type(iter)) {
    case DBUS_TYPE_BOOLEAN: {
      dbus_bool_t value = false;
      dbus_message_iter_get_basic(iter, &value);
      return Boolean::New(value);
      break;
    }
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64: {
      dbus_uint64_t value = 0; 
      dbus_message_iter_get_basic(iter, &value);
      return Integer::New(value);
      break;
    }
    case DBUS_TYPE_DOUBLE: {
      double value = 0;
      dbus_message_iter_get_basic(iter, &value);
      return Number::New(value);
      break;
    }
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE:
    case DBUS_TYPE_STRING: {
      const char *value;
      dbus_message_iter_get_basic(iter, &value); 
      return String::New(value);
      break;
    }
    case DBUS_TYPE_ARRAY:
    case DBUS_TYPE_STRUCT: {
      DBusMessageIter internal_iter, internal_temp_iter;
      int count = 0;         
     
      //count the size of the array
      dbus_message_iter_recurse(iter, &internal_temp_iter);
      count = dbus_messages_iter_size(&internal_temp_iter);
      
      //create the result object
      Local<Array> resultArray = Array::New(count);
      count = 0;
      dbus_message_iter_recurse(iter, &internal_iter);

      do {
        LOG("for each item\n");
        //this is dict entry
        if (dbus_message_iter_get_arg_type(&internal_iter) 
                      == DBUS_TYPE_DICT_ENTRY) {
          //Item is dict entry, it is exactly key-value pair
          LOG(" DBUS_TYPE_DICT_ENTRY\n");
          DBusMessageIter dict_entry_iter;
          //The key 
          dbus_message_iter_recurse(&internal_iter, &dict_entry_iter);
          Handle<Value> key 
                        = decode_reply_message_by_iter(&dict_entry_iter);
          //The value
          dbus_message_iter_next(&dict_entry_iter);
          Handle<Value> value 
                        = decode_reply_message_by_iter(&dict_entry_iter);
          
          //set the property
          resultArray->Set(key, value); 
        } else {
          //Item is array
          Handle<Value> itemValue 
                  = decode_reply_message_by_iter(&internal_iter);
          resultArray->Set(count, itemValue);
          count++;
        }
      } while(dbus_message_iter_next(&internal_iter));
      //return the array object
      return resultArray;
    }
    case DBUS_TYPE_VARIANT: {
      LOG("DBUS_TYPE_VARIANT\n");
      DBusMessageIter internal_iter;
      dbus_message_iter_recurse(iter, &internal_iter);
      
      Handle<Value> result 
                = decode_reply_message_by_iter(&internal_iter);
      return result;
    }
    case DBUS_TYPE_DICT_ENTRY: {
      ERROR("DBUS_TYPE_DICT_ENTRY: should Never be here.\n");
    }
    case DBUS_TYPE_INVALID: {
      LOG("DBUS_TYPE_INVALID\n");
    } 
    default: {
      //should return 'undefined' object
      return Undefined();
    }      
  } //end of swith
  //not a valid type, return undefined value
  return Undefined();
}


static char* get_signature_from_v8_type(Local<Value>& value) {
  //guess the type from the v8 Object
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
    return const_cast<char*>(DBUS_TYPE_ARRAY_AS_STRING);
  } else {
    return NULL;
  }
}


/// Encode the given argument from JavaScript into D-Bus message
///  append the data to DBusMessage according to the type and value
static bool encode_to_message_with_objects(Local<Value> value, 
                                           DBusMessageIter *iter, 
                                           char* signature) {
  DBusSignatureIter siter;
  int type;
  dbus_signature_iter_init(&siter, signature);
  
  switch ((type=dbus_signature_iter_get_current_type(&siter))) {
    //the Boolean value type 
    case DBUS_TYPE_BOOLEAN:  {
      dbus_bool_t data = value->BooleanValue();  
      if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &data)) {
        ERROR("Error append boolean\n");
        return false;
      }
      break;
    }
    //the Numeric value type
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_BYTE: {
      dbus_uint64_t data = value->IntegerValue();
      if (!dbus_message_iter_append_basic(iter, type, &data)) {
        ERROR("Error append numeric\n");
        return false;
      }
      break; 
    }
    case DBUS_TYPE_STRING: 
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE: {
      String::Utf8Value data_val(value->ToString());
      char *data = *data_val;
      if (!dbus_message_iter_append_basic(iter, type, &data)) {
        ERROR("Error append string\n");
        return false;
      }
      break;
    }
    case DBUS_TYPE_DOUBLE: {
      double data = value->NumberValue();
      if (!dbus_message_iter_append_basic(iter, type, &data)) {
        ERROR("Error append double\n");
        return false;
      }
      break;
    } 
    case DBUS_TYPE_ARRAY: {

      if (dbus_signature_iter_get_element_type(&siter) 
                                    == DBUS_TYPE_DICT_ENTRY) {
        //This element is a DICT type of D-Bus
        if (! value->IsObject()) {
          ERROR("Error, not a Object type for DICT_ENTRY\n");
          return false;
        }
        Local<Object> value_object = value->ToObject();
        DBusMessageIter subIter;
        DBusSignatureIter dictSiter, dictSubSiter;
        char *dict_sig;

        dbus_signature_iter_recurse(&siter, &dictSiter);
        dict_sig = dbus_signature_iter_get_signature(&dictSiter);

        if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                dict_sig, &subIter)) {
          ERROR("Can't open container for ARRAY-Dict type\n");
          dbus_free(dict_sig); 
          return false; 
        }
        dbus_free(dict_sig);
        
        Local<Array> prop_names = value_object->GetPropertyNames();
        int len = prop_names->Length();

        //for each signature
        dbus_signature_iter_recurse(&dictSiter, &dictSubSiter); //the key
        dbus_signature_iter_next(&dictSubSiter); //the value
        
        bool no_error_status = true;
        for(int i=0; i<len; i++) {
          DBusMessageIter dict_iter;

          if (!dbus_message_iter_open_container(&subIter, 
                                DBUS_TYPE_DICT_ENTRY,
                                NULL, &dict_iter)) {
            ERROR("Can't open container for DICT-ENTTY\n");
            return false;
          }

          Local<Value> prop_name = prop_names->Get(i);
          //TODO: we currently only support 'string' type as dict key type
          //      for other type as arugment, we are currently not support
          String::Utf8Value prop_name_val(prop_name->ToString());
          char *prop_str = *prop_name_val;
          //append the key
          dbus_message_iter_append_basic(&dict_iter, 
                                      DBUS_TYPE_STRING, &prop_str);
          
          //append the value 
          char *cstr = dbus_signature_iter_get_signature(&dictSubSiter);
          if ( ! encode_to_message_with_objects(
                    value_object->Get(prop_name), &dict_iter, cstr)) {
            no_error_status = false;
          }

          //release resource
          dbus_free(cstr);
          dbus_message_iter_close_container(&subIter, &dict_iter); 
          //error on encode message, break and return
          if (!no_error_status) break;
        }
        dbus_message_iter_close_container(iter, &subIter);
        //Check errors on message
        if (!no_error_status) 
          return no_error_status;
      } else {
        //This element is a Array type of D-Bus 
        if (! value->IsArray()) {
          ERROR("Error!, not a Array type for array argument");
          return false;
        }
        DBusMessageIter subIter;
        DBusSignatureIter arraySIter;
        char *array_sig = NULL;
      
        dbus_signature_iter_recurse(&siter, &arraySIter);
        array_sig = dbus_signature_iter_get_signature(&arraySIter);
        LOG("Array Signature: %s\n", array_sig); 
      
        if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, 
                                              array_sig, &subIter)) {
          ERROR("Can't open container for ARRAY type\n");
          g_free(array_sig); 
          return false; 
        }
        
        Local<Array> arrayData = Local<Array>::Cast(value);
        bool no_error_status = true;
        for (unsigned int i=0; i < arrayData->Length(); i++) {
          ERROR("Argument Arrary Item:%d\n", i);
          Local<Value> arrayItem = arrayData->Get(i);
          if ( encode_to_message_with_objects(arrayItem, 
                                          &subIter, array_sig) ) {
            no_error_status = false;
            break;
          }
        }
        dbus_message_iter_close_container(iter, &subIter);
        g_free(array_sig);
        return no_error_status;
      }
      break;
    }
    case DBUS_TYPE_VARIANT: {
      LOG("DBUS_TYPE_VARIANT\n");
      DBusMessageIter sub_iter;
      DBusSignatureIter var_siter;
       //FIXME: the variable stub
      char *var_sig = get_signature_from_v8_type(value);
      
      LOG("Guess the variable type is: %s\n", var_sig);

      dbus_signature_iter_recurse(&siter, &var_siter);
      
      if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, 
                              var_sig, &sub_iter)) {
        ERROR("Can't open contianer for VARIANT type\n");
        return false;
      }
      
      //encode the object to dbus message 
      if (!encode_to_message_with_objects(value, &sub_iter, var_sig)) { 
        dbus_message_iter_close_container(iter, &sub_iter);
        return false;
      }
      dbus_message_iter_close_container(iter, &sub_iter);

      break;
    }
    case DBUS_TYPE_STRUCT: {
      LOG("DBUS_TYPE_STRUCT");
      DBusMessageIter sub_iter;
      DBusSignatureIter struct_siter;

      if (!dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, 
                              NULL, &sub_iter)) {
        ERROR("Can't open contianer for STRUCT type\n");
        return false;
      }
      
      Local<Object> value_object = value->ToObject();

      Local<Array> prop_names = value_object->GetPropertyNames();
      int len = prop_names->Length(); 
      bool no_error_status = true;

      dbus_signature_iter_recurse(&siter, &struct_siter);
      for(int i=0 ; i<len; i++) {
        char *sig = dbus_signature_iter_get_signature(&struct_siter);
        Local<Value> prop_name = prop_names->Get(i);

        if (!encode_to_message_with_objects(value_object->Get(prop_name), 
                                          &sub_iter, sig) ) {
          no_error_status = false;
        }
        
        dbus_free(sig);
        
        if (!dbus_signature_iter_next(&struct_siter) || !no_error_status) {
          break;
        }
      }
      dbus_message_iter_close_container(iter, &sub_iter);
      return no_error_status;
    }
    default: {
      ERROR("Error! Try to append Unsupported type\n");
      return false;
    }
  }
  return true; 
}

/// decode_reply_messages: Decode the d-bus reply message to v8 Object
///  it iterate on the DBusMessage struct and wrap elements into an array
///  of v8 Object. the return type is a Array object, from the js
///  viewport, the type is Array([])
static Handle<Value> decode_reply_messages(DBusMessage *message) {
  DBusMessageIter iter;
  int type;
  int argument_count = 0;
  int count;

  if ((count=dbus_messages_size(message)) <=0 ) {
    return Undefined();
  }     

  Local<Array> resultArray = Array::New(count);
  dbus_message_iter_init(message, &iter);

  //handle error
  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
    const char *error_name = dbus_message_get_error_name(message);
    if (error_name != NULL) {
      ERROR("Error message: %s\n ",error_name);
    }
  }

  while ((type=dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
    Handle<Value> valueItem = decode_reply_message_by_iter(&iter);
    resultArray->Set(argument_count, valueItem);

    //for next message
    dbus_message_iter_next (&iter);
    argument_count++;
  } //end of while loop
  
  return resultArray; 
}

Handle<Value> DBusMethod(const Arguments& args){
  
  HandleScope scope;
  Local<Value> this_data = args.Data();
  void *data = External::Unwrap(this_data);

  DBusMethodContainer *container= (DBusMethodContainer*) data;
  LOG("Calling method: %s\n", container->method); 
  
  bool no_error_status = true;
  DBusMessage *message = dbus_message_new_method_call (
                               container->service.c_str(), 
                               container->path.c_str(),
                               container->interface.c_str(),
                               container->method.c_str() );
  //prepare the method arguments message if needed
  if (args.Length() >0) {
    //prepare for the arguments
    const char *signature = container->signature.c_str();
    DBusMessageIter iter;
    int count = 0;
    DBusSignatureIter siter;
    DBusError error;

    dbus_message_iter_init_append(message, &iter); 

    dbus_error_init(&error);        
    if (!dbus_signature_validate(signature, &error)) {
      ERROR("Invalid signature: %s\n",error.message);
    }
    
    dbus_signature_iter_init(&siter, signature);
    do {
      char *arg_sig = dbus_signature_iter_get_signature(&siter);
      //process the argument sig
      if (count >= args.Length()) {
        ERROR("Arguments Not Enough\n");
        break;
      }
      
      //encode to message with given v8 Objects and the signature
      if (! encode_to_message_with_objects(args[count], &iter, arg_sig)) {
        dbus_free(arg_sig);
        no_error_status = false; 
      }

      dbus_free(arg_sig);  
      count++;
    } while (dbus_signature_iter_next(&siter));
  }
  
  //check if there is error on encode dbus message
  if (!no_error_status) {
    if (message != NULL)
      dbus_message_unref(message);
    return Undefined();
  }

  //call the dbus method and get the returned message, and decode to 
  //target v8 object
  DBusMessage *reply_message;
  DBusError error;    
  Handle<Value> return_value = Undefined();

  dbus_error_init(&error); 
  //send message and call sync dbus_method
  reply_message = dbus_connection_send_with_reply_and_block(
          dbus_g_connection_get_connection(container->connection),
          message, -1, &error);
  if (reply_message != NULL) {
    if (dbus_message_get_type(reply_message) == DBUS_MESSAGE_TYPE_ERROR) {
      ERROR("Error reply message\n");

    } else if (dbus_message_get_type(reply_message) 
                  == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
      //method call return ok, decoe the messge to v8 Value 
      return_value = decode_reply_messages(reply_message);

    } else {
      ERROR("Unkonwn reply\n");
    }
    //free the reply message of dbus call
    dbus_message_unref(reply_message);
  } else {
      ERROR("Error calling sync method:%s\n",error.message);
      dbus_error_free(&error);
  }
 
  //free the input dbus message if needed
  if (message != NULL) {
    dbus_message_unref(message);
  } 

  return return_value;
}


/// dbus_signal_filter: the static message filter of dbus messages
///   this filter receives all dbus signal messages and then find the 
///   corressponding signal handler from the global hash map, and 
///   then call the signal handler callback with the arguments from the
///   dbus message 
static DBusHandlerResult dbus_signal_filter(DBusConnection* connection,
                                            DBusMessage* message,
                                            void *user_data) {
  if (message == NULL || 
        dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
    ERROR("Not a valid signal\n");
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  //get the interface name and signal name
  const char *interface_str = dbus_message_get_interface(message);
  const char *signal_name_str = dbus_message_get_member(message);
  if (interface_str == NULL || signal_name_str == NULL ) {
    ERROR("Not valid signal parameter\n");
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }  

  std::string interface = dbus_message_get_interface(message);
  std::string signal_name = dbus_message_get_member(message);

  //std::cout<<"Interface"<<interface<<"  "<<signal_name;
  //get the signal matching rule
  std::string match = GetSignalMatchRuleByString(interface, signal_name);
 
  //get the signal handler object
  Handle<Value> value = GetSignalObjectByString(match);
  if (value == Undefined()) {
    //std::cout<<"No Matching Rule";
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } 

  //create the execution context since its in new context
  Persistent<Context> context = Context::New();
  Context::Scope ctxScope(context); 
  HandleScope scope;
  TryCatch try_catch;

  //get the enabled property and the onemit callback
  Handle<Object> object = value->ToObject();
  Local<Value> callback_enabled 
                          = object->Get(String::New("enabled"));
  Local<Value> callback_v 
                          = object->Get(String::New("onemit"));

  
  if ( callback_enabled == Undefined() 
                      || callback_v == Undefined()) {
    ERROR("Callback undefined\n");
    context.Dispose();
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  if (! callback_enabled->ToBoolean()->Value()) {
    ERROR("Callback not enabled\n");
    context.Dispose();
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  if (! callback_v->IsFunction()) {
    ERROR("The callback is not a Function\n");
    context.Dispose();
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  Local<Function> callback 
                          = Local<Function>::Cast(callback_v);

  //Decode reply message as argument
  Handle<Value> args[1];
  Handle<Value> arg0 = decode_reply_messages(message);
  args[0] = arg0; 

  //Do call the callback
  callback->Call(callback, 1, args);

  if (try_catch.HasCaught()) {
    ERROR("Ooops, Exception on call the callback\n");
  } 
  
  context.Dispose();
  return DBUS_HANDLER_RESULT_HANDLED;
}


Handle<Value> GetSignal(Local<Object>& interface_object,
                                         DBusGConnection *connection,
                                         const char *service_name,
                                         const char *object_path,
                                         const char *interface_name,
                                         BusSignal *signal) {
  HandleScope scope;

  if (!g_is_signal_filter_attached_ ) {
    ERROR("attach signal filter\n");
    dbus_connection_add_filter(dbus_g_connection_get_connection(connection),
                             dbus_signal_filter, NULL, NULL);
    g_is_signal_filter_attached_ = true;
  }

  DBusError error;
  dbus_error_init (&error); 
  dbus_bus_add_match ( dbus_g_connection_get_connection(connection),
          "type='signal'",
          &error);
  if (dbus_error_is_set (&error)) {
    ERROR("Error Add match: %s\n",error.message);
  }

  //create the object
  Local<ObjectTemplate> signal_obj_temp = ObjectTemplate::New();
  signal_obj_temp->SetInternalFieldCount(1);
  
  Persistent<Object> signal_obj 
      = Persistent<Object>::New(signal_obj_temp->NewInstance());
  signal_obj->Set(String::New("onemit"), Undefined());
  signal_obj->Set(String::New("enabled"), Boolean::New(false));
   
  DBusSignalContainer *container = new DBusSignalContainer;
  container->service= service_name;
  container->path = object_path;
  container->interface = interface_name;
  container->signal = signal->name_; 

  AddSignalObject(container, signal_obj); 
  signal_obj->SetInternalField(0, External::New(container));
  
  //make the signal handle weak and set the callback
  signal_obj.MakeWeak(container, dbus_signal_weak_callback);


  interface_object->Set(String::New(signal->name_.c_str()),
                          signal_obj);

  return Undefined();
}

Handle<Value> GetMethod(Local<Object>& interface_object,
                          DBusGConnection *connection,
                          const char *service_name,
                          const char *object_path,
                          const char *interface_name,
                          BusMethod *method) {
  HandleScope scope;
  
  DBusMethodContainer *container = new DBusMethodContainer;
  container->connection = connection;
  container->service = service_name;
  container->path = object_path;
  container->interface = interface_name;
  container->method = method->name_;
  container->signature = method->signature_;
  

  //store the private data (container) with the FunctionTempalte
  Local<FunctionTemplate> func_template 
        = FunctionTemplate::New(DBusMethod, 
                                    External::New((void*)container));  
  Local<Function> func_obj = func_template->GetFunction();
  Persistent<Function> p_func_obj = Persistent<Function>::New(func_obj);
  
  //MakeWeak for GC
  p_func_obj.MakeWeak(container, dbus_method_weak_callback);

  interface_object->Set(String::New(container->method.c_str()), 
                        p_func_obj);

  return Undefined();
}


Handle<Value> SystemBus(const Arguments& args) {
  HandleScope scope;
  
  if (g_connetion_template_.IsEmpty()) {
    Handle<ObjectTemplate> object_template = ObjectTemplate::New();
    object_template->SetInternalFieldCount(1);
    g_connetion_template_ = Persistent<ObjectTemplate>::New(object_template);
  }

  DBusGConnection *connection = GetBusFromType(DBUS_BUS_SYSTEM);
  if (connection == NULL ) {
    return ThrowException(Exception::Error(String::New("error system bus object")));
  }

  Local<Object> connection_object = g_connetion_template_->NewInstance();
  connection_object->SetInternalField(0, External::New(connection));

  return scope.Close(connection_object);
}

Handle<Value> SessionBus(const Arguments& args) {
  HandleScope scope;
  
  if (g_connetion_template_.IsEmpty()) {
    Handle<ObjectTemplate> object_template = ObjectTemplate::New();
    object_template->SetInternalFieldCount(1);
    g_connetion_template_ = Persistent<ObjectTemplate>::New(object_template);
  }

  DBusGConnection *connection = GetBusFromType(DBUS_BUS_SESSION);
  if (connection == NULL ) {
    return ThrowException(Exception::Error(String::New("error system bus object")));
  }

  Local<Object> connection_object = g_connetion_template_->NewInstance();
  connection_object->SetInternalField(0, External::New(connection));

  return scope.Close(connection_object);
}

Handle<Value> GetInterface(const Arguments& args) {
  HandleScope scope;
 
  if (args.Length()<4) {
    return ThrowException(Exception::Error(String::New("invalid parameters")));
  }

  Local<Object> bus_object = args[0]->ToObject();
  String::Utf8Value service_name(args[1]->ToString());
  String::Utf8Value object_path(args[2]->ToString());
  String::Utf8Value interface_name(args[3]->ToString());

  DBusGConnection *connection = (DBusGConnection*) External::Unwrap(bus_object->GetInternalField(0));

  GError *error;
  DBusGProxy *proxy;
  char *iface_data;

  proxy = dbus_g_proxy_new_for_name(connection, *service_name, *object_path, 
                "org.freedesktop.DBus.Introspectable");

  error = NULL;
  if ( !dbus_g_proxy_call(proxy, "Introspect", &error, G_TYPE_INVALID, G_TYPE_STRING,
            &iface_data, G_TYPE_INVALID) ) {
    if ( error->domain == DBUS_GERROR
          && error->code == DBUS_GERROR_REMOTE_EXCEPTION ) {
      g_error_free(error);
      return ThrowException(Exception::Error(String::New("remote method proxy call exception")));  
    } else {
      g_error_free(error);
      return ThrowException(Exception::Error(String::New("error on method proxy call")));
    }
  }

  std::string introspect_data(iface_data);
  g_free(iface_data);

  //Parse the Introspect data
  Parser *parser = ParseIntrospcect(introspect_data.c_str(), introspect_data.length());
  
  if (parser == NULL)
    return ThrowException(Exception::Error(String::New("Error parse introspect")));

  BusInterface *interface = ParserGetInterface(parser, *interface_name);
  if (interface == NULL) {
    ParserRelease(&parser);
    return ThrowException(Exception::Error(String::New("Error get interface")));
  }

   //create the Interface object to return
  Local<Object> interface_object = Object::New();
  interface_object->Set(String::New("xml_source"), String::New(iface_data));

  //get all methods
  std::list<BusMethod*>::iterator method_ite;
  for( method_ite = interface->methods_.begin();
        method_ite != interface->methods_.end();
        method_ite++) {
    BusMethod *method = *method_ite;
    //get method object    
    GetMethod(interface_object, connection, *service_name, *object_path,
               *interface_name, method);
  }

  //get all interface
  std::list<BusSignal*>::iterator signal_ite;
  for( signal_ite = interface->signals_.begin();
         signal_ite != interface->signals_.end();
         signal_ite++ ) {
    BusSignal *signal = *signal_ite;
    //get siangl object
    GetSignal(interface_object, connection, *service_name,
              *object_path, *interface_name, signal);
  }

  ParserRelease(&parser);

  return scope.Close(interface_object); 
}

//BusInit:  this MUST be called after process.nextTick to ensure 
//  it would not block. 
Handle<Value> BusInit(const Arguments& args)
{
  g_type_init();

  return Undefined();
}

/// Add glib evene loop to libev
struct econtext {
  GPollFD *pfd;
  ev_io *iow;
  int nfd, afd;
  gint maxpri;

  ev_prepare pw;
  ev_check cw;
  ev_timer tw;

  GMainContext *gc;
};

static void timer_cb (EV_P_ ev_timer *w, int revents) {
  /* nop */
}

static void io_cb (EV_P_ ev_io *w, int revents) {
  /* nop */
}

static void prepare_cb (EV_P_ ev_prepare *w, int revents) {
  struct econtext *ctx = (struct econtext *)(((char *)w) - offsetof (struct econtext, pw));
  gint timeout;
  int i;
  
  g_main_context_dispatch (ctx->gc);

  g_main_context_prepare (ctx->gc, &ctx->maxpri);

  while (ctx->afd < (ctx->nfd = g_main_context_query  (
          ctx->gc,
          ctx->maxpri,
          &timeout,
          ctx->pfd,
          ctx->afd))
      )
  {
    free (ctx->pfd);
    free (ctx->iow);

    ctx->afd = 1;
    while (ctx->afd < ctx->nfd)
      ctx->afd <<= 1;

    ctx->pfd = (GPollFD*) malloc (ctx->afd * sizeof (GPollFD));
    ctx->iow = (ev_io*) malloc (ctx->afd * sizeof (ev_io));
  }

  for (i = 0; i < ctx->nfd; ++i)
  {
    GPollFD *pfd = ctx->pfd + i;
    ev_io *iow = ctx->iow + i;

    pfd->revents = 0;

    ev_io_init (
        iow,
        io_cb,
        pfd->fd,
        (pfd->events & G_IO_IN ? EV_READ : 0)
        | (pfd->events & G_IO_OUT ? EV_WRITE : 0)
        );
    iow->data = (void *)pfd;
    ev_set_priority (iow, EV_MINPRI);
    ev_io_start (EV_A_ iow);
  }

  if (timeout >= 0)
  {
    ev_timer_set (&ctx->tw, timeout * 1e-3, 0.);
    ev_timer_start (EV_A_ &ctx->tw);
  }
}

static void check_cb (EV_P_ ev_check *w, int revents) {
  struct econtext *ctx = (struct econtext *)(((char *)w) - offsetof (struct econtext, cw));
  int i;

  for (i = 0; i < ctx->nfd; ++i)
  {
    ev_io *iow = ctx->iow + i;

    if (ev_is_pending (iow))
    {
      GPollFD *pfd = ctx->pfd + i;
      int revents = ev_clear_pending (EV_A_ iow);

      pfd->revents |= pfd->events &
        ((revents & EV_READ ? G_IO_IN : 0)
         | (revents & EV_WRITE ? G_IO_OUT : 0));
    }

    ev_io_stop (EV_A_ iow);
  }

  if (ev_is_active (&ctx->tw))
    ev_timer_stop (EV_A_ &ctx->tw);

  //FIXME: would block here when run in non-interactive mode, because "prepare_cb" not called
  //oops, event loop  block here, the "prepare_cb" is never called and so blocks here
  // in the interactive node, this never blocks.  but launch with "node test_method.js"
  // blocks here on "read" inside g_main_context_check
  g_main_context_check (ctx->gc, ctx->maxpri, ctx->pfd, ctx->nfd);
}

static struct econtext default_context;

extern "C" void
init (Handle<Object> target)
{
  HandleScope scope;
  target->Set(String::New("hello"), String::New("world"));

  NODE_SET_METHOD(target , "init", BusInit);
  NODE_SET_METHOD(target, "system_bus", SystemBus);
  NODE_SET_METHOD(target, "session_bus", SessionBus);
  NODE_SET_METHOD(target, "get_interface", GetInterface);

  //add glib event loop to libev main loop in node
  GMainContext *gc     = g_main_context_default();
  struct econtext *ctx = &default_context;

  ctx->gc  = g_main_context_ref(gc);
  ctx->nfd = 0;
  ctx->afd = 0;
  ctx->iow = 0;
  ctx->pfd = 0;

  ev_prepare_init (&ctx->pw, prepare_cb);
  ev_set_priority (&ctx->pw, EV_MINPRI);
  ev_prepare_start (EV_DEFAULT_UC_ &ctx->pw);
  ev_unref(EV_DEFAULT_UC);

  ev_check_init (&ctx->cw, check_cb);
  ev_set_priority (&ctx->cw, EV_MAXPRI);
  ev_check_start (EV_DEFAULT_UC_ &ctx->cw);
  ev_unref(EV_DEFAULT_UC);

  ev_init (&ctx->tw, timer_cb);
  ev_set_priority (&ctx->tw, EV_MINPRI);
}
