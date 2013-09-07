"use strict";

var _dbus = require('../build/Release/dbus.node');

var Bus = require('./bus');

var busMap = {};

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

	this._dbus.setSignalHandler(function(uniqueName) {

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
