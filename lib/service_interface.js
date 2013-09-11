"use strict";

var Utils = require('./utils');

var ServiceInterface = module.exports = function(object, interfaceName) {
	var self = this;

	self.object = object;
	self.name = interfaceName;
	self.introspection = null;
	self.methods = {};
	self.properties = {};
	self.signals = {};
};

ServiceInterface.prototype.addMethod = function(method, opts, handler) {
	var self = this;

	var _opts = opts || {};
	var methodObj = {
		handler: handler
	};

	if (_opts['in']) {
		var argSignature = [];
		for (var index in _opts['in']) {
			argSignature.push(_opts['in'][index]);
		}

		if (argSignature.length)
			methodObj['in'] = argSignature;
	}

	if (_opts['out']) {
		methodObj['out'] = _opts['out'];
	}

	self.methods[method] = methodObj;
};

ServiceInterface.prototype.addProperty = function(propName, opts, handler) {
	var self = this;

	var _opts = opts || {};
	var propObj = {
		access: 'read',
		handler: handler
	};

	if (_opts['out']) {
		propObj['out'] = _opts['out'];
	}
	
	if (_opts['access']) {
		propObj['access'] = _opts['access'];
	}

	self.properties[propName] = propObj;
};

ServiceInterface.prototype.call = function(method, message, args) {
	var self = this;

	var member = self.methods[method];
	if (!member) {
		self.object.service.bus.dbus._sendMessageReply(message, 'Doesn\'t support such method', 'v');
		return;
	}

	// Preparing callback
	args = args.concat([ function(value) {
		self.object.service.bus.dbus._sendMessageReply(message, value, member.out.type || '');
	} ]);

	member.handler.apply(this, args);
}

ServiceInterface.prototype.getProperty = function(propName, callback) {
	var self = this;
	var prop = self.properties[propName];
	if (!prop) {
		return false;
	}

	prop.handler.apply(this, [ function(value) {
		if (callback)
			callback(value);
	} ]);

	return true;
};

ServiceInterface.prototype.getProperties = function(callback) {
	var self = this;
	var properties = {};

	var props = Object.keys(self.properties);
	Utils.ForEachAsync(props, function(propName, index, arr, next) {
		self.getProperty(propName, function(value) {
			properties[propName] = value;
			next();
		});

		return true;
	}, function() {
		if (properties)
			callback(properties);
	});
};

ServiceInterface.prototype.update = function() {
	var self = this;

	var introspection = [];

	introspection.push('<interface name="' + self.name + '">');

	// Methods
	for (var methodName in self.methods) {
		var method = self.methods[methodName];

		introspection.push('<method name="' + methodName + '">');

		// Arguments
		for (var index in method['in']) {
			var arg = method['in'][index];
			if (arg.name)
				introspection.push('<arg direction="in" type="' + arg.type + '" name="' + arg.name + '"/>');
			else
				introspection.push('<arg direction="in" type="' + arg.type + '"/>');
		}

		if (method['out']) {
			if (method['out'].name)
				introspection.push('<arg direction="out" type="' + method['out'].type + '" name="' + method['out'].name + '"/>');
			else
				introspection.push('<arg direction="out" type="' + method['out'].type + '"/>');
		}

		introspection.push('</method>');
	}

	// Properties
	for (var propName in self.properties) {
		var property = self.properties[propName];

		introspection.push('<property name="' + propName + '" type="' + property['out'].type + '" access="' + property['access'] + '"/>');
	}

	introspection.push('</interface>');

	self.introspection = introspection.join('\n');
};
