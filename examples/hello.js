var DBus = require('../');

var dbus = new DBus();

var bus = dbus.getBus('system');

bus.getInterface('org.freedesktop.DBus', '/', 'org.freedesktop.DBus.Introspectable');
