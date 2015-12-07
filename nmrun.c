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
#include <sys/select.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <errno.h>
#include <sys/epoll.h>
#include <linux/easynmc.h>

int g_debug = 1;
int g_force = 0; 
int g_nostdio = 0;
int g_detach  = 0;
int g_nosigint = 0;

struct easynmc_handle *g_handle = NULL;

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
		"Usage: %s [options] myapp.abs [arguments]   - operate on core 0 (default)\n"
		"Valid options are: \n"
		"  --help             - Show this help\n" 
		"  --core=id          - Select a core to operate on (Default - use first usused core)\n"
		"  --force            - Disable internal seatbelts (DANGEROUS!)\n" 
		"  --nostdio          - Do not auto-attach stdio\n" 
		"  --nosigint         - Do not catch SIGINT\n"
		"  --detach           - Run app in background (do not attach console)\n"
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

void nonblock(int fd, int state)
{
	struct termios ttystate;

	//get the terminal state
	tcgetattr(fd, &ttystate);
	if (state==1)
	{
		//turn off canonical mode
		ttystate.c_lflag &= ~ICANON;
		ttystate.c_lflag &= ~ECHO;
		ttystate.c_lflag = 0;
		ttystate.c_cc[VTIME] = 0; /* inter-character timer unused */
		ttystate.c_cc[VMIN] = 0; /* We're non-blocking */
		
	}
	else if (state==0)
	{
		//turn on canonical mode
		ttystate.c_lflag |= ICANON | ECHO;
	}
	//set the terminal attributes.
	tcsetattr(fd, TCSANOW, &ttystate);

}

void die()
{
	fprintf(stderr, "\nCTRL+C pressed, terminating app\n");

	if (!g_nosigint)
		easynmc_stop_app(g_handle);

	if (isatty(STDIN_FILENO))
		nonblock(STDIN_FILENO,  0);
	exit(0);	
}


void  handle_sigint(int sig)
{
	signal(sig, SIG_IGN);
	die();
}

#define NUMEVENTS 3


int read_inbound(int fd) 
{
	unsigned char fromnmc[1024];
	do { 
		int n;
		n = read(fd, fromnmc, 1024); 
		if (n == -1) {
			if (errno==EAGAIN)
				break;
			else {
				perror("read-from-nmc");
				return errno;
			}
		}
		write(STDOUT_FILENO, fromnmc, n);
	} while (1);
	return 0;
}

/* DO NOT SAY ANYTHING. Please ;) */
int run_interactive_console(struct easynmc_handle *h)
{

	int i;
	int ret = 1;
	struct epoll_event event[3];
	struct epoll_event *events;
	int efd = epoll_create(2);
	setvbuf(stdin,NULL,_IONBF,0);


	easynmc_reformat_stdout(h, 0);
	easynmc_reformat_stdin(h, 0);

	if (efd == -1)
	{
		perror ("epoll_create");
		ret = 1;
		goto errclose;
	}
	
	int flags = fcntl(h->iofd, F_GETFL, 0);
	fcntl(h->iofd, F_SETFL, flags | O_NONBLOCK);

	if (!isatty(STDIN_FILENO)) {
		flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		fcntl(h->iofd, F_SETFL, flags | O_NONBLOCK);
	} else { 
		nonblock(STDIN_FILENO,   1);
	}

	event[0].data.fd = h->iofd;
	event[0].events  = EPOLLIN | EPOLLOUT | EPOLLET;

	event[1].data.fd = h->memfd;
	event[1].events  = EPOLLNMI | EPOLLHP | EPOLLET;
	
	event[2].data.fd = STDIN_FILENO;
	event[2].events  = EPOLLIN | EPOLLET;
	
	for (i = 0; i < 3; i++) { 
		ret = epoll_ctl (efd, EPOLL_CTL_ADD, event[i].data.fd, &event[i]);
		if (ret == -1)
		{
			perror ("epoll_ctl");
			ret = 1;
			goto errclose;
		}
	}

	events = calloc (NUMEVENTS, sizeof event);
	 
	int can_read_stdin    = 0;
	int can_write_to_nmc  = 0;

	int gotfromstdin = 0; 
	int written_to_nmc = 0;
	unsigned char   tonmc[1024];
	 
	while (1) { 
		int num, i;
		num = epoll_wait(efd, events, NUMEVENTS, -1);
		for (i = 0; i < num; i++) {
			if ((events[i].data.fd == STDIN_FILENO) && (events[i].events & EPOLLIN))
				can_read_stdin=1;
			 
			if (can_read_stdin && !gotfromstdin) 
			{ 
				gotfromstdin = read(STDIN_FILENO, tonmc, 1024);
				if (isatty(STDIN_FILENO) && tonmc[0] == 3)
					die();
				
				if (-1 == gotfromstdin) { 
					perror("read-from-stdin");
					return 1;
				}
			}

			if (events[i].data.fd == h->iofd && (events[i].events & EPOLLIN)) {
				ret = read_inbound(h->iofd);
				if (ret != 0)
					return ret;
			}
			 
			if (events[i].data.fd == h->iofd && (events[i].events & EPOLLOUT)) 
				can_write_to_nmc++;
			 			 
			if ((events[i].data.fd == h->memfd) &&
			    easynmc_core_state(h) == EASYNMC_CORE_IDLE) { 
				/* 
				 * Read any bytes left in circular buffer.
				 */
				ret = read_inbound(h->iofd);
				if ( ret != 0)
					return ret;
				
				ret = easynmc_exitcode(h);				
				fprintf(stderr, "App terminated with result %d, exiting\n", ret);

				return ret;
			}

			if (can_write_to_nmc && (written_to_nmc != gotfromstdin)) {
				int n = write(h->iofd, &tonmc[written_to_nmc], 
					      gotfromstdin - written_to_nmc);

				if (n > 0) {
					written_to_nmc += n;			 
					if (written_to_nmc == gotfromstdin) {
						gotfromstdin   = 0;
						written_to_nmc = 0;
					}
				}
				else if (errno == EAGAIN) {
					break;
				} else {
					perror("write-to-nmc");
					return 1;
				}
			}
		}
	}
	
errclose:
	easynmc_close(h);
	return ret;	
}




int main(int argc, char **argv)
{
	int core = 0; // EASYNMC_CORE_ANY; /* Default - use first available core */
	int ret; 
	char* self = "nmrun";

	if (argc < 2)
		usage(argv[0]),	exit(1);
	
	while (1)
	{		
		int c; 
		int option_index = 0;
		c = getopt_long (argc, argv, "c:h",
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
		case 'h':
			usage(argv[0]);
			exit(1);
			break;
		case '?':
		default:
			break;
		}        
	}

	char* absfile = argv[optind++];	
	int num_args = argc - optind;
	char **args = &argv[optind];

	uint32_t flags = ABSLOAD_FLAG_DEFAULT;
	
	if (g_nostdio)
		flags &= ~(ABSLOAD_FLAG_STDIO);

	struct easynmc_handle *h = easynmc_open(core); 
	g_handle = h;

	if (!h) {
		fprintf(stderr, "Failed to open core %d\n", core);
		exit(1);
	}
	
	int state; 
	if ((state = easynmc_core_state(h)) != EASYNMC_CORE_IDLE) { 
		fprintf(stderr, "Core is %s, expecting core to be idle\n", easynmc_state_name(state));
		exit(1);
	}
	
	ret = easynmc_load_abs(h, absfile, &entrypoint, flags);
	if (0!=ret) {
		fprintf(stderr, "Failed to upload abs file\n");
		exit(1);
	}

	ret = easynmc_set_args(h, self, num_args, args);
	if (ret != 0) { 
		fprintf(stderr, "WARN: Failed to set arguments. Not supported by app?\n");		
	}
	
	ret = easynmc_pollmark(h);
	
	if (ret != 0) { 
		fprintf(stderr, "Failed to reset polling counter (\n");
		exit(1);		
	};


	ret = easynmc_start_app(h, entrypoint);
	if (ret != 0) { 
		fprintf(stderr, "Failed to start app (\n");
		exit(1);
	}

	ret = 0;

	if (!g_nosigint)
		signal(SIGINT, handle_sigint);
	
	easynmc_persist_set(h, g_nosigint ? EASYNMC_PERSIST_ENABLE : EASYNMC_PERSIST_DISABLE);

	if (!g_detach) { 
		fprintf(stderr, "Application now started, hit CTRL+C to %s it\n", g_nosigint ? "detach" : "stop");
		ret = run_interactive_console(h);
	} else { 
		fprintf(stderr, "Application started, detaching\n");
	}
	
	
	if (isatty(STDIN_FILENO))
		nonblock(STDIN_FILENO,  0);

	return ret;
}
