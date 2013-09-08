"use strict";

var ServiceInterface = module.exports = function(object, interfaceName) {
	var self = this;

	self.object = object;
	self.name = interfaceName;
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

	if (_opts['in'])
		methodObj['in'] = _opts['in'];

	if (_opts['out'])
		methodObj['out'] = _opts['out'];

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
