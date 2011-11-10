dbus = require("../build/default/dbus.node");
session = dbus.session_bus()
interface = dbus.get_interface(session, "org.designfu.TestService", "/org/designfu/TestService/object", "org.designfu.TestService")
interface.HelloSignal.onemit= function(args) { console.log("xxxx");  console.log(args)}
interface.HelloSignal.enabled = true
