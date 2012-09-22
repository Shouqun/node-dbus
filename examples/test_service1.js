var dbus = require("../lib/dbus");

dbus.start(function() {
	var service_path = 'org.freedesktop.DBus.TestSuitePythonService';
	var object_path = '/org/freedesktop/DBus/TestSuitePythonObject';
	var interface_path = 'org.freedesktop.DBus.TestSuiteInterface';

	console.log('Start Service');
	session = dbus.session_bus();

	var dbusRegister = new dbus.DBusRegister(dbus, session);

	/* Provide Methods */
	var Methods = {
		WhoAmI: function() {
			console.log('I have no idea who are you!');
		},
		WhoAreYou: function () {
			return 'I AM Spiderman!';
		},
		GetArray: function() {
			return [ 'Fred', 'Stacy', 123, {
				'name': 'Fred'
			} ];
		},
		GetObject: function() {
			return {
				'Superman': 'cool',
				'Batman': 'black',
				'nobody': 123,
				'Other': {
					'SecondLevel': 'Got it!',
					'Ha': 123
				},
				'sub-array': [ 'one', 'two', 3 ]
			};
		}
	};

	/* Bus Name */
	dbus.requestName(session, service_path);

	/* Register Agent Methods */
	dbusRegister.addMethods(object_path, interface_path, Methods);
	dbusRegister.addMethods(object_path, 'org.freedesktop.DBus.Introspectable', {
		Introspect: function() {
			return '<?xml version=\"1.0\" encoding=\"UTF-8\" ?>' +
				'<node name=\"' + object_path + '\">' +
					'<interface name=\"' + interface_path + '\">' +
					'<method name=\"WhoAmI\">' +
					'</method>' + 
					'<method name=\"WhoAreYou\">' +
					'<arg name=\"data\" type=\"s\" direction=\"out\" />' +
					'</method>' + 
					'<method name=\"GetArray\">' +
					'<arg type=\"a(oa{sv})\" direction=\"out\"/>' +
					'</method>' + 
					'<method name=\"GetObject\">' +
					'<arg type=\"a{sv}\" direction=\"out\"/>' +
					'</method>' + 
					'</interface>' +
					'<interface name=\"org.freedesktop.DBus.Introspectable\">' +
					'<method name=\"Introspect\">' +
					'<arg name=\"data\" type=\"s\" direction=\"out\" />' +
					'</method>' + 
					'</interface>' +
				'</node>';
		}
	});
});
