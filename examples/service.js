var DBus = require('../');

var dbus = new DBus();

// Create a new service, object and interface
var service = dbus.registerService('session', 'nodejs.dbus.ExampleService');
var obj = service.createObject('/nodejs/dbus/ExampleService');
var iface = obj.createInterface('nodejs.dbus.ExampleService.Interface1');

// Create methods
iface.addMethod('Hello', null, function(callback) {
	callback('Hello There!');
});
