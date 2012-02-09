var dbus = require("../lib/dbus");

dbus.start( function() {
  console.log('xxx');
  session = dbus.session_bus();
  
  interface = dbus.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");

  interface.WhoAmI();
  }
);
