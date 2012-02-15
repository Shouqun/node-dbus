#!/usr/bin/env python

usage = """Usage:
python example-signal-emitter.py &
python example-signal-recipient.py
python example-signal-recipient.py --exit-service
"""

# Copyright (C) 2004-2006 Red Hat Inc. <http://www.redhat.com/>
# Copyright (C) 2005-2007 Collabora Ltd. <http://www.collabora.co.uk/>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import sys
import traceback

import gobject

import dbus
import dbus.mainloop.glib

def handle_reply(msg):
    print msg

def handle_error(e):
    print str(e)

def emit_signal():
   #call the emitHelloSignal method 
   object.emitHelloSignal(dbus_interface="com.example.TestService")
                          #reply_handler = handle_reply, error_handler = handle_error)
   # exit after waiting a short time for the signal
   #gobject.timeout_add(2000, loop.quit)

   if sys.argv[1:] == ['--exit-service']:
      object.Exit(dbus_interface='com.example.TestService')

   return False

def hello_signal_handler(hello_string):
    print ("Received signal (by connecting using remote object) and it says: "
           + hello_string)
    gobject.timeout_add(2000, emit_signal)

def catchall_signal_handler(*args, **kwargs):
    print ("Caught signal (in catchall handler) "
           + kwargs['dbus_interface'] + "." + kwargs['member'])
    for arg in args:
        print "        " + str(arg)

def catchall_hello_signals_handler(hello_string):
    print "Received a hello signal and it says " + hello_string
    
def catchall_testservice_interface_handler(hello_string, dbus_message):
    print "com.example.TestService interface says " + hello_string + " when it sent signal " + dbus_message.get_member()


if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    bus = dbus.SessionBus()
    try:
        object  = bus.get_object("com.example.TestService","/com/example/TestService/object")

        object.connect_to_signal("HelloSignal", hello_signal_handler, dbus_interface="com.example.TestService", arg0="Hello")
    except dbus.DBusException:
        traceback.print_exc()
        print usage
        sys.exit(1)

    #lets make a catchall
    bus.add_signal_receiver(catchall_signal_handler, interface_keyword='dbus_interface', member_keyword='member')

    bus.add_signal_receiver(catchall_hello_signals_handler, dbus_interface = "com.example.TestService", signal_name = "HelloSignal")

    bus.add_signal_receiver(catchall_testservice_interface_handler, dbus_interface = "com.example.TestService", message_keyword='dbus_message')

    # Tell the remote object to emit the signal after a short delay
    gobject.timeout_add(2000, emit_signal)

    loop = gobject.MainLoop()
    loop.run()
