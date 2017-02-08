var withService = require('./with-service');
var tap = require('tap');
var DBus = require('../');

tap.plan(1);
withService(function(err, done) {
	if (err) throw err;

	var dbus = new DBus();
	var bus = dbus.getBus('session');

	bus.getInterface('test.dbus.TestService', '/test/dbus/TestService', 'test.dbus.TestService.Interface1', function(err, iface) {
		iface.NoArgs.timeout = 1000;
		iface.NoArgs.finish = function(result) {
			tap.equal(result, 'result!');
			done();
		};
		iface.NoArgs();
	});
});
