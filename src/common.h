#ifndef NODE_DBUS_COMMON_H
#define NODE_DBUS_COMMON_H

#ifdef DEBUG
#include <stdio.h>
#define LOG(fmt, args...)  fprintf(stdout, fmt, ##args);
#define ERROR(fmt, args...) fprintf(stderr, fmt, ##args);
#else
#define LOG(fmt, args...) 
#define ERROR(fmt, args...)
#endif

#endif

