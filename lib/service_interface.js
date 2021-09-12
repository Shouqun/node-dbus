"use strict";

var util = require('util');
var events = require('events');
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

util.inherits(ServiceInterface, events.EventEmitter);

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

	return self;
};

ServiceInterface.prototype.addProperty = function(propName, opts) {
	var self = this;

	var _opts = opts || {};
	var propObj = {
		access: 'read',
		type: opts.type || 'v',
		getter: opts.getter || null,
		setter: opts.setter || null
	};

	if (_opts['setter']) {
		propObj['access'] = 'readwrite';
	}

	self.properties[propName] = propObj;

	return self;
};

ServiceInterface.prototype.addSignal = function(signalName, opts) {
	var self = this;

	if (!opts)
		return;

	self.signals[signalName] = opts;

	self.on(signalName, function() {
		var args = [ signalName ].concat(Array.prototype.slice.call(arguments));
		self.emitSignal.apply(this, args);
	});

	return self;
};

ServiceInterface.prototype.call = function(method, message, args) {
	var self = this;

	var member = self.methods[method];
	if (!member) {
		self.object.service.bus._sendErrorMessageReply(message, 'org.freedesktop.DBus.Error.UnknownMethod');
		return;
	}

	var inArgs = member['in'] || [];
	if (inArgs.length != args.length) {
		self.object.service.bus._sendErrorMessageReply(message, 'org.freedesktop.DBus.Error.InvalidArgs');
		return;
	}

	var type;
	if (typeof member.out === 'function') {
		// allow interfaces such as 'org.freedesktop.DBus.Properties' to return out definition according arguments 
		type = member.out.apply(this, [self.name, method, args]).type;
	} else if (member.out) {
		type = member.out.type || '';
	}

	// Preparing callback
	args = Array.prototype.slice.call(args).concat([ function(err, value) {
		// Error handling
		if (err) {
			var errorName = 'org.freedesktop.DBus.Error.Failed';
			var errorMessage = err.toString();
			if (err instanceof Error) {
				errorMessage = err.message;
				errorName = err.dbusName || 'org.freedesktop.DBus.Error.Failed';
			}
			self.object.service.bus._sendErrorMessageReply(message, errorName, errorMessage);

			return;
		}

		self.object.service.bus._sendMessageReply(message, value, type);
	} ]);

	member.handler.apply(this, args);
}

ServiceInterface.prototype.getProperty = function(propName, callback) {
	var self = this;
	var prop = self.properties[propName];
	if (!prop) {
		return false;
	}

	prop.getter.apply(this, [ function(err, value) {
		if (callback)
			callback(err, value);
	} ]);

	return true;
};

ServiceInterface.prototype.setProperty = function(propName, value, callback) {
	var self = this;
	var prop = self.properties[propName];
	if (!prop) {
		return false;
	}

	var args = [value];

	args.push(function(err) {
		// Completed
		callback(err);
	});

	prop.setter.apply(this, args);

	return true;
};

ServiceInterface.prototype.getProperties = function(callback) {
	var self = this;
	var properties = {};

	var props = Object.keys(self.properties);
	Utils.ForEachAsync(props, function(propName, index, arr, next) {

		// Getting property
		var prop = self.properties[propName];
		prop.getter(function(err, value) {
			if (err) {
					// TODO: What do we do if a property throws an error?
					// For now, just skip the property?
					return next();
			}
			properties[propName] = value;
			next();
		});

		return true;
	}, function() {
		if (callback)
			callback(null, properties);
	});
};

ServiceInterface.prototype.emitSignal = function() {
	var self = this;

	var service = self.object.service;
	if (!service.connected) {
		throw new Error('Service is no longer connected');
	}

	var conn = self.object.service.bus.connection;
	var objPath = self.object.path;
	var interfaceName = self.name;
	var signalName = arguments[0];
	var args = Array.prototype.slice.call(arguments);
	args.splice(0, 1);

	var signal = self.signals[signalName] || null;
	if (!signal)
		return;

	var signatures = [];
	for (var index in signal.types) {
		signatures.push(signal.types[index].type);
	}

	self.object.service.bus._dbus.emitSignal(conn, objPath, interfaceName, signalName, args, signatures);

	return self;
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
			if (typeof arg === 'function') {
				arg = arg.apply(this, [self.name, method, []]);
			}
			if (arg.name)
				introspection.push('<arg direction="in" type="' + arg.type + '" name="' + arg.name + '"/>');
			else
				introspection.push('<arg direction="in" type="' + arg.type + '"/>');
		}

		if (method['out']) {
			var out_def = method['out'];
			if (typeof out_def === 'function') {
				out_def = out_def.apply(this, [self.name, method, []]);
			}
			if (out_def.name)
				introspection.push('<arg direction="out" type="' + out_def.type + '" name="' + out_def.name + '"/>');
			else
				introspection.push('<arg direction="out" type="' + out_def.type + '"/>');
		}

		introspection.push('</method>');
	}

	// Properties
	for (var propName in self.properties) {
		var property = self.properties[propName];

		introspection.push('<property name="' + propName + '" type="' + property['type'].type + '" access="' + property['access'] + '"/>');
	}

	// Signal
	for (var signalName in self.signals) {
		var signal = self.signals[signalName];

		introspection.push('<signal name="' + signalName + '">');

		// Arguments
		if (signal.types) {
			for (var index in signal.types) {
				var arg = signal.types[index];
				if (arg.name)
					introspection.push('<arg type="' + arg.type + '" name="' + arg.name + '"/>');
				else
					introspection.push('<arg type="' + arg.type + '"/>');
			}
		}

		introspection.push('</signal>');
	}

	introspection.push('</interface>');

	self.introspection = introspection.join('\n');

	return self;
};
