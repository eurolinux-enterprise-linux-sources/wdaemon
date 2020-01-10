#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "device_types.h"
#include "wacom.h"
#include "stddev.h"

#define WACOM_START 0
#define KEYBOARD_START 100
#define KEYBOARD_END (KEYBOARD_START + KEYBOARD_MAX - 1)
#define MOUSE_START 150
#define MOUSE_END (MOUSE_START + MOUSE_MAX - 1)

struct device_type device_types[] = {
	[KEYBOARD_START ... KEYBOARD_END] = { .add = keyboard_add_device },
	[MOUSE_START ... MOUSE_END] = { .add = mouse_add_device },
};
int device_type_max = sizeof(device_types) / sizeof(struct device_type);

void init_device_types(void)
{
	int i;

	for (i = 0; i < wacom_get_n_devices(); i++)
		device_types[i].add = add_wacom_device;
}

void print_device_types(void)
{
	int i;

	printf("Available device types:\n");
	for (i = WACOM_START; i < wacom_get_n_devices(); i++) {
		printf("%i ", i);
		wacom_print_device_type(i);
		printf("\n");
	}
	for (i = KEYBOARD_START; i <= KEYBOARD_END; i++) {
		printf("%i ", i);
		keyboard_print_device_type(i);
		printf("\n");
	}
	for (i = MOUSE_START; i <= MOUSE_END; i++) {
		printf("%i ", i);
		mouse_print_device_type(i);
		printf("\n");
	}
}

int add_device(struct session *session, char *path, int type)
{
	if (type > device_type_max)
		goto err;

	if (device_types[type].add != 0)
		return device_types[type].add(session, path, type);

err:
	errno = ENODEV;
	return 1;
}

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
