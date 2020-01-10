/*
 *    Copyright © 2011 Red Hat, Inc.
 *
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
 *    along with wademon; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "session.h"

int evemu_extract_device(struct session *session, const char *path, const char *output_file);
int evemu_load_device(struct session *session,
		      const char *input_file,
		      char *path);

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
