#include <stdio.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libelf.h>
#include <gelf.h>
#include <easynmc.h>


#define dbg(fmt, ...) if (g_libeasynmc_debug) { \
	fprintf(stderr, "libeasynmc: " fmt, ##__VA_ARGS__); \
	}

#define err(fmt, ...) if (g_libeasynmc_errors) { \
	fprintf(stderr, "libeasynmc: " fmt, ##__VA_ARGS__); \
	}

static int stdio_handle_section(struct easynmc_handle *h, char* name, FILE *rfd, GElf_Shdr shdr)
{
	int type = -1; 

	if (strcmp(name, ".easynmc_stdin")==0) 
		type = 0;
	else if (strcmp(name, ".easynmc_stdout")==0)
		type = 1;

	if (shdr.sh_size == 0) 
		return 0; /* If section optimized out - only name remains */

	if (-1 == type) 
		return 0;
	
	
	int rq = (type) ? IOCTL_NMC3_ATTACH_STDOUT : IOCTL_NMC3_ATTACH_STDIN;
	uint32_t addr = shdr.sh_addr << 2;
	dbg("Attaching %s io buffer size %d words\n", name, h->imem32[shdr.sh_addr + 1]);
	
	int ret = ioctl(h->iofd, rq, &addr);
	if (ret != 0) { 
		perror("ioctl");
		return 0;
	}

	rq = (type) ? IOCTL_NMC3_REFORMAT_STDOUT : IOCTL_NMC3_REFORMAT_STDIN;
	uint32_t rfmt = 1; /* reformat stdio by default */ 

	ret = ioctl(h->iofd, rq, &rfmt);
	if (ret != 0) { 
		perror("ioctl");
		return 0;
	}
	
	return 1; /* Handled! */
}

static struct easynmc_section_filter stdio_filter = {
	.name = "stdio",
	.handle_section = stdio_handle_section
};

void easynmc_init_default_filters(struct easynmc_handle *h) 
{
	easynmc_register_section_filter(h, &stdio_filter);	
}
