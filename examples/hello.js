var DBus = require('../');

var dbus = new DBus();

var bus = dbus.getBus('system');

bus.getInterface('org.freedesktop.DBus', '/', 'org.freedesktop.DBus', function(err, iface) {
	iface.Hello['timeout'] = 1000;
	iface.Hello['finish'] = function(result) {
		console.log(result);
	};
	iface.Hello();
});
