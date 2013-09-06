"usr strict";

var Bus = module.exports = function(dbus, busName) {
	var self = this;

	self.dbus = dbus;
	self.type = busName;

	switch(busName) {
	case 'system':
		self.bus = dbus._dbus.getBus(0);
		break;

	case 'session':
		self.bus = dbus._dbus.getBus(1);
		break;
	}
};

Bus.prototype.getInterface = function(serviceName, objectPath, interfaceName, callback) {
	var self = this;

	// TODO: Introspect

	self.dbus._dbus.callMethod(self.bus,
		serviceName,
		objectPath,
		interfaceName,
		'Introspect',
		's',
		{},
		function(introspect) {
			var obj = self.dbus._dbus.parseIntrospectSource(introspect);
			console.log(obj);
		});
};
