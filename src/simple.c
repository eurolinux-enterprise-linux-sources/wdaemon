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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "log.h"
#include "session.h"
#include "monitored.h"
#include "hotplug.h"
#include "uinput.h"
#include "input.h"
#include "wdaemon-config.h"
#include "debug.h"
#include "device_types.h"
#include "wacom.h"
#include "wdaemon-evemu.h"

#define MAX_DEVICES 15

static char *options = "p:t:wd:c:fo:asvhx:l:";

static void do_help(void)
{
	printf("wdaemon version %s\n", VERSION);
	printf("wdaemon -p <device file> -t <type> ... options\n");
	printf("wdaemon -w\n");
	printf("wdaemon -a\n");
	printf("wdaemon -v\n");
	printf("wdaemon -h\n");
	printf("options:\n");
	printf("\t-p <device file>\tdevice to monitor\n");
	printf("\t-t <type>\t\ttype of the device, use -w to get a list\n");
	printf("\t-c <file>\t\tuse <file> as configuration file\n");
	printf("\t-d <level>\t\tenables debug messages up to <level>\n");
	printf("\t-o <file>\t\tredirect debug messages to file <file>\n");
	printf("\t-s\t\t\tuse syslog for debug messages, conflict with -o\n");
	printf("\t-f\t\t\tfork and enter in daemon mode\n");
	printf("\n");
	printf("\t-w\t\t\tget a list of supported devices and their numbers\n");
	printf("\n");
	printf("\t-a\t\t\tgenerate a configuration file based on plugged "
		"tablets\n");
	printf("\n");
	printf("\t-v\t\t\tprints the version\n");
	printf("\t-h\t\t\tprints this usage help\n");
	printf("\t-l <file>\t\tload device description from <file>\n");
	printf("\t-x <file>\t\textract device description into <file>\n");
	printf("\nUse UINPUT_DEVICE (default %s, current %s) environment variable \n"
		"to set uinput device file\n", UINPUT_DEVICE, getenv("UINPUT_DEVICE"));
}

static char *default_output_file = "/dev/null";
static char *output_file;
int main(int argc, char *argv[])
{
	struct session session;
	char *path = NULL;
	int type = -1, retval, devices, daemon = 0;
	char *input_file = NULL;
	int extract = 0, load = 0;

	output_file = NULL;
	openlog("wdaemon", 0, LOG_DAEMON);
	set_log_stdout();

	session_init(&session);

	session.devices = monitored_devices_init(MAX_DEVICES);
	if (session.devices == NULL)
		return ENOMEM;

	if (input_init(session.devices))
		return ENOMEM;

	if ((retval = hotplug_init(&session)))
		return retval;
	if (hotplug_start(&session)) {
		log(LOG_ERR, "Unable to start directory monitoring");
		return 1;
	}

	devices = 0;
	while (1) {
		retval = getopt(argc, argv, options);
		switch (retval) {
			/* options that immediately return */
			case 'w':
				set_log_stdout();
				print_device_types();
				return 0;
			case 'a':
				set_log_stdout();
				return wacom_autoconfigure();
			case 'v':
				printf("wdaemon version %s\n", VERSION);
				return 0;
			case 'h':
				do_help();
				return 0;
			case 'p':
				path = optarg;
				break;
			case 't':
				type = atoi(optarg);
				break;

			/* options that just change runtime settings */
			case 'o':
				output_file = strdup(optarg);
				set_syslog(0);
				break;
			case 'd':
			{
				int level = atoi(optarg);
				log(LOG_INFO, "Setting debug level to %i\n", level);
				set_debug_level(level);
				break;
			}
			case 'f':
				set_log_syslog();
				daemon = 1;
				break;
			case 's':
				if (output_file) {
					free(output_file);
					output_file = NULL;
				}
				set_syslog(1);
				break;

			/* other options */
			case 'c':
				if (use_config_file(optarg, &session,
						    &devices))
					return 1;
				break;

			case 'l':
				input_file = strdup(optarg);
				load = 1;
				break;
			case 'x':
				output_file = strdup(optarg);
				extract = 1;
				break;
			default:
				break;
		}
		if (retval == -1)
			break;
	}

	if (extract)
		return evemu_extract_device(&session, path, output_file);

	if (load) {
		if (evemu_load_device(&session, input_file, path))
				    return 1;
		devices++;
	}

	if (type != -1 && path != NULL)
	{
		if (add_device(&session, path, type))
			return 1;
		devices++;
		type = -1;
		path = NULL;
	}

	if (devices == 0) {
		log(LOG_ERR, "%s: no devices were specified!\n", argv[0]);
		do_help();
		return 1;
	}

	if (daemon) {
		daemon = fork();
		if (daemon < 0) {
			log(LOG_ERR, "Error forking: %s\n", strerror(errno));
			return 1;
		}
		else if (daemon) {
			dprint(1, "Forked to a child's process, exiting\n");
			return 0;
		}

		/* we need to restart so the signal handler can be
		 * reinstalled */
		if (hotplug_start(&session)) {
			log(LOG_ERR, "Unable to start directory monitoring: %s\n",
			    strerror(errno));
			return 1;
		}
	}

	if (daemon || output_file != default_output_file ||
	    !strcmp(output_file, "-")) {
		if (freopen(output_file, "a+", stdout) == NULL) {
			log(LOG_ERR, "Unable to open file for output: %s\n",
			    strerror(errno));
			return 1;
		}
		if (freopen(output_file, "a+", stderr) == NULL) {
			log(LOG_ERR, "Unable to open file for output: %s\n",
			    strerror(errno));
			return 1;
		}
	}

	while(1)
		if (input_poll(&session)) {
			log(LOG_ERR, "Failed to poll for device events\n\n");
			return errno;
		}


	return 0;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
