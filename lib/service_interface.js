"use strict";

var ServiceInterface = module.exports = function(object, interfaceName) {
	var self = this;

	self.object = object;
	self.name = interfaceName;
	self.introspection = null;
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

ServiceInterface.prototype.update = function() {
	var self = this;

	var introspection = [];

	introspection.push('<interface name="' + self.name + '">');

	// Method
	for (var methodName in self.methods) {
		var method = self.methods[methodName];

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

	self.introspection = introspection.join('\n');

	self.object.updateIntrospection();
};
