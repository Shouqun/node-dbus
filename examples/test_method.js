var dbus = require("../lib/dbus");

process.nextTick( function() {

  console.log("start test method call");
  
  dbus.init();

  session = dbus.session_bus();
  
  interface = dbus.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");

  interface.WhoAmI();

  timeout = function(ms, func) {
              return setTimeout(func, ms);
            };
  
  timeout(2000000, function() {return console.log("ok, that was 2000 seconds");  });


});

