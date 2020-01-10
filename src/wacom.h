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

#ifndef WACOM_H
#define WACOM_H
struct uinput_info;
struct uinput_user_dev;
struct session;
struct wacom_priv {
	int type;
};
void wacom_init(struct uinput_info *info);
int add_wacom_device(struct session *session, char *path, int type);
void wacom_print_device_type(int index);
int wacom_autoconfigure(void);
int wacom_check_type(int x);
int wacom_get_n_devices(void);
#endif	/* WACOM_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
