var DBus = require('../');

var dbus = new DBus();

var bus = dbus.getBus('session');

bus.getInterface('nodejs.dbus.ExampleService', '/nodejs/dbus/ExampleService', 'nodejs.dbus.ExampleService.Interface1', function(err, iface) {

	iface.SendObject['timeout'] = 1000;
	iface.SendObject['finish'] = function(result) {
		console.log(result);
	};
	iface.SendObject({
		name: 'Fred',
		email: 'cfsghost@gmail.com'
	});

});
