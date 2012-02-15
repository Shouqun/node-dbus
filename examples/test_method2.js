var dbus = require("../lib/dbus");

var dbusConnection = dbus.createConnection("session", 
                       "org.freedesktop.DBus.TestSuitePythonService", 
                       "/org/freedesktop/DBus/TestSuitePythonObject",
                       "org.freedesktop.DBus.TestSuiteInterface");

process.nextTick(function() {
  console.log("xxxx");

  console.log(dbusConnection.interface);

  console.log(dbusConnection.interface.WhoAmI());

});

console.log(dbusConnection.interface);

//dbus.start( function() {
//  console.log('xxx');
//  session = dbus.session_bus();
//  
//  interface = dbus.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");
//
//  interface.WhoAmI();
//  }
//);
