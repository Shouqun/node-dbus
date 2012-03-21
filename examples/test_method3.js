var dbus = require("../lib/dbus");

var dbusConnection = dbus.createConnection("session", 
                       "com.example.SampleService", 
                       "/SomeObject",
                       "com.example.SampleInterface");

process.nextTick(function() {
  console.log("xxxx");

  console.log(dbusConnection.interface);
  
  var dict = dbusConnection.interface.GetDict();
  console.log(dict);

  console.log(dbusConnection.interface.GetDict());
  
  var struct = dbusConnection.interface.GetTuple();
  console.log(struct);

  var tstr = dbusConnection.interface.GetTwoString();
  console.log(tstr);

  var complex = dbusConnection.interface.GetComplexType();
  console.log(complex);
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
