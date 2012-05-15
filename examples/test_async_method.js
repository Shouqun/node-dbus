var dbus = require("../lib/dbus");

dbus.start( function() {
  console.log('Start async call');
  session = dbus.session_bus();
  
  interface = dbus.get_interface(session, "com.example.SampleService", "/SomeObject", "com.example.SampleInterface");
  
  interface.HelloWorld['finish'] = function (args){ console.log(args) };
  //interface.HelloWorld['error'] = function (args){ console.log(args) };
  result = interface.HelloWorld('Hello world form async call');
  console.log(result);


  timeout = function(ms, func) {
              return setTimeout(func, ms);
            };
  
  timeout(2000000, function() {return console.log("ok, that was 2000 seconds");  });
  }
);
