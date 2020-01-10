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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <glob.h>
#include "log.h"
#include "monitored.h"
#include "input.h"
#include "uinput.h"
#include "session.h"
#include "hotplug.h"
#include "wacom.h"
#include "device_types.h"


int add_wacom_device(struct session *session, char *path, int type)
{
	struct wacom_priv *priv = malloc(sizeof(struct wacom_priv));
	int rc;

	if (priv == NULL)
		return ENOMEM;

	if (wacom_check_type(type)) {
		log(LOG_ERR, "Invalid wacom type: %i, use -w to get a "
		    "list\n", type);
		return EINVAL;
	}

	priv->type = type;

	rc = hotplug_add_file(session, path);
	if (rc)
		return rc;

	return monitored_device_add(session->devices, wacom_init, priv, path);
}

#define set_event(x, y, z) do { if (ioctl((x)->fd, (y), (z)) == -1) { \
					log(LOG_ERR, "Error enabling %s (%s)\n", \
					    #z, strerror(errno)); \
					return 1; \
				} \
			} while(0)
#define wacom_priv(x) ((struct wacom_priv *)(x->priv))

enum {
	PENPARTNER = 0,
	GRAPHIRE,
	G4,
	PTU,
	PL,
	INTUOS,
	INTUOS3S,
	INTUOS3,
	INTUOS3L,
	CINTIQ,
	BEE,
	MO,
	TABLETPC,
	INTUOS4S,
	INTUOS4,
	INTUOS4L,
	WACOM_21UX2,
	DTU,
	MAX_TYPE
};

static struct wacom_features {
	char *name;
	int x_max;
	int y_max;
	int pressure_max;
	int distance_max;
	int type;
	int product_id;
} wacom_features[] = {
	{ "Wacom Penpartner",      5040,  3780,  255,  0, PENPARTNER, 0x00 },
	{ "Wacom Graphire",       10206,  7422,  511, 63, GRAPHIRE,   0x10 },
	{ "Wacom Graphire2 4x5",  10206,  7422,  511, 63, GRAPHIRE,   0x11 },
	{ "Wacom Graphire2 5x7",  13918, 10206,  511, 63, GRAPHIRE,   0x12 },
	{ "Wacom Graphire3",      10208,  7424,  511, 63, GRAPHIRE,   0x13 },
	{ "Wacom Graphire3 6x8",  16704, 12064,  511, 63, GRAPHIRE,   0x14 },
	{ "Wacom Graphire4 4x5",  10208,  7424,  511, 63, G4,         0x15 },
	{ "Wacom Graphire4 6x8",  16704, 12064,  511, 63, G4,         0x16 },
	{ "Wacom Volito",          5104,  3712,  511, 63, GRAPHIRE,   0x60 },
	{ "Wacom PenStation2",     3250,  2320,  255, 63, GRAPHIRE,   0x61 },
	{ "Wacom Volito2 4x5",     5104,  3712,  511, 63, GRAPHIRE,   0x62 },
	{ "Wacom Volito2 2x3",     3248,  2320,  511, 63, GRAPHIRE,   0x63 },
	{ "Wacom PenPartner2",     3250,  2320,  255, 63, GRAPHIRE,   0x64 },
	{ "Wacom Intuos 4x5",     12700, 10600, 1023, 31, INTUOS,     0x20 },
	{ "Wacom Intuos 6x8",     20320, 16240, 1023, 31, INTUOS,     0x21 },
	{ "Wacom Intuos 9x12",    30480, 24060, 1023, 31, INTUOS,     0x22 },
	{ "Wacom Intuos 12x12",   30480, 31680, 1023, 31, INTUOS,     0x23 },
	{ "Wacom Intuos 12x18",   45720, 31680, 1023, 31, INTUOS,     0x24 },
	{ "Wacom PL400",           5408,  4056,  255,  0, PL,         0x30 },
	{ "Wacom PL500",           6144,  4608,  255,  0, PL,         0x31 },
	{ "Wacom PL600",           6126,  4604,  255,  0, PL,         0x32 },
	{ "Wacom PL600SX",         6260,  5016,  255,  0, PL,         0x33 },
	{ "Wacom PL550",           6144,  4608,  511,  0, PL,         0x34 },
	{ "Wacom PL800",           7220,  5780,  511,  0, PL,         0x35 },
	{ "Wacom PL700",           6758,  5406,  511,  0, PL,         0x37 },
	{ "Wacom PL510",           6282,  4762,  511,  0, PL,         0x38 },
	{ "Wacom DTU710",         34080, 27660,  511,  0, PL,         0x39 },
	{ "Wacom DTF521",          6282,  4762,  511,  0, PL,         0xC0 },
	{ "Wacom DTF720",          6858,  5506,  511,  0, PL,         0xC4 },
	{ "Wacom Cintiq Partner", 20480, 15360,  511,  0, PTU,        0x03 },
	{ "Wacom Intuos2 4x5",    12700, 10600, 1023, 31, INTUOS,     0x41 },
	{ "Wacom Intuos2 6x8",    20320, 16240, 1023, 31, INTUOS,     0x42 },
	{ "Wacom Intuos2 9x12",   30480, 24060, 1023, 31, INTUOS,     0x43 },
	{ "Wacom Intuos2 12x12",  30480, 31680, 1023, 31, INTUOS,     0x44 },
	{ "Wacom Intuos2 12x18",  45720, 31680, 1023, 31, INTUOS,     0x45 },
	{ "Wacom Intuos3 4x5",    25400, 20320, 1023, 63, INTUOS3S,   0xB0 },
	{ "Wacom Intuos3 6x8",    40640, 30480, 1023, 63, INTUOS3,    0xB1 },
	{ "Wacom Intuos3 9x12",   60960, 45720, 1023, 63, INTUOS3,    0xB2 },
	{ "Wacom Intuos3 12x12",  60960, 60960, 1023, 63, INTUOS3L,   0xB3 },
	{ "Wacom Intuos3 12x19",  97536, 60960, 1023, 63, INTUOS3L,   0xB4 },
	{ "Wacom Intuos3 6x11",   54204, 31750, 1023, 63, INTUOS3,    0xB5 },
	{ "Wacom Cintiq 21UX",    87200, 65600, 1023, 63, CINTIQ,     0x3F },
	{ "Wacom Intuos2 6x8",    20320, 16240, 1023, 31, INTUOS,     0x47 },
	{ "Wacom Intuos3 4x6",    31496, 19685, 1023, 63, INTUOS3S,   0xB7 },
	{ "Wacom Bamboo",         14760,  9225,  511, 63, MO,         0x65 },
	{ "Wacom Cintiq 12WX",    53020, 33440, 1023, 63, BEE,        0xC6 },
	{ "Wacom Cintiq 20WSX",   86680, 54180, 1023, 63, BEE,        0xC5 },
	{ "Wacom BambooFun 4x5",  14760,  9225,  511, 63, MO,         0x17 },
	{ "Wacom BambooFun 6x8",  21648, 13530,  511, 63, MO,         0x18 },
	{ "Wacom Bamboo1",         5104,  3712,  511, 63, GRAPHIRE,   0x69 },
	{ "Wacom Bamboo1 Medium", 16704, 12064,  511, 63, GRAPHIRE,   0x19 },
	{ "Wacom DTU1931",        37832, 30305,  511,  0, PL,         0xC7 },
	{ "Wacom ISDv4 90",       26202, 16325,  255,  0, TABLETPC,   0x90 },
	{ "Wacom ISDv4 93",       26202, 16325,  255,  0, TABLETPC,   0x93 },
	{ "Wacom ISDv4 9A",       26202, 16325,  255,  0, TABLETPC,   0x9A },
	{ "Wacom Intuos4 4x6",    31496, 19685, 2047, 63, INTUOS4S,   0xB8 },
	{ "Wacom Intuos4 6x9",    44704, 27940, 2047, 63, INTUOS4,    0xB9 },
	{ "Wacom Intuos4 8x13",   65024, 40640, 2047, 63, INTUOS4L,   0xBA },
	{ "Wacom Intuos4 12x19",  97536, 60960, 2047, 63, INTUOS4L,   0xBB },
	{ "Wacom Cintiq 21UX2",   87200, 65600, 2047, 63, WACOM_21UX2,0xCC },
	{ "Wacom DTU1631",        34623, 19553, 511,   0, DTU,        0xF0 },
	{ "Wacom DTU2231",        47864, 27011, 511,   0, DTU,        0xCE },
};
#define WACOM_N_TABLETS (sizeof(wacom_features)/sizeof(wacom_features[0]))
int wacom_check_type(int x)
{
	return (x > WACOM_N_TABLETS);
}

int wacom_get_n_devices(void)
{
	return WACOM_N_TABLETS;
}

static int wacom_set_events(struct uinput_info *info, struct uinput_user_dev *dev)
{
	struct wacom_priv *priv = wacom_priv(info);
	struct wacom_features *features = &wacom_features[priv->type];

	/* common */
	set_event(info, UI_SET_EVBIT, EV_KEY);
	set_event(info, UI_SET_EVBIT, EV_ABS);

	set_event(info, UI_SET_KEYBIT, BTN_TOOL_PEN);
	set_event(info, UI_SET_KEYBIT, BTN_TOUCH);
	set_event(info, UI_SET_KEYBIT, BTN_STYLUS);

	set_event(info, UI_SET_ABSBIT, ABS_MISC);

	switch(features->type) {
	case MO:
		set_event(info, UI_SET_KEYBIT, BTN_1);
		set_event(info, UI_SET_KEYBIT, BTN_5);
		/* fall through */
	case G4:
		set_event(info, UI_SET_EVBIT, EV_MSC);

		set_event(info, UI_SET_MSCBIT, MSC_SERIAL);

		set_event(info, UI_SET_KEYBIT, BTN_TOOL_FINGER);
		set_event(info, UI_SET_KEYBIT, BTN_0);
		set_event(info, UI_SET_KEYBIT, BTN_4);
		/* fall through */
	case GRAPHIRE:
		set_event(info, UI_SET_EVBIT, EV_REL);

		set_event(info, UI_SET_RELBIT, REL_WHEEL);

		set_event(info, UI_SET_KEYBIT, BTN_LEFT);
		set_event(info, UI_SET_KEYBIT, BTN_RIGHT);
		set_event(info, UI_SET_KEYBIT, BTN_MIDDLE);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_RUBBER);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_MOUSE);
		set_event(info, UI_SET_KEYBIT, BTN_STYLUS2);
		break;
	case WACOM_21UX2:
		set_event(info, UI_SET_KEYBIT, BTN_A);
		set_event(info, UI_SET_KEYBIT, BTN_B);
		set_event(info, UI_SET_KEYBIT, BTN_C);
		set_event(info, UI_SET_KEYBIT, BTN_X);
		set_event(info, UI_SET_KEYBIT, BTN_Y);
		set_event(info, UI_SET_KEYBIT, BTN_Z);
		set_event(info, UI_SET_KEYBIT, BTN_BASE);
		set_event(info, UI_SET_KEYBIT, BTN_BASE2);
		/* fall thru */
	case BEE:
		set_event(info, UI_SET_KEYBIT, BTN_8);
		set_event(info, UI_SET_KEYBIT, BTN_9);
	case INTUOS3:
	case INTUOS3L:
	case CINTIQ: 
		set_event(info, UI_SET_KEYBIT, BTN_4);
		set_event(info, UI_SET_KEYBIT, BTN_5);
		set_event(info, UI_SET_KEYBIT, BTN_6);
		set_event(info, UI_SET_KEYBIT, BTN_7);
		/* fall thru */
	case INTUOS3S:
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_FINGER);
		set_event(info, UI_SET_KEYBIT, BTN_0);
		set_event(info, UI_SET_KEYBIT, BTN_1);
		set_event(info, UI_SET_KEYBIT, BTN_2);
		set_event(info, UI_SET_KEYBIT, BTN_3);
		set_event(info, UI_SET_ABSBIT, ABS_Z);
		/* fall thru */
	case INTUOS:
		set_event(info, UI_SET_EVBIT, EV_MSC);
		set_event(info, UI_SET_EVBIT, EV_REL);

		set_event(info, UI_SET_MSCBIT, MSC_SERIAL);

		set_event(info, UI_SET_RELBIT, REL_WHEEL);

		set_event(info, UI_SET_KEYBIT, BTN_LEFT);
		set_event(info, UI_SET_KEYBIT, BTN_RIGHT);
		set_event(info, UI_SET_KEYBIT, BTN_MIDDLE);
		set_event(info, UI_SET_KEYBIT, BTN_SIDE);
		set_event(info, UI_SET_KEYBIT, BTN_EXTRA);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_RUBBER);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_MOUSE);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_BRUSH);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_PENCIL);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_AIRBRUSH);
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_LENS);
		set_event(info, UI_SET_KEYBIT, BTN_STYLUS2);

		break;
	case TABLETPC:
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_DOUBLETAP);
		/* fall thru */
	case PL:
	case PTU:
	case DTU:
		set_event(info, UI_SET_KEYBIT, BTN_STYLUS2);
		/* fall thru */
	case PENPARTNER:
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_RUBBER);
		break;
	case INTUOS4:
	case INTUOS4L:
		set_event(info, UI_SET_ABSBIT, ABS_Z);
		set_event(info, UI_SET_KEYBIT, BTN_7);
		set_event(info, UI_SET_KEYBIT, BTN_8);
		/* fall trhu */
	case INTUOS4S:
		set_event(info, UI_SET_KEYBIT, BTN_TOOL_FINGER);
		set_event(info, UI_SET_KEYBIT, BTN_0);
		set_event(info, UI_SET_KEYBIT, BTN_1);
		set_event(info, UI_SET_KEYBIT, BTN_2);
		set_event(info, UI_SET_KEYBIT, BTN_3);
		set_event(info, UI_SET_KEYBIT, BTN_4);
		set_event(info, UI_SET_KEYBIT, BTN_5);
		set_event(info, UI_SET_KEYBIT, BTN_6);
		break;
	}

	set_event(info, UI_SET_EVBIT, EV_KEY);
	set_event(info, UI_SET_EVBIT, EV_ABS);
	set_event(info, UI_SET_EVBIT, EV_REL);
	set_event(info, UI_SET_EVBIT, EV_SYN);
	set_event(info, UI_SET_EVBIT, EV_MSC);

	set_event(info, UI_SET_KEYBIT, BTN_TOOL_PEN);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_PENCIL);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_BRUSH);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_AIRBRUSH);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_RUBBER);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_MOUSE);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_LENS);
	set_event(info, UI_SET_KEYBIT, BTN_TOOL_FINGER);
	set_event(info, UI_SET_KEYBIT, BTN_TOUCH);
	set_event(info, UI_SET_KEYBIT, BTN_STYLUS);
	set_event(info, UI_SET_KEYBIT, BTN_STYLUS2);
	set_event(info, UI_SET_KEYBIT, BTN_RIGHT);
	set_event(info, UI_SET_KEYBIT, BTN_LEFT);
	set_event(info, UI_SET_KEYBIT, BTN_MIDDLE);
	set_event(info, UI_SET_KEYBIT, BTN_SIDE);
	set_event(info, UI_SET_KEYBIT, BTN_EXTRA);
	set_event(info, UI_SET_KEYBIT, BTN_0);
	set_event(info, UI_SET_KEYBIT, BTN_1);
	set_event(info, UI_SET_KEYBIT, BTN_2);
	set_event(info, UI_SET_KEYBIT, BTN_3);
	set_event(info, UI_SET_KEYBIT, BTN_4);
	set_event(info, UI_SET_KEYBIT, BTN_5);
	set_event(info, UI_SET_KEYBIT, BTN_6);
	set_event(info, UI_SET_KEYBIT, BTN_7);
	set_event(info, UI_SET_KEYBIT, BTN_MIDDLE);

	set_event(info, UI_SET_ABSBIT, ABS_X);
	set_event(info, UI_SET_ABSBIT, ABS_Y);
	set_event(info, UI_SET_ABSBIT, ABS_RX);
	set_event(info, UI_SET_ABSBIT, ABS_RY);
	set_event(info, UI_SET_ABSBIT, ABS_RZ);
	set_event(info, UI_SET_ABSBIT, ABS_TILT_X);
	set_event(info, UI_SET_ABSBIT, ABS_TILT_Y);
	set_event(info, UI_SET_ABSBIT, ABS_PRESSURE);
	set_event(info, UI_SET_ABSBIT, ABS_DISTANCE);
	set_event(info, UI_SET_ABSBIT, ABS_WHEEL);
	set_event(info, UI_SET_ABSBIT, ABS_THROTTLE);
	set_event(info, UI_SET_ABSBIT, ABS_MISC);

	set_event(info, UI_SET_RELBIT, REL_WHEEL);

	set_event(info, UI_SET_MSCBIT, MSC_SERIAL);

	return 0;
}

#define USB_VENDOR_ID_WACOM 0x056a
#ifndef BUS_VIRTUAL
#define BUS_VIRTUAL 6
#endif
static int wacom_set_initial_values(struct uinput_info *info,
				     struct uinput_user_dev *dev)
{
	struct wacom_priv *priv = wacom_priv(info);
	struct wacom_features *features = &wacom_features[priv->type];

	snprintf(dev->name, sizeof(dev->name),
		 "%s%s%s", info->name,
		 (strlen(info->name) > 0) ? ": " : "",
		 wacom_features[priv->type].name);
	dev->id.bustype = BUS_VIRTUAL;
	dev->id.vendor = USB_VENDOR_ID_WACOM;
	dev->id.product = features->product_id;
	dev->id.version = 9999;	/* XXX */

	/* common */
        dev->absmax[ABS_X] = features->x_max;
        dev->absmax[ABS_Y] = features->y_max;
        dev->absmax[ABS_PRESSURE] = features->pressure_max;
        dev->absmax[ABS_DISTANCE] = features->distance_max;

	switch (features->type) {
	case MO:
		dev->absmax[ABS_WHEEL] = 71;
		/* fall through */
	case G4:
		/* fall through */
	case GRAPHIRE:
		break;

	case WACOM_21UX2:
	case BEE:
	case INTUOS3:
	case INTUOS3L:
	case CINTIQ: 
		dev->absmax[ABS_RY] = 4096;
		/* fall through */
	case INTUOS3S:
		dev->absmax[ABS_RX] = 4096;
		dev->absmax[ABS_Z] = 899;
		dev->absmin[ABS_Z] = -900;
		/* fall through */
	case INTUOS:
		dev->absmax[ABS_WHEEL] = 1023;
		dev->absmax[ABS_TILT_X] = 127;
		dev->absmax[ABS_TILT_Y] = 127;
		dev->absmin[ABS_RZ] = -900;
		dev->absmax[ABS_RZ] = 899;
        	dev->absmin[ABS_THROTTLE] = -1023;
	        dev->absmax[ABS_THROTTLE] = 1023;
		break;
	case PL:
	case PTU:
		break;
	case PENPARTNER:
		break;
	case TABLETPC:
		dev->absmax[ABS_RX] = 1023;
		dev->absmax[ABS_RY] = 1023;
	case INTUOS4S:
	case INTUOS4:
	case INTUOS4L:
		dev->absmin[ABS_Z] = -900;
		dev->absmax[ABS_Z] = 899;
	}

	return uinput_write_dev(info, dev);
}

void wacom_init(struct uinput_info *info)
{
	info->init_info = wacom_set_initial_values;
	info->enable_events = wacom_set_events;
	info->create_mode = WDAEMON_CREATE;
}

void wacom_print_device_type(int index)
{
	printf("%s", wacom_features[index].name);
}

#define DEV_DIR "/dev/input"
#ifndef UDEV_DIR
#define UDEV_DIR "/dev/input/wacom-tablets"
#endif
static void probe_event_file(const char *filename)
{
	struct input_id id;
	struct stat buff;
	static long long used;
	int fd, i, bit;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		log(LOG_ERR, "Error opening device '%s': %s\n",
		    filename, strerror(errno));
		return;
	}

	if (fstat(fd, &buff) < 0) {
		log(LOG_ERR, "Error retrieving file information: %s\n",
		    strerror(errno));
		close(fd);
		return;
	}

	if (ioctl(fd, EVIOCGID, &id)) {
		log(LOG_INFO, "Error retrieving device id: %s. not a input "
		    "device?\n", strerror(errno));
		close(fd);
		return;
	}
	close(fd);

	/* evdev begins on 64 */
	bit = (buff.st_rdev & 0xFF) - 64;
	if (used & (1 << bit))
		/* we already shown this event file */
		return;

	used |= (1 << bit);

	if (id.vendor != USB_VENDOR_ID_WACOM)
		/* not a Wacom product */
		return;

	if (id.bustype != BUS_USB)
		/* probably a uinput tablet */
		return;

	for (i = 0; i < wacom_get_n_devices(); i++)
		if (wacom_features[i].product_id == id.product) {
			printf("device = %i,%s\n", i, filename);
			return;
		}

	log(LOG_ERR, "Tablet not supported yet: %#x:%#x\n", id.vendor, id.product);
}

int wacom_autoconfigure(void)
{
	glob_t glob_buff;
	char pattern[50];
	int i;

	/* first trying the udev directory */
	snprintf(pattern, sizeof(pattern), "%s/%s", UDEV_DIR, "*");
	if (glob(pattern, GLOB_ERR, NULL, &glob_buff)) {
		log(LOG_ERR, "Unable to find the wacom udev directory "
		    "(%s) or no tablets found. Trying all event devices\n",
		    UDEV_DIR);
		/* trying all event devices */
		snprintf(pattern, sizeof(pattern), "%s/%s", DEV_DIR, "event*");
		if (glob(pattern, GLOB_ERR, NULL, &glob_buff)) {
			log(LOG_ERR, "Error matching the event devices: %s\n",
			    strerror(errno));
			return 1;
		}
	}

	for (i = 0; i < glob_buff.gl_pathc; i++)
		probe_event_file(glob_buff.gl_pathv[i]);

	globfree(&glob_buff);

	return 0;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
