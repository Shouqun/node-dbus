var DBus = require('../');

var dbus = new DBus();

// Create a new service, object and interface
var service = dbus.registerService('session', 'test.dbus.TestService');
var obj = service.createObject('/test/dbus/TestService');

// Create interface

var iface1 = obj.createInterface('test.dbus.TestService.Interface1');

iface1.addMethod('NoArgs', { out: DBus.Define(String) }, function(callback) {
	callback('result!');
});

iface1.addMethod('Add', { in: [DBus.Define(Number), DBus.Define(Number)], out: DBus.Define(Number) }, function(n1, n2, callback) {
	callback(n1 + n2);
});

iface1.addMethod('LongProcess', { out: DBus.Define(Number) }, function(callback) {
	setTimeout(function() {
		callback(0);
	}, 5000).unref();
});

var author = 'Fred Chien';
iface1.addProperty('Author', {
	type: DBus.Define(String),
	getter: function(callback) {
		callback(author);
	},
	setter: function(value, complete) {
		author = value;

		complete();
	}
});

// Read-only property
var url = 'http://stem.mandice.org';
iface1.addProperty('URL', {
	type: DBus.Define(String),
	getter: function(callback) {
		callback(url);
	}
});

// Signal
var counter = 0;
iface1.addSignal('pump', {
	types: [
		DBus.Define(Number)
	]
});

iface1.update();

// Emit signal per one second
var interval = setInterval(function() {
	counter++;
	iface1.emit('pump', counter);
}, 1000);
interval.unref();

// Create second interface
var iface2 = obj.createInterface('test.dbus.TestService.Interface2');

iface2.addMethod('Hello', { out: DBus.Define(String) }, function(callback) {
	callback('Hello There!');
});

iface2.update();

process.send({ message: 'ready' });
process.on('message', function(msg) {
	if (msg.message === 'done') {
		clearInterval(interval);
		service.disconnect();
		process.removeAllListeners('message');
	}
});
