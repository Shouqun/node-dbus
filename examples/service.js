var DBus = require('../');

var dbus = new DBus();

// Create a new service, object and interface
var service = dbus.registerService('session', 'nodejs.dbus.ExampleService');
var obj = service.createObject('/nodejs/dbus/ExampleService');

// Create interface
var iface1 = obj.createInterface('nodejs.dbus.ExampleService.Interface1');

iface1.addMethod('Hello', null, function(callback) {
	callback('Hello There!');
});

iface1.update();

// Create second interface
var iface2 = obj.createInterface('nodejs.dbus.ExampleService.Interface2');

iface2.addMethod('Hello', null, function(callback) {
	callback('Hello There!');
});

iface2.update();
