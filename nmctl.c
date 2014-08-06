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
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/epoll.h>


//int access(const char *pathname, int mode);

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


int do_dump_core_info(int coreid, char* optarg) 
{
	struct easynmc_handle *h = easynmc_open_noboot(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	
	char name[64];
	char type[64];
	int ret;

	ret = ioctl(h->iofd, IOCTL_NMC3_GET_NAME, name);
	if (ret != 0) {
		perror("ioctl");
		return 1;
	}
	
	ret = ioctl(h->iofd, IOCTL_NMC3_GET_TYPE, type);
	if (ret != 0) {
		perror("ioctl");
		return 1;
	}
	
	struct nmc_core_stats stats; 
	
	ret = ioctl(h->iofd, IOCTL_NMC3_GET_STATS, &stats);
	if (ret != 0) {
		perror("ioctl");
		return 1;
	}

	/* Now, let's read some magic bytes */
	const char *status = easynmc_state_name(easynmc_core_state(h));
	
	printf("%d. name: %s type: %s (%s)\n", 
	       coreid, name, type, status
		);
	
	if (stats.started) { 
		uint32_t codever = h->imem32[NMC_REG_CODEVERSION];
		printf("   Initcode version: %x, %s\n", codever, 
		       easynmc_startupcode_is_compatible(codever) ? "compatible" : "incompatible");
	}
	
	printf("   IRQs Recv: HP:  %d LP: %d\n",
	       stats.irqs_recv[NMC_IRQ_HP], 
	       stats.irqs_recv[NMC_IRQ_LP]
		);
	
	printf("   IRQs Sent: NMI: %d HP: %d LP: %d\n",
	       stats.irqs_sent[NMC_IRQ_NMI], 
	       stats.irqs_sent[NMC_IRQ_HP], 
	       stats.irqs_sent[NMC_IRQ_LP]
		);
	
	easynmc_close(h);
	return 0;
}


int do_boot_core(int coreid, char* optarg)
{
	struct easynmc_handle *h = easynmc_open_noboot(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	int ret; 
	ret = easynmc_boot_core(h, (optarg ? 1 : 0) );
	if (ret) 
		fprintf(stderr, "Failed to boot core #%d\n", coreid);
	
	/* Opening and closing does the trick */
	easynmc_close(h); 
	return 0;
}


int do_dump_ldr_info(int coreid, char* optarg)
{
	struct easynmc_handle *h = easynmc_open_noboot(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	
	printf("  CODEVER      %x\n", h->imem32[NMC_REG_CODEVERSION]);
	printf("  ISR_ON_START %x\n", h->imem32[NMC_REG_ISR_ON_START]);
	printf("  STATUS       %x\n", h->imem32[NMC_REG_CORE_STATUS]);
	printf("  START        %x\n", h->imem32[NMC_REG_CORE_START]);
	printf("  ENTRY        %x\n", h->imem32[NMC_REG_PROG_ENTRY]);
	printf("  RETCODE      %x\n", h->imem32[NMC_REG_PROG_RETURN]);

	easynmc_close(h); 
	return 0;
}

int do_reset_stats(int coreid, char* optarg)
{
	int ret;
	struct easynmc_handle *h = easynmc_open_noboot(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}

	ret = easynmc_reset_stats(h);
	
	easynmc_close(h); 
	return ret;
}

int do_load_abs(int coreid, char* optarg)
{
	int ret;
	struct easynmc_handle *h = easynmc_open(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}

	int flags = ABSLOAD_FLAG_DEFAULT; 
	
	if (g_nostdio) 
		flags &= ~(ABSLOAD_FLAG_STDIO);

	if (g_force)
		flags |= ABSLOAD_FLAG_FORCE;
	
	/* No args processing in nmctl */
	
	flags &= ~(ABSLOAD_FLAG_ARGS);
	
	ret = easynmc_load_abs(h, optarg, &entrypoint, flags);
	if (ret == 0) 
		printf("ABS file %s loaded, ok\n", optarg);
	else
		printf("Failed to load ABS file %s\n", optarg);
	easynmc_close(h); 
	return ret;	
}

int do_start_app(int coreid, char* optarg)
{
	int ret;
	struct easynmc_handle *h = easynmc_open(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	
	ret = easynmc_start_app(h, entrypoint);	
	if (ret == 0)
		printf("NMC app now started!\n");
	else
		printf("Failed to start app!\n");		

	easynmc_close(h);
	return ret;
}

int do_irq(int coreid, char* optarg)
{
	int ret = 1;
	int irq = -1;
	struct easynmc_handle *h = easynmc_open(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	if (strcmp(optarg,"nmi")==0)
		irq = NMC_IRQ_NMI;
	else if (strcmp(optarg,"lp")==0)
		irq = NMC_IRQ_LP;
	else if (strcmp(optarg,"hp")==0)
		irq = NMC_IRQ_HP;
	
	if (irq!=-1)
		ret = easynmc_send_irq(h,irq);
	
	easynmc_close(h);
	return ret;
}

int do_mon(int coreid, char* optarg)
{
	int ret = 1;
	printf("Monitoring events on core %d, CTRL+C to terminate\n", coreid);
	struct easynmc_handle *h = easynmc_open(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	int evt;
	struct easynmc_token *tok = easynmc_token_new(h, EASYNMC_EVT_ALL);
	while (1) { 
		evt = easynmc_token_wait(tok, 50000);
		if (evt != EASYNMC_EVT_TIMEOUT)
			printf("Event: %s\n", easynmc_evt_name(evt));
		
	}
	easynmc_close(h);
	return ret;	
}

int do_kill(int coreid, char* optarg)
{
	int ret=0;
	struct easynmc_handle *h = easynmc_open(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	
	if (easynmc_core_state(h) != EASYNMC_CORE_RUNNING) {
		printf("Trying to kill app on core %d when app not running\n", coreid);
		goto done;
	}
	
	ret = easynmc_stop_app(h);
	if (ret==0) { 
		printf("App on core %d terminated\n", coreid);
		goto done;
	}
	
	printf("Failed to terminate app on core %d\n", coreid);

done:
	easynmc_close(h);
	return ret;	
	
}

#define NUMEVENTS 16
int do_mon_epoll(int coreid, char* optarg)
{
	int ret = 1;
	printf("Monitoring events on core %d (epoll), CTRL+C to terminate\n", coreid);
	struct easynmc_handle *h = easynmc_open(coreid);
	if (!h) { 
		fprintf(stderr, "easynmc_open() failed\n");
		return 1;
	}
	
	if (0!=easynmc_pollmark(h))
		goto errclose;

	struct epoll_event event;
	struct epoll_event *events;
	int efd = epoll_create(1);
	if (efd == -1)
	{
		perror ("epoll_create");
		ret = 1;
		goto errclose;
	}
	
	event.data.fd = h->memfd;
	event.events = EPOLLNMI | EPOLLHP | EPOLLLP;
	ret = epoll_ctl (efd, EPOLL_CTL_ADD, h->memfd, &event);
	if (ret == -1)
	{
		perror ("epoll_ctl");
		ret = 1;
		goto errclose;
	}
	
	 events = calloc (NUMEVENTS, sizeof event);
	 
	 while (1) { 
		 int n, i;
		 n = epoll_wait(efd, events, NUMEVENTS, -1);
		 for (i = 0; i < n; i++) {
			 if (events[i].events & EPOLLNMI)
				 printf("Event: NMI\n");
			 if (events[i].events & EPOLLLP)
				 printf("Event: LP\n");
			 if (events[i].events & EPOLLHP)
				 printf("Event: HP\n");
			 if (events[i].events & EPOLLERR)
				 printf("Event: ERROR\n");
		 }
	 }
errclose:
	easynmc_close(h);
	return ret;	
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
	{"boot",             optional_argument,   0, 'b' },
	{"reset-stats",      no_argument,         0, 'r' },
	{"load",             required_argument,   0, 'L' },
	{"start",            required_argument,   0, 's' },
	{"irq",              required_argument,   0, 'i' },
	{"mon",              no_argument,         0, 'm' },
	{"mon-epoll",        no_argument,         0, 'M' },
	{"kill",             no_argument,         0, 'k' },


	/* Debugging hacks */
	{"debug-lib",        no_argument,        &g_libeasynmc_debug, 1 },
	{"debug",            no_argument,        &g_debug,            1 },
	{"dump-ldr-regs",    optional_argument,   0,                'D' },

	{0, 0, 0, 0}
};

void usage(char *nm)
{
	fprintf(stderr, 
		"nmctl - The EasyNMC control utility\n"
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
		"            When no env is set nmctl will search a set of predefined paths\n"
		,nm, nm, nm
);
}

static int for_each_core(int (*action)(int, char *), char *optarg) {
	int i=0;
	int ret; 
	char tmp[64];
	int retcode = 0;
	do { 
		/* TODO: Better way to enumerate cores. Current sucks */ 
		sprintf(tmp, "/dev/nmc%dmem", i);
		ret = access(tmp, R_OK);
		if (ret==0)
			retcode += action(i,optarg);
		else
			break;
		i++;
	} while (1);
	return retcode;
}

static int for_each_core_optarg(int core, int (*action)(int, char*), char* optarg)
{
	if (core==-1)
		return for_each_core(action, optarg);
	else
		return action(core, optarg);
}

int main (int argc, char **argv) 
{
	int core = 0; /* Default - use first available core */
	
	if (argc < 2)
		usage(argv[0]),	exit(1);
	
	while (1)
	{		
		int c; 
		int option_index = 0;
		int ret; 
		c = getopt_long (argc, argv, "lhb:zr:",
				 long_options, &option_index);
		
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c)
		{
		case 'c':
			if (strcmp(optarg, "all") == 0)
				core = -1;
			else
				core = atoi(optarg);
			break;
		case 'M':
			return do_mon_epoll(core, NULL);
		case 'm':
			return do_mon(core, NULL);
		case 'r':
			return for_each_core_optarg(core, do_reset_stats, NULL);
		case 'k':
			return for_each_core_optarg(core, do_kill, NULL);
		case 'i':
			return for_each_core_optarg(core, do_irq, optarg);			
		case 'D':
			return for_each_core_optarg(core, do_dump_ldr_info, NULL);			
		case 'L':
		case 's':
			ret = for_each_core_optarg(core, do_load_abs, optarg);
			/* start */
			if (ret != 0) {
				fprintf(stderr, "Failed to load abs file to nmc core(s)\n");
				return ret;
			}
			if (c=='s')
				ret = for_each_core_optarg(core, do_start_app, optarg);
			return ret; 
			break;
		case 'b':
			return for_each_core_optarg(core, do_boot_core, optarg);
		case 'l':
			for_each_core(do_dump_core_info, NULL);
			exit(0);
			break;
		case 'h':
			usage(argv[0]);
			exit(1);
			break;
		}        
	}
	return 0;
}
