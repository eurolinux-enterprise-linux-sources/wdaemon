#if HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef DEVICE_TYPES_H
#define DEVICE_TYPE_H
struct session;
struct device_type {
	int (*add)(struct session *session, char *path, int type);
};
extern struct device_type device_types[];
extern int device_type_max;
void init_device_types(void);
void print_device_types(void);
int add_device(struct session *session, char *path, int type);
#endif

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
