"use strict";

var ServiceInterface = require('./service_interface');

var ServiceObject = module.exports = function(service, objectPath) {
	var self = this;

	self.service = service;
	self.path = objectPath;
	self.interfaces = {};
	self.introspection = null;

	// Initializing introspectable interface
	self.introspectableInterface = self.createInterface('org.freedesktop.DBus.Introspectable');
	self.introspectableInterface.addMethod('Introspect', { out: String, }, function(callback) {
		callback(self.introspection);
	});
	self.updateIntrospection();
};

ServiceObject.prototype.createInterface = function(interfaceName) {
	var self = this;

	if (!self.interfaces[interfaceName])
		self.interfaces[interfaceName] = new ServiceInterface(self, interfaceName);

	self.updateIntrospection();

	return self.interfaces[interfaceName];
};

ServiceObject.prototype.updateIntrospection = function() {
	var self = this;

	var introspection = [
		'<!DOCTYPE node PUBLIC "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"',
		'"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">',
		'<node name="' + self.path + '">'
	];

	for (var interfaceName in self.interfaces) {
		var iface = self.interfaces[interfaceName];
		introspection.push('<interface name="' + interfaceName + '">');

		// Method
		for (var methodName in iface.methods) {
			var method = iface.methods[methodName];

			introspection.push('<method name="' + methodName + '">');

			// Arguments
			for (var index in method['in']) {
				introspection.push('<arg direction="in" type="' + method['in'][index] + '"/>');
			}

			if (method['out'])
				introspection.push('<arg direction="out" type="' + method['out'] + '"/>');

			introspection.push('</method>');
		}

		introspection.push('</interface>');
	}

	introspection.push('</node>');

	self.introspection = introspection.join('\n');
};
