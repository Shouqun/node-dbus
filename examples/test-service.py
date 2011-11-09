#!/usr/bin/env python

# Copyright (C) 2004 Red Hat Inc. <http://www.redhat.com/>
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
import os
import logging
from time import sleep

import dbus

#if not dbus.__file__.startswith(pydir):
#    raise Exception("DBus modules are not being picked up from the package")

import dbus.service
import dbus.glib
import gobject
import random

from dbus.gobject_service import ExportedGObject


logging.basicConfig(filename='./test-service.log', filemode='w')
logging.getLogger().setLevel(1)
logger = logging.getLogger('test-service')


NAME = "org.freedesktop.DBus.TestSuitePythonService"
IFACE = "org.freedesktop.DBus.TestSuiteInterface"
OBJECT = "/org/freedesktop/DBus/TestSuitePythonObject"

class RemovableObject(dbus.service.Object):
    # Part of test for https://bugs.freedesktop.org/show_bug.cgi?id=10457
    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def IsThere(self):
        return True

    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def RemoveSelf(self):
        self.remove_from_connection()
        return True

class TestGObject(ExportedGObject):
    def __init__(self, bus_name, object_path=OBJECT + '/GObject'):
        super(TestGObject, self).__init__(bus_name, object_path)

    @dbus.service.method(IFACE)
    def Echo(self, arg):
        return arg

class TestInterface(dbus.service.Interface):
    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def CheckInheritance(self):
        return False

class Fallback(dbus.service.FallbackObject):
    def __init__(self, conn, object_path=OBJECT + '/Fallback'):
        super(Fallback, self).__init__(conn, object_path)
        self.add_to_connection(conn, object_path + '/Nested')

    @dbus.service.method(IFACE, in_signature='', out_signature='oos',
                         path_keyword='path', rel_path_keyword='rel',
                         connection_keyword='conn')
    def TestPathAndConnKeywords(self, path=None, conn=None, rel=None):
        return path, rel, conn.get_unique_name()

    @dbus.service.signal(IFACE, signature='s', rel_path_keyword='rel_path')
    def SignalOneString(self, test, rel_path=None):
        logger.info('SignalOneString(%r) @ %r', test, rel_path)

    # Deprecated usage
    @dbus.service.signal(IFACE, signature='ss', path_keyword='path')
    def SignalTwoStrings(self, test, test2, path=None):
        logger.info('SignalTwoStrings(%r, %r) @ %r', test, test2, path)

    @dbus.service.method(IFACE, in_signature='su', out_signature='',
                         path_keyword='path')
    def EmitSignal(self, signal, value, path=None):
        sig = getattr(self, str(signal), None)
        assert sig is not None

        assert path.startswith(OBJECT + '/Fallback')
        rel_path = path[len(OBJECT + '/Fallback'):]
        if rel_path == '':
            rel_path = '/'

        if signal == 'SignalOneString':
            logger.info('Emitting %s from rel %r', signal, rel_path)
            sig('I am a fallback', rel_path=rel_path)
        else:
            val = ('I am', 'a fallback')
            logger.info('Emitting %s from abs %r', signal, path)
            sig('I am', 'a deprecated fallback', path=path)

class MultiPathObject(dbus.service.Object):
    SUPPORTS_MULTIPLE_OBJECT_PATHS = True

class TestObject(dbus.service.Object, TestInterface):
    def __init__(self, bus_name, object_path=OBJECT):
        dbus.service.Object.__init__(self, bus_name, object_path)
        self._removable = RemovableObject()
        self._multi = MultiPathObject(bus_name, object_path + '/Multi1')
        self._multi.add_to_connection(bus_name.get_bus(),
                                      object_path + '/Multi2')
        self._multi.add_to_connection(bus_name.get_bus(),
                                      object_path + '/Multi2/3')

    """ Echo whatever is sent
    """
    @dbus.service.method(IFACE)
    def Echo(self, arg):
        return arg

    @dbus.service.method(IFACE, in_signature='s', out_signature='s')
    def AcceptUnicodeString(self, foo):
        assert isinstance(foo, unicode), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE, in_signature='s', out_signature='s', utf8_strings=True)
    def AcceptUTF8String(self, foo):
        assert isinstance(foo, str), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE, in_signature='', out_signature='soss',
            sender_keyword='sender', path_keyword='path',
            destination_keyword='dest', message_keyword='msg')
    def MethodExtraInfoKeywords(self, sender=None, path=None, dest=None,
            msg=None):
        return (sender, path, dest,
                msg.__class__.__module__ + '.' + msg.__class__.__name__)

    @dbus.service.method(IFACE, in_signature='ay', out_signature='ay')
    def AcceptListOfByte(self, foo):
        assert isinstance(foo, list), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE, in_signature='ay', out_signature='ay', byte_arrays=True)
    def AcceptByteArray(self, foo):
        assert isinstance(foo, str), (foo, foo.__class__.__mro__)
        return foo

    @dbus.service.method(IFACE)
    def GetComplexArray(self):
        ret = []
        for i in range(0,100):
            ret.append((random.randint(0,100), random.randint(0,100), str(random.randint(0,100))))

        return dbus.Array(ret, signature="(uus)")

    def returnValue(self, test):
        if test == 0:
            return ""
        elif test == 1:
            return "",""
        elif test == 2:
            return "","",""
        elif test == 3:
            return []
        elif test == 4:
            return {}
        elif test == 5:
            return ["",""]
        elif test == 6:
            return ["","",""]

    @dbus.service.method(IFACE, in_signature='u', out_signature='s')
    def ReturnOneString(self, test):
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='ss')
    def ReturnTwoStrings(self, test):
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='(ss)')
    def ReturnStruct(self, test):
        logger.info('ReturnStruct(%r) -> %r', test, self.returnValue(test))
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='as')
    def ReturnArray(self, test):
        return self.returnValue(test)

    @dbus.service.method(IFACE, in_signature='u', out_signature='a{ss}')
    def ReturnDict(self, test):
        #return self.returnValue(test)
        return dbus.Dictionary({"aaa":"bbb", "ccc":"ddd"})

    @dbus.service.method(IFACE, in_signature='a{ss}', out_signature='s')
    def ParamDict(self, test):
        print test
        string = str(test);
        return string

    @dbus.service.method(IFACE, in_signature='a{us}', out_signature='s')
    def ParamDict1(self, test):
        print test
        string = str(test);
        return string 

    @dbus.service.signal(IFACE, signature='s')
    def SignalOneString(self, test):
        logger.info('SignalOneString(%r)', test)

    @dbus.service.signal(IFACE, signature='ss')
    def SignalTwoStrings(self, test, test2):
        logger.info('SignalTwoStrings(%r, %r)', test, test2)

    @dbus.service.signal(IFACE, signature='(ss)')
    def SignalStruct(self, test):
        logger.info('SignalStruct(%r)', test)

    @dbus.service.signal(IFACE, signature='as')
    def SignalArray(self, test):
        pass

    @dbus.service.signal(IFACE, signature='a{ss}')
    def SignalDict(self, test):
        pass

    @dbus.service.method(IFACE, in_signature='su', out_signature='')
    def EmitSignal(self, signal, value):
        sig = getattr(self, str(signal), None)
        assert(sig != None)

        val = self.returnValue(value)
        # make two string case work by passing arguments in by tuple
        if (signal == 'SignalTwoStrings' and (value == 1 or value == 5)):
            val = tuple(val)
        else:
            val = tuple([val])

        logger.info('Emitting %s with %r', signal, val)
        sig(*val)

    def CheckInheritance(self):
        return True

    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def TestListExportedChildObjects(self):
        objs_root = session_bus.list_exported_child_objects('/')
        assert objs_root == ['org'], objs_root
        objs_org = session_bus.list_exported_child_objects('/org')
        assert objs_org == ['freedesktop'], objs_org
        return True

    @dbus.service.method(IFACE, in_signature='bbv', out_signature='v', async_callbacks=('return_cb', 'error_cb'))
    def AsynchronousMethod(self, async, fail, variant, return_cb, error_cb):
        try:
            if async:
                gobject.timeout_add(500, self.AsynchronousMethod, False, fail, variant, return_cb, error_cb)
                return
            else:
                if fail:
                    raise RuntimeError
                else:
                    return_cb(variant)

                return False # do not run again
        except Exception, e:
            error_cb(e)

    @dbus.service.method(IFACE, in_signature='', out_signature='s', sender_keyword='sender')
    def WhoAmI(self, sender):
        return sender

    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def AddRemovableObject(self):
        # Part of test for https://bugs.freedesktop.org/show_bug.cgi?id=10457
        # Keep the removable object reffed, since that's the use case for this
        self._removable.add_to_connection(self._connection,
                                          OBJECT + '/RemovableObject')
        return True

    @dbus.service.method(IFACE, in_signature='', out_signature='b')
    def HasRemovableObject(self):
        # Part of test for https://bugs.freedesktop.org/show_bug.cgi?id=10457
        objs = session_bus.list_exported_child_objects(OBJECT)
        return ('RemovableObject' in objs)

    @dbus.service.method(IFACE)
    def MultipleReturnWithoutSignature(self):
        # https://bugs.freedesktop.org/show_bug.cgi?id=10174
        return dbus.String('abc'), dbus.Int32(123)

    @dbus.service.method(IFACE, in_signature='', out_signature='')
    def BlockFor500ms(self):
        sleep(0.5)

    @dbus.service.method(IFACE, in_signature='', out_signature='',
                         async_callbacks=('return_cb', 'raise_cb'))
    def AsyncWait500ms(self, return_cb, raise_cb):
        def return_from_async_wait():
            return_cb()
            return False
        gobject.timeout_add(500, return_from_async_wait)

    @dbus.service.method(IFACE, in_signature='', out_signature='')
    def RaiseValueError(self):
        raise ValueError('Wrong!')

    @dbus.service.method(IFACE, in_signature='', out_signature='')
    def RaiseDBusExceptionNoTraceback(self):
        class ServerError(dbus.DBusException):
            """Exception representing a normal "environmental" error"""
            include_traceback = False
            _dbus_error_name = 'com.example.Networking.ServerError'

        raise ServerError('Server not responding')

    @dbus.service.method(IFACE, in_signature='', out_signature='')
    def RaiseDBusExceptionWithTraceback(self):
        class RealityFailure(dbus.DBusException):
            """Exception representing a programming error"""
            include_traceback = True
            _dbus_error_name = 'com.example.Misc.RealityFailure'

        raise RealityFailure('Botched invariant')

    @dbus.service.method(IFACE, in_signature='', out_signature='',
                         async_callbacks=('return_cb', 'raise_cb'))
    def AsyncRaise(self, return_cb, raise_cb):
        class Fdo12403Error(dbus.DBusException):
            _dbus_error_name = 'org.freedesktop.bugzilla.bug12403'

        raise_cb(Fdo12403Error())

session_bus = dbus.SessionBus()
global_name = dbus.service.BusName(NAME, bus=session_bus)
object = TestObject(global_name)
g_object = TestGObject(global_name)
fallback_object = Fallback(session_bus)
loop = gobject.MainLoop()
loop.run()
