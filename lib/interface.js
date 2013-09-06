"use strict";

var Interface = module.exports = function(bus, serviceName, objectPath, interfaceName, obj) {
	var self = this;

	self.dbus = bus.dbus;
	self.bus = bus;

	function _callMethod(method, signature, timeout, args, callback) {

		self.dbus._dbus.callMethod(bus.connection,
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
				_callMethod(method, signature, this[method].timeout || -1, args, this[method].finish || null);
			}
		})(methodName, obj['method'][methodName].out || '');
	}
};
