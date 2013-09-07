"use strict";

var _dbus = require('../build/Release/dbus.node');

var Bus = require('./bus');

var DBus = module.exports = function() {
	var self = this;

	self._dbus = _dbus;
	self.signalHandlers = {};
};

DBus.prototype.getBus = function(busName) {
	var self = this;

	return new Bus(self, busName);
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
