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

#ifndef HOTPLUG_H
#define HOTPLUG_H
#define HOTPLUG_MAX_DIRS 10
struct session;
struct monitored_devices;
struct hotplug {
	struct hotplug_dir {
		int fd;
		char *name;
		int users;
		int delayed;
	} dirs[HOTPLUG_MAX_DIRS];
	int dir_count;
	int pipe[2];
};
int hotplug_init(struct session *session);
int hotplug_start(struct session *session);
void dnotify_do_pending_work(struct session *session);
int hotplug_add_file(struct session *session, char *file);
int hotplug_get_pipe_fd(struct session *session);
#endif	/* HOTPLUG_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
