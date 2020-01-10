/*
 *    This file is part of wdaemon.
 *
 *    wdaemon is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    wdaemon is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with wdaemon; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef MONITORED_H
#define MONITORED_H
#include <sys/select.h>
#include "uinput.h"
/** max times wdaemon should try to open a file before giving up */
#define MAX_ADD_TRIES	10

enum pending_action {
	NONE = 0,
	ADD,
	REMOVE,
};

struct monitored_device {
	char *name;
	char *short_name;
	struct uinput_info info;
	enum pending_action pending_action;
	int plugged;
	int fd;
	int tries;
};

struct pollfd;
struct monitored_devices {
	int count;
	int size;
	struct monitored_device *device;
	struct pollfd *ufds;
};

int monitored_device_add(struct monitored_devices *devices,
			 void (*init)(struct uinput_info *info),
			 void *priv, char *device);
struct monitored_devices *monitored_devices_init(int count);
int monitored_device_plug(struct monitored_devices *devices, char *name);
int monitored_device_unplug(struct monitored_devices *devices, char *name);
#endif	/* MONITORED_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
