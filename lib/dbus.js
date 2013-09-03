"use strict";

var _dbus = require('../build/Release/dbus.node');

var Bus = require('./bus');

var DBus = module.exports = function() {
	var self = this;

	self._dbus = _dbus;
};

DBus.prototype.getBus = function(busName) {
	var self = this;

	return new Bus(self, busName);
};
