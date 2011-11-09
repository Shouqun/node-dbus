#!/usr/bin/env python

import Options
from os import unlink, symlink, popen
from os.path import exists 

srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')

  conf.env.append_value("LIB_EXPAT", "expat");
  conf.check_cfg(package='dbus-1', args='--cflags --libs', uselib_store='DBUS');
  conf.check_cfg(package='dbus-glib-1', args='--cflags --libs', uselib_store='GDBUS');


def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'dbus'
  obj.source = '''
              src/dbus.cc
              src/dbus_introspect.cc
              '''
  obj.uselib = 'DBUS GDBUS EXPAT'
