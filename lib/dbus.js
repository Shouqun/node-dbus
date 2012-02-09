var dbuslib = require('dbus.node');

dbuslib.start = function(callback) {
    process.nextTick( function() {
        dbuslib.init();
        callback();
    });
}

module.exports =  dbuslib;

