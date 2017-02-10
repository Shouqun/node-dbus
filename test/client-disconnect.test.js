var tap = require('tap');
var DBus = require('../');

var dbus = new DBus();
var bus = dbus.getBus('session');

tap.plan(1);

var timeout = setTimeout(function() {
	tap.fail('Should have disconnected by now');
	process.exit();
}, 1000);
timeout.unref();

tap.ok('This is a dummy test. The real test is whether the tap.fail gets called on the timeout.');
bus.disconnect();
