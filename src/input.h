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

#ifndef INPUT_H
#define INPUT_H
struct session;
int input_init(struct monitored_devices *devices);
int input_add_device(struct monitored_devices *devices, struct monitored_device *dev);
int input_remove_device(struct monitored_devices *devices, struct monitored_device *dev);
int input_poll(struct session *session);
#endif	/* INPUT_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
