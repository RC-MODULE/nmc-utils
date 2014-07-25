#include <stdio.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/epoll.h>

int g_debug = 1;
int g_force = 0; 
int g_nostdio = 0;
static uint32_t entrypoint;

#define dbg(fmt, ...) if (g_debug) { \
	fprintf(stderr, "libeasynmc: " fmt, ##__VA_ARGS__); \
	}

#define err(fmt, ...) if (g_debug) { \
	fprintf(stderr, "libeasynmc: " fmt, ##__VA_ARGS__); \
	}


#include <easynmc.h>

void usage(char *nm)
{
	fprintf(stderr, 
		"nmrun - The EasyNMC app runner wrapper\n"
		"(c) 2014 RC Module | Andrew 'Necromant' Andrianov <andrew@ncrmnt.org>\n"
		"This is free software; see the source for copying conditions.  There is NO\n"
                "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
		"License: LGPLv2 \n"
		"Usage: %s [options] [actions]   - operate on core 0 (default)\n"
		"       %s --core=n    [options] - operate on selected core\n"
		"       %s --core=all  [options] - operate all cores\n"
		"Valid options are: \n"
		"  --core=id          - Select a core to operate on (--core=all selects all cores)\n"
		"  --list             - list available nmc cores in this system and their state\n"
		"  --help             - Show this help\n" 
		"  --force            - Disable internal seatbelts (DANGEROUS!)\n" 
		"  --nostdio          - Do not auto-attach stdio\n" 
		"  --debug            - print lots of debugging info (nmctl)\n"
		"  --debug-lib        - print lots of debugging info (libeasynmc)\n"
		"Valid actions are: \n"
		"  --boot             - Load initcode and boot a core (all cores)\n"
		"  --reset-stats      - Reset driver statistics for core (all cores)\n"
		"  --load=file.abs    - Load abs file to core internal memory\n"
		"  --start=file.abs   - Load abs file to core internal memory and start it\n"
		"  --irq=[nmi,lp,hp]  - Send an interrupt to NMC\n"
		"  --kill             - Abort nmc program execution\n"
		"  --mon              - Monitor IRQs from NMC\n"
		"  --dump-ldr-regs    - Dump init code memory registers\n\n"
		"ProTIP(tm): You can supply init code file to use via NMC_STARTUPCODE env var\n"
		"            When no env is set nmctl will use: " DEFAULT_STARTUPFILE "\n"
		,nm, nm, nm
);
}

static struct option long_options[] =
{
	/* Generic stuff. */
	{"list",             no_argument,         0, 'l' },
	{"help",             no_argument,         0, 'h' },

	/* Options */
	{"core",             required_argument,   0, 'c' },
	{"force",            no_argument,        &g_force,   1 },
	{"nostdio",          no_argument,        &g_nostdio, 1 },

	/* Actual actions */
	{"boot",             no_argument,         0, 'b' },
	{"reset-stats",      no_argument,         0, 'r' },
	{"load",             required_argument,   0, 'L' },
	{"start",            required_argument,   0, 's' },
	{"irq",              required_argument,   0, 'i' },
	{"mon",              no_argument,         0, 'm' },
	{"mon-epoll",        no_argument,         0, 'M' },


	/* Debugging hacks */
	{"debug-lib",        no_argument,        &g_libeasynmc_debug, 1 },
	{"debug",            no_argument,        &g_debug,            1 },
	{"dump-ldr-regs",    optional_argument,   0,                'D' },

	{0, 0, 0, 0}
};


int main()
{
	return 0;
}
