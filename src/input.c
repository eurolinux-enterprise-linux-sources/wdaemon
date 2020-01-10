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
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <linux/input.h>
#include "log.h"
#include "session.h"
#include "monitored.h"
#include "input.h"
#include "uinput.h"
#include "hotplug.h"
#include "debug.h"

int input_init(struct monitored_devices *devices)
{
	return 0;
}

int input_add_device(struct monitored_devices *devices, struct monitored_device *dev)
{
	int retval;

	dprint(1, "Opening device %s\n", dev->name);

	if (dev->tries > MAX_ADD_TRIES) {
		log(LOG_WARNING, "Giving up on device %s, unable to open it "
		    "after %i tries\n", dev->name, MAX_ADD_TRIES);
		dev->tries = 0;
		dev->pending_action = NONE;
		dev->plugged = 0;
		dev->fd = -1;
		return 1;
	}
	dev->tries++;
	/* if the device is already active, close and reinit it */
	if (dev->fd >= 0)
		close(dev->fd);

	dev->fd = open(dev->name, O_RDWR);
	if (dev->fd < 0) {
		log(LOG_WARNING, "Unable to open file just created "
			"%s (%s) ", dev->name, strerror(errno));
		return 1;
	}

	/* grab the device */
	retval = ioctl(dev->fd, EVIOCGRAB, 1);
	if (retval < 0) {
		log(LOG_WARNING, "Unable to grab device %s (%s)\n", dev->name,
			strerror(errno));
		close(dev->fd);
		dev->fd = -1;
		return 1;
	}

	dev->tries = 0;
	dev->pending_action = NONE;
	dev->plugged = 1;

	return 0;
}

int input_remove_device(struct monitored_devices *devices, struct monitored_device *dev)
{
	dev->pending_action = NONE;
	dev->plugged = 0;
	dev->tries = 0;

	dprint(1, "Closing device %s\n", dev->name);

	if (dev->fd >= 0) {
		close(dev->fd);
		dev->fd = -1;
	}

	return 0;
}

static int input_process(struct monitored_device *dev)
{
	struct input_event ev;
	int retval, flags;

	/* switch to non blocking mode */
	retval = fcntl(dev->fd, F_GETFL);
	if (retval == -1) {
		log(LOG_WARNING, "Unable to get file flags from %s: %s\n",
			dev->name, strerror(errno));
		return 1;
	}
	flags = retval;
	retval = fcntl(dev->fd, F_SETFL, flags | O_NONBLOCK);
	if (retval == -1) {
		log(LOG_WARNING, "Unable to set file as non blocking: %s (%s)\n",
			dev->name, strerror(errno));
		return 1;
	}

	/* read and process all data */
	while((retval = read(dev->fd, &ev, sizeof(ev))) == sizeof(ev))
		if (uinput_write_event(&dev->info, &ev))
			break;

	if (retval == -EBADF || retval == -ENODEV)
		return 1;

	retval = fcntl(dev->fd, F_SETFL, flags);
	if (retval == -1) {
		log(LOG_WARNING, "Unable to set back the file's flags: %s (%s)\n",
			dev->name, strerror(errno));
		return 1;
	}

	return 0;
}

int input_poll(struct session *session)
{
	struct monitored_devices *devices = session->devices;
	struct monitored_device *dev;
	int retval, i, j;
	short events;

	memset(devices->ufds, 0, sizeof(devices->ufds));
	for (i = 0, j = 1; i < devices->count; i++) {
		dev = &devices->device[i];

		if (dev->pending_action == ADD)
			input_add_device(devices, dev);
		else if (dev->pending_action == REMOVE)
			input_remove_device(devices, dev);

		if (dev->plugged == 0)
			continue;
		devices->ufds[j].fd = dev->fd;
		devices->ufds[j].events = POLLIN | POLLPRI;
		j++;
	}
	/* the hotplug pipe is always there */
	devices->ufds[0].fd = hotplug_get_pipe_fd(session);
	devices->ufds[0].events = POLLIN | POLLPRI;

	/* pool for device input */
	dprint(10, "Polling start, %i entries\n", j);
	retval = poll(devices->ufds, j, -1);

	if (retval == 0) {
		dprint(10, "poll: no entries\n");
		return 0;
	}

	if (retval == -1) {
		dprint(5, "poll returned error %i (%s)\n", errno,
			strerror(errno));

		switch(errno) {
		case EBADF:
		case EINTR:
			/* we just go ahead and look what happened */
			break;
		default:
			log(LOG_ERR, "Error while polling for events: %s\n",
			    strerror(errno));
			return 1;
		}
	}

	/* first, check if we got something from hotplug */
	events = devices->ufds[0].revents;
	if (events & POLLNVAL || events & POLLERR) {
		log(LOG_ERR, "The hotplug pipe is gone!\n");
		return 1;
	}

	if (events & POLLIN || events & POLLPRI) {
		dprint(10, "input directory change reported\n");
		dnotify_do_pending_work(session);
	}

	/* find the file descriptor on list */
	for (i = 0, j = 1; i < devices->count; i++) {
		dev = &devices->device[i];

		if (dev->plugged == 0)
			continue;

		events = devices->ufds[j].revents;

		if (events & POLLERR)
			/* device was removed */
			input_remove_device(devices, dev);
		else if (events & POLLIN || events & POLLPRI) {
			dprint(10, "Data incoming from %s\n", dev->name);
			if (input_process(dev))
				input_remove_device(devices, dev);
		}
		j++;
	}

	return 0;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
