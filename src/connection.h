#ifndef CONNECTION_H_
#define CONNECTION_H_

namespace node_dbus {
struct BusObject;
}

namespace Connection {

using namespace node;
using namespace v8;
using namespace std;

void Init(node_dbus::BusObject* bus);
void UnInit(node_dbus::BusObject* bus);
}  // namespace Connection

#endif
