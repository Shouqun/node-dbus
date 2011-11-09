#ifndef _DBUS_INTROSPECT_H
#define _DBUS_INTROSPECT_H

#include <list>
#include <string>

namespace dbus_library {

struct BusArgument {
  std::string type_;
  std::string direction_;
};

struct BusMethod {
  std::string name_;
  std::string signature_;
  std::list<BusArgument*> args_;
};

struct BusSignal {
  std::string name_;
  std::list<BusArgument*> args_;
};

struct BusInterface{ 
  std::string name_;
  std::list<BusMethod*> methods_;
  std::list<BusSignal*> signals_;
};

struct BusNode {
  std::string name_; 
  std::list<BusInterface*> interfaces_;
};

struct Parser {
  BusNode* root_node;
  BusNode* current_node;
  bool is_on_node;
  BusInterface* current_interface;
  bool is_on_interface;
  BusMethod* current_method;
  bool is_on_method;
  BusSignal* current_signal;
  bool is_on_signal;
  BusArgument* current_argument;
  bool is_on_argument;
};

Parser* ParserNew();
void ParserPrint(Parser *parser);
void ParserRelease(Parser **pparer);

Parser* ParseIntrospcect(const char *source, int size);

BusInterface* ParserGetInterface(Parser *parser, const char *iface);

}

#endif

