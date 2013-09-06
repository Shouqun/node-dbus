var DBus = require('../');

var dbus = new DBus();

var bus = dbus.getBus('system');

//bus.getInterface('org.freedesktop.DBus', '/', 'org.freedesktop.DBus.Introspectable');
bus.getInterface('org.freedesktop.DisplayManager', '/org/freedesktop/DisplayManager', 'org.freedesktop.DBus.Introspectable');
