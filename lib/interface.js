"use strict";

var util = require('util');
var events = require('events');

var Interface = module.exports = function(bus, serviceName, objectPath, interfaceName, obj) {
	var self = this;

	self.bus = bus;
	self.serviceName = serviceName;
	self.objectPath = objectPath;
	self.interfaceName = interfaceName;
	self.object = obj;

	function _callMethod(method, signature, timeout, args, callback) {

		self.bus.dbus.callMethod(self.bus.connection,
			serviceName,
			objectPath,
			interfaceName,
			method,
			signature,
			timeout,
			args,
			function(result) {
				if (callback)
					callback(result);
			});
	}

	// Initializing methods
	for (var methodName in obj['method']) {

		self[methodName] = (function(method, signature) {

			return function() {
				var args = Array.prototype.slice.call(arguments);
				var timeout = this[method].timeout || -1;
				var handler = this[method].finish || null

				//_callMethod(method, signature, timeout, args, handler);
				self.bus.dbus.callMethod(self.bus.connection,
					self.serviceName,
					self.objectPath,
					self.interfaceName,
					method,
					signature,
					timeout,
					args,
					function(result) {

						if (handler)
							handler(result);
					});
			};
		})(methodName, obj['method'][methodName]['in'].join('') || '');

	}
};

util.inherits(Interface, events.EventEmitter);

Interface.prototype.init = function(callback) {
	var self = this;

	// Initializing signal handler
	var signals = Object.keys(self.object['signal']);
	if (signals.length) {
		self.bus.registerSignalHandler(self.serviceName, self.objectPath, self.interfaceName, self);
	}

	process.nextTick(callback);
};

Interface.prototype.setProperty = function(propertyName, value, callback) {
	var self = this;

	self.bus.dbus.callMethod(self.bus.connection,
		self.serviceName,
		self.objectPath,
		'org.freedesktop.DBus.Properties',
		'Set',
		'ssv',
		-1,
		[ self.interfaceName, propertyName, value ],
		function() {
			if (callback)
				callback();
		});
};

Interface.prototype.getProperty = function(propertyName, callback) {
	var self = this;

	self.bus.dbus.callMethod(self.bus.connection,
		self.serviceName,
		self.objectPath,
		'org.freedesktop.DBus.Properties',
		'Get',
		'ss',
		10000,
		[ self.interfaceName, propertyName ],
		function(value) {

			if (callback)
				callback(value);
		});
};

Interface.prototype.getProperties = function(callback) {
	var self = this;

	self.bus.dbus.callMethod(self.bus.connection,
		self.serviceName,
		self.objectPath,
		'org.freedesktop.DBus.Properties',
		'GetAll',
		's',
		-1,
		[ self.interfaceName ],
		function(value) {

			if (callback)
				callback(value);
		});
};
