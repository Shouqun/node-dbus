var dbus = require("../lib/dbus");

dbus.start( function() {
	console.log('Start');
	session = dbus.session_bus();

	interface = dbus.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");

	interface.WhoAmI();
	console.log(interface.WhoAreYou());

	var arr = interface.GetArray();
	for (var index in arr) {
		console.log('Array[' + index + ']=' + arr[index]);
	}

	var obj = interface.GetObject();
	for (var key in obj) {
		console.log('Object.' + key + '=' + obj[key]);
	}

	console.log('Original Object Structure:');
	console.log(obj);
	
	// Asynchronize method
	var result = interface.AsyncMethod();
	console.log(result);
});
