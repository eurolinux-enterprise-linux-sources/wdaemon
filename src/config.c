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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "log.h"
#include "wdaemon-config.h"
#include "session.h"
#include "device_types.h"
#include "debug.h"

int use_config_file(char *file, struct session *session, int *devices)
{
	char key[15], value[251];
	FILE *f;
	int n, line = 1;

	f = fopen(file, "r");
	if (f == NULL) {
		log(LOG_ERR, "Error opening configuration file %s: %s\n",
		    file, strerror(errno));
		return errno;
	}

	n = fscanf(f, "%12s = %250s", key, value);
	while (n != EOF) {
		dprint(2, "Did %i conversions\n", n);
		if (n < 2) {
			dprint(2, "Ignoring line %i\n", line);
			goto next;
		}

		dprint(2, "Got key [%s], value [%s]\n", key, value);
		if (!strcmp(key, "device")) {
			char type[4], filename[150], *pos;
			int size, t;
			pos = strchr(value, ',');
			if (pos == NULL) {
				log(LOG_ERR, "Parse error on device line: "
				    "format should be\ndevice = <number>,"
				    "<path>\n");
				fclose(f);
				return 1;
			}
			memset(type, 0, sizeof(type));
			size = pos - value;
			if (size > 3)
				size = 3;
			memcpy(type, value, size);

			t = atoi(type);
			snprintf(filename, sizeof(filename), "%s", pos + 1);
			dprint(2, "Got device type %i and filename %s\n",
			       t, filename);

			if (add_device(session, filename, t)) {
				log(LOG_ERR, "Error adding device\n");
				return 1;
			}
			*devices = *devices + 1;
		}
		else if (!strcmp(key, "debug")) {
			int level = atoi(value);
			log(LOG_INFO, "config: Setting debug level to %i\n",
			    level);
			set_debug_level(level);
		} else if (!strcmp(key, "description")) {
			char *pos;
			char desc[126] = {0},
			     path[126] = {0};

			pos = strchr(value, ',');
			if (pos == NULL) {
				fprintf(stderr, "Parse error on device line: "
					"format should be\ndevice = <number>,"
					"<path>\n");
				fclose(f);
				return 1;
			}
			strncpy(desc, value, pos - value);
			strcpy(path, ++pos);

			dprint(2, "Got desc '%s' and filename %s\n", desc, path);

			if (add_device_desc(session, path, desc)) {
				fprintf(stderr, "Error adding device description\n");
				return 1;
			}
			*devices = *devices + 1;
		}
		else {
			log(LOG_ERR, "Invalid key [%s] on line %i\n", key,
			    line);
			fclose(f);
			return 1;
		}

next:
		n = fscanf(f, "%10s = %150s", key, value);
		line++;
	}
	fclose(f);

	return 0;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
