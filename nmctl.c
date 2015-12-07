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
#include <linux/easynmc.h>


int g_debug = 1;
int g_force = 0; 
int g_nostdio = 0;
static uint32_t entrypoint;

#define dbg(fmt, ...) if (g_debug) { \
	fprintf(stderr, "nmctl: " fmt, ##__VA_ARGS__); \
	}

#define err(fmt, ...) if (g_debug) { \
	fprintf(stderr, "nmctl: " fmt, ##__VA_ARGS__); \
	}


#include <easynmc.h>


int do_dump_core_info(struct easynmc_handle *h, void *udata) 
{

	char name[64];
	char type[64];
	int ret;
	
	ret = ioctl(h->iofd, IOCTL_NMC3_GET_NAME, name);
	if (ret != 0) {
		perror("ioctl");
		exit(1);
	}
	
	ret = ioctl(h->iofd, IOCTL_NMC3_GET_TYPE, type);
	if (ret != 0) {
		perror("ioctl");
		exit(1);
	}
	
	struct nmc_core_stats stats; 
	
	ret = ioctl(h->iofd, IOCTL_NMC3_GET_STATS, &stats);
	if (ret != 0) {
		perror("ioctl");
		exit(1);
	}

	/* Now, let's read some magic bytes */
	const char *status = easynmc_state_name(easynmc_core_state(h));
	const char *appid = easynmc_appid_get(h);

	printf("%d. name: %s type: %s (%s) appid: %s\n", 
	       h->id, name, type, status, appid
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
	
	return 0;
}


int do_boot_core(struct easynmc_handle *h, void *optarg)
{
	int ret; 

	printf("Booting core %d with %s ipl\n", h->id,  (optarg ? "debug" : "production"));
	
	ret = easynmc_boot_core(h, (optarg ? 1 : 0) );
	if (ret)  {
		fprintf(stderr, "Failed to boot core #%d\n", h->id);
		exit(1);
	}
	return 0;
}


int do_dump_ldr_info(struct easynmc_handle *h, void *optarg)
{

	printf("=== Init code registers dump ===\n");
	printf("  CODEVER      %x\n", h->imem32[NMC_REG_CODEVERSION]);
	printf("  ISR_ON_START %x\n", h->imem32[NMC_REG_ISR_ON_START]);
	printf("  STATUS       %x\n", h->imem32[NMC_REG_CORE_STATUS]);
	printf("  START        %x\n", h->imem32[NMC_REG_CORE_START]);
	printf("  ENTRY        %x\n", h->imem32[NMC_REG_PROG_ENTRY]);
	printf("  RETCODE      %x\n", h->imem32[NMC_REG_PROG_RETURN]);
	printf("  APPDATA      %x\n", h->imem32[NMC_REG_APPDATA_SIZE]);
	return 0;
}

int do_reset_stats(struct easynmc_handle *h, void *optarg)
{
	if (0!=easynmc_reset_stats(h))
		exit(1);
	return 0;
}

int do_load_abs(struct easynmc_handle *h, void *arg)
{
	int ret;
	char *optarg = arg;
	int flags = ABSLOAD_FLAG_DEFAULT; 
	
	if (easynmc_core_state(h) == EASYNMC_CORE_COLD)
		ret = easynmc_boot_core(h, 0);
	if (ret)
		exit(1);

	if (g_nostdio) 
		flags &= ~(ABSLOAD_FLAG_STDIO);

	if (g_force)
		flags |= ABSLOAD_FLAG_FORCE;
	
	/* No args processing in nmctl */
	
	flags &= ~(ABSLOAD_FLAG_ARGS);
	
	ret = easynmc_load_abs(h, optarg, &entrypoint, flags);
	if (ret == 0) 
		printf("ABS file %s loaded, ok\n", optarg);
	else {
		printf("Failed to load ABS file %s\n", optarg);
		exit(1);
	}

	return 0;	
}

int do_start_app(struct easynmc_handle *h, void *optarg)
{
	int ret;	
	ret = easynmc_start_app(h, entrypoint);	
	if (ret == 0)
		printf("NMC app now started!\n");
	else
		printf("Failed to start app!\n");
	
	easynmc_persist_set(h, EASYNMC_PERSIST_ENABLE);

	easynmc_close(h);

	return 0;
}

int do_irq(struct easynmc_handle *h, void *optarg)
{
	int ret = 1;
	int irq = -1;

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

int do_mon(struct easynmc_handle *h, void *optarg)
{
	printf("Monitoring events, CTRL+C to terminate\n");
	int evt;
	struct easynmc_token *tok = easynmc_token_new(h, EASYNMC_EVT_ALL);
	while (1) { 
		evt = easynmc_token_wait(tok, 50000);
		if (evt != EASYNMC_EVT_TIMEOUT)
			printf("Event: %s\n", easynmc_evt_name(evt));
		
	}
	return 0;	
}

int do_kill(struct easynmc_handle *h, void *optarg)
{
	int ret=0;

	if ((easynmc_core_state(h) == EASYNMC_CORE_RUNNING) && (!g_force)) {
		fprintf(stderr, "Application is in state running (not killable)\n");
		fprintf(stderr, "Killing it may cause userspace to misbehave\n");
		fprintf(stderr, "Use --force to kill it anyway\n");
		return 0;
	}

	ret = easynmc_stop_app(h);
	if (ret==0) { 
		printf("App on core %d terminated\n", h->id);
		goto done;
	}
	
	if ((easynmc_core_state(h) != EASYNMC_CORE_IDLE) && 
	    (easynmc_core_state(h) != EASYNMC_CORE_COLD)) { 
		printf("Failed to terminate app on core %d\n", h->id);
		printf("This will likely be only fixed by a reboot, sorry\n");
	}
done:
	return 0;	
	
}

#define NUMEVENTS 16
int do_mon_epoll(struct easynmc_handle *h, void *optarg)
{
	int ret = 1;
	printf("Monitoring events on core %d (epoll), CTRL+C to terminate\n", h->id);
	
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

static int for_each_core_optarg(int core, int (*cb)(struct easynmc_handle *h, void *optarg), char* optarg)
{
	int ret = 0 ; 
	/* In case we operate on all cores */
	if (core==-2) { 
		ret = easynmc_for_each_core(cb, 0, optarg);
		if (!ret) 
			fprintf(stderr, "Iterated over 0 cores, kernel driver problem?\n");
		return ret ? 0 : 1;
	}
	/* In case we operate on all cores */
	struct easynmc_handle *h = easynmc_open_noboot(core, 0);
	if (!h) { 
		fprintf(stderr, "easynmc_open_noboot() failed. Kernel driver problem or invalid core?\n");
		return 1;
	}
	cb(h, optarg);
	return 0;
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
			return for_each_core_optarg(core, do_mon, optarg);
		case 'm':
			return for_each_core_optarg(core, do_mon, optarg);
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
			for_each_core_optarg(-2, do_dump_core_info, NULL);
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
