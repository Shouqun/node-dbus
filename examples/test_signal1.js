var dbus = require("../lib/dbus");

dbus.start( function() {
  
  console.log("start test signal");

  dbus.init();

  session = dbus.session_bus();
  interface = dbus.get_interface(session, "org.designfu.TestService", "/org/designfu/TestService/object", "org.designfu.TestService")
  
  interface.HelloSignal.onemit= function(args) { console.log("xxxx");  console.log(args)}
  interface.HelloSignal.enabled = true;
  
  interface.emitHelloSignal();

  timeout = function(ms, func) {
              return setTimeout(func, ms);
            };
  
  timeout(200000, function() {return console.log("ok, that was 2000 seconds");  });

});

