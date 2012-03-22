var dbus = require("../lib/dbus");

dbus.start( function() {
	console.log('Start');
	session = dbus.session_bus();

	interface = dbus.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");

  //set the signal handler
  interface.HelloSignal.onemit = function (args) {
    console.log("Receive HelloSignal");
  }
  interface.HelloSignal.enabled = true;

  interface.ComplexSignal.onemit = function(args) {
    console.log("Receive ComplexSignal");
    console.log(args);
  }
  interface.ComplexSignal.enabled = true; 

	interface.WhoAmI();
	console.log(interface.WhoAreYou());

	interface.Echo();

  interface.EchoComplex();

  timeout = function(ms, func) {
              return setTimeout(func, ms);
            };
  
  timeout(200000, function() {return console.log("ok, that was 2000 seconds");  });

});
