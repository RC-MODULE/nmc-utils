/* 
 * libEasyNMC DSP communication library. 
 * Copyright (C) 2014  RC "Module"
 * Witten by Andrew 'Necromant' Andrianov <andrew@ncrmnt.org>
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
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <libelf.h>
#include <gelf.h>
#include <linux/easynmc.h>
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
	printf("Attaching %s io buffer size %d words @ 0x%x\n", name, 
	       le32_to_host(h->imem32[shdr.sh_addr + 1]), addr);
	
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


static int arg_handle_section(struct easynmc_handle *h, char* name, FILE *rfd, GElf_Shdr shdr)
{
	if (strcmp(name, ".easynmc_args")!=0)
		return 0;

	if (shdr.sh_size == 0) 
		return 0; /* If section optimized out - only name remains */

	h->argoffset = shdr.sh_addr;
	h->argdatalen   = shdr.sh_size - 2; 

	dbg("Arguments @0x%x size %d words\n", h->argoffset, h->argdatalen);
	return 1; /* Handled! */
}

static struct easynmc_section_filter arg_filter = {
	.name = "args",
	.handle_section = arg_handle_section
};



void easynmc_init_default_filters(struct easynmc_handle *h) 
{
	easynmc_register_section_filter(h, &stdio_filter);	
	easynmc_register_section_filter(h, &arg_filter);	
}
