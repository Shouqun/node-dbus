var dbus = require("../lib/dbus");
var fs = require("fs");

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
  
  /*Set the introspectable method, read xml source file fist*/
  xmlSource = fs.readFileSync('examples/test_service.xml', encoding='utf8');
  console.log(xmlSource);
  dbusRegister.setIntrospectable(object_path, xmlSource );

	/* Loop */
	dbus.runListener();
});
