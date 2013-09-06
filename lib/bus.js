"usr strict";

var Interface = require('./interface');

var Bus = module.exports = function(dbus, busName) {
	var self = this;

	self.dbus = dbus;
	self.type = busName;

	switch(busName) {
	case 'system':
		self.connection = dbus._dbus.getBus(0);
		break;

	case 'session':
		self.connection = dbus._dbus.getBus(1);
		break;
	}
};

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
