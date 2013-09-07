"use strict";

var Interface = module.exports = function(bus, serviceName, objectPath, interfaceName, obj) {
	var self = this;

	function _callMethod(method, signature, timeout, args, callback) {

		bus.dbus._dbus.callMethod(bus.connection,
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

	function _getProperty(propertyName, timeout, callback) {

		bus.dbus._dbus.callMethod(bus.connection,
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

		bus.dbus._dbus.callMethod(bus.connection,
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
				_callMethod(method, signature, this[method].timeout || -1, args, this[method].finish || null);
			}
		})(methodName, obj['method'][methodName]['in'].join('') || '');
	}

	// Initializing properties
	self.setProperty = function(propertyName, value, callback) {

		bus.dbus._dbus.callMethod(bus.connection,
			serviceName,
			objectPath,
			'org.freedesktop.DBus.Properties',
			'Set',
			'ssv',
			timeout,
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
		_getProperties(10000, callback);
	};

};
