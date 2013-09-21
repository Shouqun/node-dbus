var DBus = require('../');

var dbus = new DBus();

var bus = dbus.getBus('session');

bus.getInterface('nodejs.dbus.ExampleService', '/nodejs/dbus/ExampleService', 'nodejs.dbus.ExampleService.Interface1', function(err, iface) {

	iface.getProperty('Author', function(value) {
		console.log(value);
	});

	iface.getProperty('JavaScriptOS', function(value) {
		console.log(value);
	});

	iface.getProperty('URL', function(value) {
		console.log(value);
	});

	// Get all properties
	iface.getProperties(function(props) {
		console.log('Properties:');
		console.log(props);
	});

});
