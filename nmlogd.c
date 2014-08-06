/* 
 * libEasyNMC DSP communication library. 
 * Copyright (C) 2014  RC "Module"
 * Written by Andrew 'Necromant' Andrianov <andrew@ncrmnt.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * ut WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>


void usage(char *nm)
{
	fprintf(stderr, 
		"nmlogd - The EasyNMC logging daemon\n"
		"(c) 2014 RC Module | Andrew 'Necromant' Andrianov <andrew@ncrmnt.org>\n"
		"This is free software; see the source for copying conditions.  There is NO\n"
                "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
		"License: LGPLv2 \n"
		"Usage: %s [options]   - operate on core 0 (default)\n"
		"Valid options are: \n"
		"  --help             - Show this help\n"
		"  --syslog[=facility]- Log to syslog facility (default - neuromatrix)\n"
		"  --file=/tmp/log    - Log to a file.  "
		"  --nodaemon         - Do not daemonize\n"
		"  --core=n           - Log on core n. Can be specified multiple times.\n"
		"  --pidfile          - Write a pid file\n" 
		"  --log-interrupts   - Log incoming interrupts\n"
		"Debugging options: \n"
		"  --debug            - Print lots of debugging info (nmctl)\n"
		"  --debug-lib        - Print lots of debugging info (libeasynmc)\n"
		, nm
);
}

static struct option long_options[] =
{
	/* Generic stuff. */
	{"help",             no_argument,         0, 'h' },

	/* Options */
	{"core",             required_argument,   0, 'c' },
	{"force",            no_argument,        &g_force,    1 },
	{"nostdio",          no_argument,        &g_nostdio,  1 },
	{"nosigint",         no_argument,        &g_nosigint, 1 },
	{"detach",           no_argument,        &g_detach,   1 },

	/* Debugging hacks */
	{"debug-lib",        no_argument,        &g_libeasynmc_debug, 1 },
	{"debug",            no_argument,        &g_debug,            1 },
	{"dump-ldr-regs",    optional_argument,   0,                'D' },

	{0, 0, 0, 0}
};

int main(void) {

 openlog("slog", LOG_PID|LOG_CONS, LOG_USER);
 syslog(LOG_INFO, "A different kind of Hello world ... ");
 closelog();

 return 0;
}
