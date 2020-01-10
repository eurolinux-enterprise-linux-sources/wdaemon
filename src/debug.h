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

#ifndef DEBUG_H
#define DEBUG_H
#include <sys/time.h>
#include <syslog.h>

extern int debug_level;
extern int use_syslog;

static inline void set_debug_level(int i)
{
	debug_level = i;
}

void set_syslog(int i);

static inline int get_debug_level(void)
{
	return debug_level;
}

#define dprint(l, x...) do { \
				if (get_debug_level() >= l) {\
					if (use_syslog) { \
						syslog(LOG_INFO, x); \
					} else { \
						struct timeval tv;\
						gettimeofday(&tv, NULL);\
						printf("%li.%li ", tv.tv_sec, tv.tv_usec);\
						printf(x); \
						fflush(stdout); \
					} \
				} \
			} while(0)
#define D do { dprint(-1, "%s: %i\n", __FILE__, __LINE__); } while(0)
#endif	/* DEBUG_H */

/* vim: set noexpandtab tabstop=8 shiftwidth=8: */
