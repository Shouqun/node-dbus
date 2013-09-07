"usr strict";

var util = require('util');
var events = require('events');
var Interface = require('./interface');

var Bus = module.exports = function(dbus, busName) {
	var self = this;

	self.dbus = dbus;
	self.type = busName;
	self.signalHandlers = {};

	switch(busName) {
	case 'system':
		self.connection = dbus._dbus.getBus(0);
		break;

	case 'session':
		self.connection = dbus._dbus.getBus(1);
		break;
	}

	self.on('signal', function(uniqueBusName, sender, objectPath, interfaceName, signalName, args) {
		var signalHash = objectPath + ':' + interfaceName;

		if (self.signalHandlers[signalHash]) {
			var args = [ signalName ].concat(Array.prototype.slice.call(arguments));

			var interfaceObj = self.signalHandlers[signalHash];
			interfaceObj.emit.apply(interfaceObj, args);
		}
	});

	// Register signal handler of this connection
	self.dbus.signalHandlers[self.connection.uniqueName] = self;
	self.dbus.enableSignal();
};

util.inherits(Bus, events.EventEmitter);

Bus.prototype.introspect = function(serviceName, objectPath, callback) {
	var self = this;

	self.dbus._dbus.callMethod(self.connection,
		serviceName,
		objectPath,
		'org.freedesktop.DBus.Introspectable',
		'Introspect',
		's',
		10000,
		[],
		function(introspect) {
			var obj = self.dbus._dbus.parseIntrospectSource(introspect);

			if (callback)
				callback(null, obj);
		});
};

Bus.prototype.getInterface = function(serviceName, objectPath, interfaceName, callback) {
	var self = this;

	self.introspect(serviceName, objectPath, function(err, obj) {
		if (!obj[interfaceName]) {
			if (callback)
				callback(new Error('No such interface'));

			return;
		}

		// Create a interface object based on introspect
		var iface = new Interface(self, serviceName, objectPath, interfaceName, obj[interfaceName]);

		if (callback)
			callback(null, iface);
	});
};

Bus.prototype.registerSignalHandler = function(serviceName, objectPath, interfaceName, interfaceObj) {
	var self = this;

	// Make a hash
	var signalHash = objectPath + ':' + interfaceName;
	self.signalHandlers[signalHash] = interfaceObj;

	// Register interface signal handler
	self.dbus.addSignalFilter(self, interfaceName);
};
