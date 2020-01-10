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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/poll.h>
#include "log.h"
#include "session.h"
#include "monitored.h"
#include "uinput.h"
#include "input.h"

int monitored_device_add(struct monitored_devices *devices,
			 void (*init)(struct uinput_info *info),
			 void *priv, char *device)
{
	struct monitored_device *dev;

	if (devices->count == devices->size) {
		log(LOG_WARNING, "monitored_device_add: maximum device "
		    "number reached (%i)\n", devices->size);
		return 1;
	}

	dev = &devices->device[devices->count];
	dev->name = strdup(device);
	dev->short_name = basename(strdup(device));
	strcpy(dev->info.name, WDAEMON_DEVNAME_PREFIX);

	init(&dev->info);
	dev->info.priv = priv;

	if (uinput_create(&dev->info))
		return 1;
	dev->tries = 0;
	dev->plugged = 0;
	devices->count++;

	return 0;
}

struct monitored_devices *monitored_devices_init(int count)
{
	struct monitored_devices *devices;
	int i;

	devices = calloc(1, sizeof(struct monitored_devices));
	if (devices == NULL)
		goto out;

	devices->device = calloc(count, sizeof(struct monitored_device));
	if (devices->device == NULL)
		goto free_devices;

	devices->count = 0;
	devices->size = count;
	for (i = 0; i < count; i++)
		devices->device[i].fd = -1;

	devices->ufds = calloc(count, sizeof(struct pollfd));
	if (devices->ufds == NULL)
		goto free_device;

out:
	return devices;
free_device:
	free(devices->device);
free_devices:
	free(devices);
	goto out;
}

static struct monitored_device *monitored_device_find(struct monitored_devices *devices,
						      char *name)
{
	struct monitored_device *dev;
	int i;

	for (i = 0; i < devices->count; i++) {
		dev = &devices->device[i];
		if (!strcmp(dev->name, name))
			return dev;
	}
	return NULL;
}

int monitored_device_plug(struct monitored_devices *devices, char *name)
{
	struct monitored_device *dev;

	dev = monitored_device_find(devices, name);
	if (dev == NULL) {
		log(LOG_ERR, "BUG: monitored_device_plug: unable to find "
		    "the device %s\n", name);
		exit(1);
	}

	dev->plugged = 1;
	if (input_add_device(devices, dev))
		return 1;

	return 0;
}

int monitored_device_unplug(struct monitored_devices *devices, char *name)
{
	struct monitored_device *dev;

	dev = monitored_device_find(devices, name);
	if (dev == NULL) {
		/* bug */
		return 1;
	}

	dev->plugged = 0;
	if (input_remove_device(devices, dev)) {
		/* another error */
		return 1;
	}

	return 0;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
