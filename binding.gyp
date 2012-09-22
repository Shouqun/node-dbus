{
	'targets': [
		{
			'target_name': 'dbus',
			'sources': [
				'src/dbus.cc',
				'src/context.cc',
				'src/dbus_introspect.cc',
				'src/dbus_register.cc'
			],
			'copies': [{
				'destination': '<(module_root_dir)/lib/',
				'files': [
					'<(module_root_dir)/build/Release/dbus.node'
				]
			}],
			'conditions': [
				['OS=="linux"', {
					'cflags': [
						'<!@(pkg-config --cflags dbus-1)',
						'<!@(pkg-config --cflags dbus-glib-1)'
					],
					'ldflags': [
						'<!@(pkg-config  --libs-only-L --libs-only-other dbus-1)',
						'<!@(pkg-config  --libs-only-L --libs-only-other dbus-glib-1)'
					],
					'libraries': [
						'<!@(pkg-config  --libs-only-l --libs-only-other dbus-1)',
						'<!@(pkg-config  --libs-only-l --libs-only-other dbus-glib-1)'
					]
				}]
			]
		}
	]
}
