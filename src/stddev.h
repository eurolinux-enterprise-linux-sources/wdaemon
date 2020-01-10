#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef STDDEV_H
#define STDDEV_H
#include "wacom.h"
#define KEYBOARD_MAX 1
#define MOUSE_MAX 1
void keyboard_print_device_type(int index);
int keyboard_add_device(struct session *session, char *path, int type);
void mouse_print_device_type(int index);
int mouse_add_device(struct session *session, char *path, int type);
#endif

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
