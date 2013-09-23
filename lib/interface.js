"use strict";

var util = require('util');
var events = require('events');

var Interface = module.exports = function(bus, serviceName, objectPath, interfaceName, obj) {
	var self = this;

	function _callMethod(method, signature, timeout, args, callback) {

		bus.dbus.callMethod(bus.connection,
			serviceName,
			objectPath,
			interfaceName,
			method,
			signature,
			timeout,
			args,
			function(result) {
				process.nextTick(function() {
					if (callback)
						callback(result);
				});
			});
	}

	function _getProperty(propertyName, timeout, callback) {

		bus.dbus.callMethod(bus.connection,
			serviceName,
			objectPath,
			'org.freedesktop.DBus.Properties',
			'Get',
			'ss',
			timeout,
			[ interfaceName, propertyName ],
			function(value) {
				if (callback)
					callback(value);
			});
	}

	function _getProperties(timeout, callback) {

		bus.dbus.callMethod(bus.connection,
			serviceName,
			objectPath,
			'org.freedesktop.DBus.Properties',
			'GetAll',
			's',
			timeout,
			[ interfaceName ],
			function(value) {
				if (callback)
					callback(value);
			});
	}

	// Initializing methods
	for (var methodName in obj['method']) {

		self[methodName] = (function(method, signature) {
			return function() {
				var args = Array.prototype.slice.call(arguments);
				var timeout = this[method].timeout || -1;
				var handler = this[method].finish || null

				_callMethod(method, signature, timeout, args, handler);
			}
		})(methodName, obj['method'][methodName]['in'].join('') || '');

	}

	// Initializing properties
	self.setProperty = function(propertyName, value, callback) {

		bus.dbus.callMethod(bus.connection,
			serviceName,
			objectPath,
			'org.freedesktop.DBus.Properties',
			'Set',
			'ssv',
			-1,
			[ interfaceName, propertyName, value ],
			function() {

				if (callback)
					callback();
			});
	};

	self.getProperty = function(propertyName, callback) {
		_getProperty(propertyName, 10000, callback);
	};

	self.getProperties = function(callback) {
		_getProperties(-1, callback);
	};

	// Initializing signal handler
	var signals = Object.keys(obj['signal']);
	if (signals.length) {
		bus.registerSignalHandler(serviceName, objectPath, interfaceName, self);
	}
};

util.inherits(Interface, events.EventEmitter);
