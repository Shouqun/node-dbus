var dbus = require("../lib/dbus");

process.nextTick( function() {
  
  console.log("start test signal");

  dbus.init();

  session = dbus.session_bus();
  interface = dbus.get_interface(session, "com.example.TestService", "/com/example/TestService/object", "com.example.TestService")
  interface.HelloSignal.onemit= function(args) { console.log("Receive Signal");  console.log(args)}
  interface.HelloSignal.enabled = true;

  timeout = function(ms, func) {
              return setTimeout(func, ms);
            };
  
  timeout(2000000, function() {return console.log("ok, that was 2000 seconds");  });

});

