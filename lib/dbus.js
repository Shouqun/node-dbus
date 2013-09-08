"use strict";

var _dbus = require('../build/Release/dbus.node');

var Bus = require('./bus');
var Service = require('./service');

var busMap = {};
var serviceMap = {};

// Dispatch events to service
_dbus.setObjectHandler(function(uniqueName, sender, objectPath, interfaceName, member, message, args) {

	for (var hash in serviceMap) {
		var service = serviceMap[hash];

		if (service.bus.connection.uniqueName != uniqueName)
			continue;

		// Fire event
		var args = [ 'request' ].concat(Array.prototype.slice.call(arguments));
		service.emit.apply(service, args);

		break;
	}
});

var DBus = module.exports = function() {
	var self = this;

	self._dbus = _dbus;
	self.signalHandlers = {};
};

DBus.prototype.getBus = function(busName) {
	var self = this;

	// Return bus existed
	if (busMap[busName])
		return busMap[busName];

	var bus = new Bus(self, busName);
	busMap[busName] = bus;

	return bus;
};

DBus.prototype.enableSignal = function() {
	var self = this;

	_dbus.setSignalHandler(function(uniqueName) {

		if (self.signalHandlers[uniqueName]) {
			var args = [ 'signal' ].concat(Array.prototype.slice.call(arguments));

			self.signalHandlers[uniqueName].emit.apply(self.signalHandlers[uniqueName], args);
		}
	});
};

DBus.prototype.addSignalFilter = function(bus, interfaceName) {
	var rule = 'type=\'signal\',interface=\'' + interfaceName + '\'';

	_dbus.addSignalFilter(bus.connection, rule);
};

DBus.prototype.registerService = function(busName, serviceName) {
	var self = this;

	// Return bus existed
	var serviceHash = busName + ':' + serviceName;
	if (serviceMap[serviceHash])
		return serviceMap[serviceHash];

	// Create a new connection
	var bus = new Bus(self, busName);

	// Create service
	var service = new Service(bus, serviceName);
	serviceMap[serviceHash] = service;

	_dbus.requestName(bus.connection, serviceName);

	return service;
};
