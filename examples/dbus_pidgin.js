var dbus = require('./build/default/dbus.node');

process.nextTick( function () {
  console.log("start pidgin session");

  dbus.init();

  session_dbus = dbus.session_bus();

  intf = dbus.get_interface(session_dbus, 'im.pidgin.purple.PurpleService', '/im/pidgin/purple/PurpleObject', 'im.pidgin.purple.PurpleInterface');

  intf.ReceivedImMsg.onemit = function(args) { console.log("" + args[1] + " said: " + args[2])};
  intf.ReceivedImMsg.enabled = true;

  timeout = function(ms, func) {
              return setTimeout(func, ms);
            };
  
  timeout(2000000, function() {return console.log("ok, that was 2000 seconds");  });
}
);


