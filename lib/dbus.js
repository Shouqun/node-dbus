var util = require('util');
var EventEmitter = require('events').EventEmitter;
var dbuslib = require('./dbus.node');


function DBusConnection() {
  this.classname= "DBusConnection";
}

util.inherits(DBusConnection, EventEmitter);

dbuslib.createConnection = function(type, name, path, iface) {
   var connection = new DBusConnection();
    process.nextTick( function() {
        dbuslib.init();
        
        if (type == "session")  {
          connection.bus = dbuslib.session_bus();   
        } else if (type == "system") {
          connection.bus = dbuslib.system_bus();
        }

        connection.interface = dbuslib.get_interface(connection.bus,
                                    name, path, iface);
        for (var propertyName in connection.interface ) {
          console.log(propertyName);

          // Check whether it is a signal
          if ( connection.interface[propertyName].hasOwnProperty('onemit')) {
            //set the onemit callback
            connection.interface[propertyName].onemit = function (args) {
              console.log(args);
              connection.emit(propertyName, args);
            }

            connection.interface[propertyName].enabled = true;
          }
        }
        console.log(connection.data); 
    });

    return connection;
}

dbuslib.start = function(callback) {
    process.nextTick( function() {
        dbuslib.init();
        callback();
    });
}

module.exports =  dbuslib;

module.exports.DBusConnection = DBusConnection;

