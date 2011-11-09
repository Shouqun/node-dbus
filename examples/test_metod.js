var dbus = require("../build/default/dbus.node");

session = dbus.session_bus();

interface = dbus.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");

interface.WhoAmI();

