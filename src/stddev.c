#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <errno.h>
#include <string.h>
#include "log.h"
#include "session.h"
#include "hotplug.h"
#include "monitored.h"
#include "uinput.h"
#include "device_types.h"
#include "stddev.h"

static int keyboard_num = 1;
static int mouse_num = 1;

struct keyboard_priv {
	int type;
};

#ifndef BUS_VIRTUAL
#define BUS_VIRTUAL 6
#endif
static int keyboard_init_info(struct uinput_info *info,
			      struct uinput_user_dev *dev)
{
	snprintf(dev->name, sizeof(dev->name), "%s", "wdaemon keyboard");
	dev->id.bustype = BUS_VIRTUAL;
	dev->id.vendor = 1;
	dev->id.product = keyboard_num++;

	return uinput_write_dev(info, dev);
}

static short enabled_keys[] = {
	KEY_ESC,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_0,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_Q,
	KEY_W,
	KEY_E,
	KEY_R,
	KEY_T,
	KEY_Y,
	KEY_U,
	KEY_I,
	KEY_O,
	KEY_P,
	KEY_LEFTBRACE,
	KEY_RIGHTBRACE,
	KEY_ENTER,
	KEY_LEFTCTRL,
	KEY_A,
	KEY_S,
	KEY_D,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_GRAVE,
	KEY_LEFTSHIFT,
	KEY_BACKSLASH,
	KEY_Z,
	KEY_X,
	KEY_C,
	KEY_V,
	KEY_B,
	KEY_N,
	KEY_M,
	KEY_COMMA,
	KEY_DOT,
	KEY_SLASH,
	KEY_RIGHTSHIFT,
	KEY_KPASTERISK,
	KEY_LEFTALT,
	KEY_SPACE,
	KEY_CAPSLOCK,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_NUMLOCK,
	KEY_SCROLLLOCK,
	KEY_KP7,
	KEY_KP8,
	KEY_KP9,
	KEY_KPMINUS,
	KEY_KP4,
	KEY_KP5,
	KEY_KP6,
	KEY_KPPLUS,
	KEY_KP1,
	KEY_KP2,
	KEY_KP3,
	KEY_KP0,
	KEY_KPDOT,
	KEY_102ND,
	KEY_F11,
	KEY_F12,
	KEY_RO,
	KEY_KPJPCOMMA,
	KEY_KPENTER,
	KEY_RIGHTCTRL,
	KEY_KPSLASH,
	KEY_SYSRQ,
	KEY_RIGHTALT,
	KEY_LINEFEED,
	KEY_HOME,
	KEY_UP,
	KEY_PAGEUP,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_END,
	KEY_DOWN,
	KEY_PAGEDOWN,
	KEY_INSERT,
	KEY_DELETE,
	KEY_MACRO,
	KEY_PAUSE,
	KEY_LEFTMETA,
	KEY_RIGHTMETA,
	KEY_COMPOSE,
	KEY_MAX,
};

#define set_event(x, y, z) do { if (ioctl((x)->fd, (y), (z)) == -1) { \
					log(LOG_ERR, "Error enabling %s (%s)\n", \
					    #z, strerror(errno)); \
					return 1; \
				} \
			} while(0)
static int keyboard_enable_events(struct uinput_info *info,
				  struct uinput_user_dev *dev)
{
	int i;

	set_event(info, UI_SET_EVBIT, EV_KEY);

	for (i = 0; enabled_keys[i] != KEY_MAX; i++)
		set_event(info, UI_SET_KEYBIT, enabled_keys[i]);

	return 0;
}

static void keyboard_init(struct uinput_info *info)
{
	info->init_info = keyboard_init_info;
	info->enable_events = keyboard_enable_events;
	info->create_mode = WDAEMON_CREATE;
}

void keyboard_print_device_type(int index)
{
	printf("Standard Keyboard");
}

int keyboard_add_device(struct session *session, char *path, int type)
{
	struct keyboard_priv *priv;
	int rc;

	priv = malloc(sizeof(struct keyboard_priv));
	if (priv == NULL)
		return ENOMEM;
	priv->type = type;

	rc = hotplug_add_file(session, path);
	if (rc)
		return rc;

	return monitored_device_add(session->devices, keyboard_init, priv, path);
}

struct mouse_priv {
	int type;
};

void mouse_print_device_type(int index)
{
	printf("Standard Mouse");
}

static int mouse_init_info(struct uinput_info *info,
			      struct uinput_user_dev *dev)
{
	snprintf(dev->name, sizeof(dev->name), "%s", "wdaemon mouse");
	dev->id.bustype = BUS_VIRTUAL;
	dev->id.vendor = 0x2;
	dev->id.product = mouse_num++;

	return uinput_write_dev(info, dev);
}

static int mouse_enable_events(struct uinput_info *info,
			       struct uinput_user_dev *dev)
{
	set_event(info, UI_SET_EVBIT, EV_KEY);
	set_event(info, UI_SET_EVBIT, EV_REL);
	set_event(info, UI_SET_EVBIT, EV_SYN);

	set_event(info, UI_SET_KEYBIT, BTN_MOUSE);
	set_event(info, UI_SET_KEYBIT, BTN_LEFT);
	set_event(info, UI_SET_KEYBIT, BTN_RIGHT);
	set_event(info, UI_SET_KEYBIT, BTN_MIDDLE);
	set_event(info, UI_SET_KEYBIT, BTN_SIDE);

	set_event(info, UI_SET_RELBIT, REL_X);
	set_event(info, UI_SET_RELBIT, REL_Y);
	set_event(info, UI_SET_RELBIT, REL_Z);
	set_event(info, UI_SET_RELBIT, REL_WHEEL);

	return 0;
}

static void mouse_init(struct uinput_info *info)
{
	info->init_info = mouse_init_info;
	info->enable_events = mouse_enable_events;
}

int mouse_add_device(struct session *session, char *path, int type)
{
	struct mouse_priv *priv;
	int rc;

	priv = malloc(sizeof(struct mouse_priv));
	if (priv == NULL)
		return ENOMEM;
	priv->type = type;

	rc = hotplug_add_file(session, path);
	if (rc)
		return rc;

	return monitored_device_add(session->devices, mouse_init, priv, path);
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
