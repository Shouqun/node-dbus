var DBus = require('../');

var dbus = new DBus();

// Create a new service, object and interface
var service = dbus.registerService('session', 'nodejs.dbus.ExampleService');
var obj = service.createObject('/nodejs/dbus/ExampleService');

// Create interface

var iface1 = obj.createInterface('nodejs.dbus.ExampleService.Interface1');

iface1.addMethod('Hello', { out: DBus.Define(String) }, function(callback) {
	callback('Hello There!');
});

iface1.addMethod('Ping', { out: DBus.Define(String) }, function(callback) {
	callback('Pong!');
});

iface1.addMethod('Equal', {
	in: [
		DBus.Define(Number),
		DBus.Define(Number)
	],
	out: DBus.Define(Boolean)
}, function(a, b, callback) {

	if (a == b)
		callback(true);
	else
		callback(false);

});

iface1.addMethod('GetNameList', { out: DBus.Define(Array, 'list') }, function(callback) {
	callback([
		'Fred',
		'Stacy',
		'Charles',
		'Rance',
		'Wesley',
		'Frankie'
	]);
});

iface1.addMethod('GetContacts', { out: DBus.Define(Object, 'contacts') }, function(callback) {
	callback({
		Fred: {
			email: 'fred@mandice.com',
			url: 'http://fred-zone.blogspot.com/'
		},
		Stacy: {
			email: 'stacy@mandice.com',
			url: 'http://www.mandice.com/'
		},
		Charles: {
			email: 'charles@mandice.com',
			url: 'http://www.mandice.com/'
		},
		Rance: {
			email: 'lzy@mandice.com',
			url: 'http://www.mandice.com/'
		},
		Wesley: {
			email: 'wesley@mandice.org',
			url: 'http://www.mandice.org/'
		},
		Frankie: {
			email: 'frankie@mandice.com'
		}
	});
});

// Writable property
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

// Read-only property
var jsOS = 'Stem OS';
iface1.addProperty('JavaScriptOS', {
	type: DBus.Define(String),
	getter: function(callback) {
		callback(jsOS);
	}
});

iface1.update();

// Create second interface
var iface2 = obj.createInterface('nodejs.dbus.ExampleService.Interface2');

iface2.addMethod('Hello', { out: DBus.Define(String) }, function(callback) {
	callback('Hello There!');
});

iface2.update();

