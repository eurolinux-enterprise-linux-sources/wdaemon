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

#include <dirent.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#define __USE_GNU
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "log.h"
#include "session.h"
#include "hotplug.h"
#include "monitored.h"
#include "input.h"
#include "debug.h"

static int pipe_fd = -1;

static void notify_change(void)
{
	if (write(pipe_fd, "a", 1) < 0)
		fprintf(stderr, "Error writing pipe: %s\n", strerror(errno));
}

int hotplug_get_pipe_fd(struct session *session)
{
	struct hotplug *h = session->hotplug;
	return h->pipe[0];
}

static int hotplug_check_directory(struct hotplug_dir *hdir)
{
	hdir->fd = open(hdir->name, O_RDONLY);
	if (hdir->fd < 0) {
		dprint(2, "hotplug: delaying initialization of %s to later as"
			" it's not available now\n", hdir->name);
		hdir->delayed = 1;
		return 0;
	}
	hdir->delayed = 0;
	if (fcntl(hdir->fd, F_SETSIG, SIGRTMIN + 1)) {
		log(LOG_ERR, "Error setting new signal code for directory "
		    "change: %s\n", strerror(errno));
		close(hdir->fd);
		return 1;
	}
	if (fcntl(hdir->fd, F_NOTIFY, DN_CREATE | DN_DELETE |
					   DN_MULTISHOT)) {
		log(LOG_ERR, "Error enabling dnotify: %s\n", strerror(errno));
		close(hdir->fd);
		return 1;
	}
	return 0;
}

void dnotify_do_pending_work(struct session *session)
{
	struct stat buff;
	struct hotplug *h = session->hotplug;
	struct hotplug_dir *hdir;
	struct monitored_devices *devices = session->devices;
	struct monitored_device *dev;
	int i, retval;
	char b;

	dprint(2, "dnotify_do_pending_work: checking for changes for %i "
		"devices\n", devices->count);

	/* read everything from the buffer */
	while(read(h->pipe[0], &b, 1) > 0)
		;

	/* scan for missing directories and delayed ones */
	for (i = 0; i < h->dir_count; i++) {
		hdir = &h->dirs[i];

		if (hdir->delayed == 0 && hdir->fd >= 0) {
			retval = stat(hdir->name, &buff);
			if (retval) {
				dprint(2, "directory %s is gone\n",
					hdir->name);
				close(hdir->fd);
				hdir->fd = -1;
				hdir->delayed = 1;
			}
			continue;
		}
		dprint(3, "checking for delayed directory %s\n", hdir->name);
		/* ignore result and try again next time */
		if (hotplug_check_directory(hdir))
			notify_change();
	}

	for (i = 0; i < devices->count; i++) {
		dev = &devices->device[i];
		retval = stat(dev->name, &buff);
		if (retval == 0) {
			if (dev->plugged == 0) {
				dprint(1, "monitored device %s attached\n",
				       dev->name);
				dev->pending_action = ADD;
			}
			else
				dprint(4, "monitored device %s still "
				       "available\n", dev->name);
		}
		if (retval == -1) {
			if (errno == ENOENT && dev->plugged == 1) {
				/* device was removed */
				dprint(1, "monitored device %s disconnected\n",
				       dev->name);
				dev->pending_action = REMOVE;
			}
			else
				dprint(4, "error %i (%s) while looking for "
				       "%s\n", errno, strerror(errno),
				       dev->name);
		}
	}
}

void dnotify_handler(int sig, siginfo_t *si, void *data)
{
	notify_change();
}

int hotplug_init(struct session *session)
{
	struct hotplug *h;
	struct hotplug_dir *hdir;
	long flags;

	h = (struct hotplug *)malloc(sizeof(struct hotplug));
	if (h == NULL) {
		log(LOG_ERR, "hotplug_init: not enough memory\n");
		return -1;
	}
	memset(h, 0, sizeof(struct hotplug));
	session->hotplug = h;

	if (pipe(h->pipe)) {
		log(LOG_ERR, "pipe error: %s\n", strerror(errno));
		return -1;
	}
	pipe_fd = h->pipe[1];
	flags = fcntl(pipe_fd, F_GETFL);
	if (fcntl(pipe_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		log(LOG_ERR, "Error setting the pipe write file descriptor "
		    "flags: %s\n", strerror(errno));
		return -1;
	}
	flags = fcntl(h->pipe[0], F_GETFL);
	if (fcntl(h->pipe[0], F_SETFL, flags | O_NONBLOCK) < 0) {
		log(LOG_ERR, "Error setting the pipe write file descriptor "
		    "flags: %s\n", strerror(errno));
		return -1;
	}

	/* initialize with pending work to do: we need to scan the already
	 * plugged devices */
	notify_change();

	/* add /dev/input as default */
	h->dir_count = 1;
	hdir = &h->dirs[0];
	hdir->name = "/dev/input";
	hdir->delayed = 1;

	dprint(2, "hotplug_init: initialized\n");

	return 0;
}

int hotplug_add_file(struct session *session, char *file)
{
	char *dir;
	struct hotplug *h = session->hotplug;
	struct hotplug_dir *hdir;
	int i;

	dir = dirname(strdup(file));

	for (i = 0; i < h->dir_count; i++) {
		hdir = &h->dirs[i];
		if (hdir->name == NULL)
			break;
		if (!strcmp(hdir->name, dir)) {
			hdir->users++;
			free(dir);
			return 0;
		}
	}
	if (h->dir_count == HOTPLUG_MAX_DIRS) {
		log(LOG_ERR, "hotplug_add_file: too many directories, "
			"adjust HOTPLUG_MAX_DIRS\n");
		return -1;
	}
	hdir = &h->dirs[h->dir_count];
	memset(hdir, 0, sizeof(struct hotplug_dir));
	hdir->name = dir;
	hdir->delayed = 1;
	h->dir_count++;
	dprint(2, "hotplug_add_file: added directory %s\n", hdir->name);

	return 0;
}

int hotplug_start(struct session *session)
{
	struct sigaction act;

	act.sa_sigaction = dnotify_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	return sigaction(SIGRTMIN + 1, &act, NULL);
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
