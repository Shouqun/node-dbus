var withService = require('./with-service');
var tap = require('tap');
var DBus = require('../');

tap.plan(1);
withService(function(err, done) {
	if (err) throw err;

	var dbus = new DBus();
	var bus = dbus.getBus('session');

	bus.getInterface('test.dbus.TestService', '/test/dbus/TestService', 'test.dbus.TestService.Interface1', function(err, iface) {
		iface.LongProcess.timeout = 1000;
		iface.LongProcess.finish = function(result) {
			tap.fail('This should not have succeeded');
			done();
		};
		iface.LongProcess.error = function(err) {
			tap.pass('The call timed out as expected');
			done();
		};
		iface.LongProcess();
	});
});
