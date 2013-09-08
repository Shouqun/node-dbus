"use strict";

var ServiceInterface = module.exports = function(object, interfaceName) {
	var self = this;

	self.object = object;
	self.name = interfaceName;
	self.methods = {};
	self.properties = {};
	self.signals = {};
};

ServiceInterface.prototype.getSignature = function(type) {
	if (type === String) {
		return 's';
	} else if (type === Number) {
		return 'd';
	} else if (type === Array) {
		return 'av';
	} else if (type === Object) {
		return 'a{sv}';
	}

	return null;
};

ServiceInterface.prototype.addMethod = function(method, opts, handler) {
	var self = this;

	var _opts = opts || {};
	var methodObj = {
		'handler': handler
	};

	if (_opts['in']) {
		var argSignature = [];
		for (var index in _opts['in']) {
			var signature = self.getSignature(_opts['in'][index]);
			if (signature)
				argSignature.push(signature);
		}

		if (argSignature.length)
			methodObj['in'] = argSignature;
	}

	if (_opts['out']) {
		methodObj['out'] = self.getSignature(_opts['out']);
	}

	self.methods[method] = methodObj;
};

ServiceInterface.prototype.call = function(method, message, args) {
	var self = this;

	var member = self.methods[method];

	// Preparing callback
	args = args.concat([ function(value) {
		
		self.object.service.bus.dbus._dbus.sendMessageReply(message, value, member.out);
	} ]);

	member.handler.apply(this, args);
}
