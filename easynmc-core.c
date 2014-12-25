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

#define _GNU_SOURCE
#include <stdio.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <alloca.h>
#include <fcntl.h>
#include <string.h>
#include <libelf.h>
#include <gelf.h>
#include <easynmc.h>
#include <errno.h>


int g_libeasynmc_debug  = 0;
int g_libeasynmc_errors = 1;

#define dbg(fmt, ...) if (g_libeasynmc_debug) { \
	fprintf(stderr, "libeasynmc: " fmt, ##__VA_ARGS__); \
	}

#define err(fmt, ...) if (g_libeasynmc_errors) { \
	fprintf(stderr, "libeasynmc: " fmt, ##__VA_ARGS__); \
	}


static uint32_t supported_startupcodes[] = {
	0x20140715,
	0x20141219,
};

static void easynmc_warn_unsupported(uint32_t codever)
{
	if (codever < 0x20141128) {
		fprintf(stderr, "Warning: IPL version in use is outdated!\n");
		fprintf(stderr, "Warning: This IPL does not support easynmc_appdata_*()!\n");
		fprintf(stderr, "Warning: This IPL does not support EASYNMC_STATE_KILLABLE!\n");
	}
}

/** \defgroup lowlevel Core library API
 *   This section contains lower level API functions.
 *
 *	\addtogroup lowlevel
 *	@{
 *
 */

/**
 * Checks if the startup code version is compatible with this library version
 *
 * @param codever
 * @return 1 - compatible, 0 - not compatible
 */
int easynmc_startupcode_is_compatible(uint32_t codever)
{
	int i;
	for (i=0; i<ARRAY_SIZE(supported_startupcodes); i++) {
		if (supported_startupcodes[i] == codever)
			return 1;
	}
	return 0;
}


/**
 * Query current core state.
 *
 * @param h
 * @return
 */
enum easynmc_core_state easynmc_core_state(struct easynmc_handle *h)
{
	struct nmc_core_stats stats; 
	int ret;
	ret = ioctl(h->iofd, IOCTL_NMC3_GET_STATS, &stats);
	if (ret != 0) {
		perror("ioctl");
		return EASYNMC_CORE_INVALID;
	}

	if (!stats.started)
		return EASYNMC_CORE_COLD;

	uint32_t codever = h->imem32[NMC_REG_CODEVERSION];

	if (!easynmc_startupcode_is_compatible(codever))
		return EASYNMC_CORE_INVALID;
	
	/* Warn the user of any unsupported features */
	easynmc_warn_unsupported(codever);

	uint32_t status  = h->imem32[NMC_REG_CORE_STATUS];
	if (status > EASYNMC_CORE_INVALID)
		return EASYNMC_CORE_INVALID;
	return status;
}

static const char* statuses[] = {
	"cold",
	"idle",
	"running",
	"paused",
	"killable",
	"invalid",
};

/**
 * Transforms enum easynmc_core_state into a human-readable
 * state name
 *
 * @param state
 * @return
 */
const char* easynmc_state_name(enum easynmc_core_state state) 
{
	if (state > EASYNMC_CORE_INVALID)
		state = EASYNMC_CORE_INVALID;
	return statuses[state];
}

/**
 * Returns the nmc core name
 *
 * @param h
 * @param str
 * @return
 */
int easynmc_get_core_name(struct easynmc_handle *h, char* str)
{
	return ioctl(h->iofd, IOCTL_NMC3_GET_NAME, str);
}

/**
 * Returns the easynmc core type
 *
 * @param h
 * @param str
 * @return
 */
int easynmc_get_core_type(struct easynmc_handle *h, char* str)
{
	return ioctl(h->iofd, IOCTL_NMC3_GET_TYPE, str);
}

/**
 * Send an irq to the nmc core.
 *
 * @param h
 * @param irq
 * @return
 */
int easynmc_send_irq(struct easynmc_handle *h, enum nmc_irq irq)
{
	return ioctl(h->iofd, IOCTL_NMC3_SEND_IRQ, &irq);
}

/**
 * Reset internal IRQ statistics.
 *
 * WARNING: Never call this when there are any active waits on tokens
 * or when there's somebody polling nmc. Bad things WILL happen.
 *
 * @param h
 * @return
 */
int easynmc_reset_stats(struct easynmc_handle *h)
{
	return ioctl(h->iofd, IOCTL_NMC3_RESET_STATS, NULL);
}


/**
 * Reset a Neuromatrix Core
 *
 * @param h
 */
void easynmc_reset_core(struct easynmc_handle *h)
{
	ioctl(h->iofd, IOCTL_NMC3_RESET, NULL);
}


static int str2nmc(uint32_t *dst, char* src, int len)
{ 
	int ret = len;
	while(len--)
		*dst++ = *src++;
	return ret;
}

/* FixMe: Take $(PREFIX) into account */

char *iplpaths[] = {
	"/usr/share/easynmc-" LIBEASYNMC_VERSION "/ipl/ipl-%s%s.abs",
	"/usr/local/share/easynmc-" LIBEASYNMC_VERSION "/ipl/ipl-%s%s.abs",
	"./ipl-%s.abs"
};

/**
 * Returns the path to the default ipl file.
 * if debug is '1' a path to an IPL with board-specific debugging functionality
 * is returned.
 *
 * @param name
 * @param debug
 * @return
 */
char *easynmc_get_default_ipl(char* name, int debug)
{
	int i;
	char *tmp;

	for (i=0; i<ARRAY_SIZE(iplpaths); i++) {
		asprintf(&tmp, iplpaths[i], name, debug ? "-debug" : "");
		dbg("trying: %s\n", tmp)
		if (0 == access(tmp, R_OK))
			return tmp;
		free(tmp);
	}
	return NULL;

}


static int find_unused(struct easynmc_handle *h, void *udata)
{
	struct easynmc_handle **dest = udata;
	if ((easynmc_core_state(h) == EASYNMC_CORE_IDLE) ||
	    (easynmc_core_state(h) == EASYNMC_CORE_COLD)) {
		*dest = h;
		return EASYNMC_ITERATE_STOP | EASYNMC_ITERATE_NOCLOSE;
	}
	return 0;
}
static int find_killable(struct easynmc_handle *h, void *udata)
{
	struct easynmc_handle **dest = udata;
	if ((easynmc_core_state(h) == EASYNMC_CORE_KILLABLE)) {
		*dest = h;
		return EASYNMC_ITERATE_STOP | EASYNMC_ITERATE_NOCLOSE;
	}
	return 0;
}

/**
 * Open a Neuromatrix core skipping the usual initialization.
 * Opening a NeuroMatrix core with easynmc_open_noboot() will try to acquire an exclusive lock if 
 * 'exclusive' is 1. Killing the userspace application that has the lock automatically releases the core. 
 * 
 * easynmc_open_noboot() doesn't guarantee that the core returned will be in 'idle' state. 
 *
 * @param coreid Number of the core or EASYNMC_CORE_ANY to get first unused one
 * @param exclusive. Attempt to get exlusive access to the NMC core
 *
 * @return
 */
struct easynmc_handle *easynmc_open_noboot(int coreid, int exclusive)
{
	char path[1024];
	int ret;

	struct easynmc_handle *h = NULL;
	
	if (coreid == EASYNMC_CORE_ANY) { 
		easynmc_for_each_core(find_unused, exclusive, &h);
		if (!h)
			easynmc_for_each_core(find_killable, exclusive, &h);
		return h;
	}
	
	dbg("opening core %d\n", coreid);
	
	h = malloc(sizeof(struct easynmc_handle));
	if (!h)
		return NULL;
	h->persistent = 0;
	/* let's open core mem, io, and do the mmap */
	h->id = coreid;

	h->appid = NULL;

	h->sfilters = NULL;

	sprintf(path, "/dev/nmc%dio", coreid);
	h->iofd = open(path, O_RDWR);
	if (!h->iofd) {
		err("Couldn't open NMC IO device\n");
		goto errfreenmc;
	}

	sprintf(path, "/dev/nmc%d", coreid);
	h->memfd = open(path, O_RDWR);
	if (!h->memfd) {
		err("Couldn't open NMC MEM device\n");
		goto errcloseiofd;
	}
	
	if (exclusive && (flock(h->memfd, LOCK_EX | LOCK_NB) != 0)) /* Locking failed */
		goto errcloseiofd;

	ret = ioctl(h->iofd, IOCTL_NMC3_GET_IMEMSZ, &h->imem_size);
	if (ret!=0) {
		err("Couldn't get NMC internal memory size\n");
		goto errclosememfd;
	}

	h->imem = mmap(NULL, h->imem_size, PROT_READ | PROT_WRITE, MAP_SHARED, h->memfd, 0);
	if (h->imem == MAP_FAILED) {
		err("Couldn't MMAP internal memory\n");
		goto errclosememfd;
	}

	h->imem32 = (uint32_t *) h->imem;

	dbg("Opened core %d iofd %d memfd %d. mmap imem %u bytes @ 0x%lx\n",
	    coreid, h->iofd, h->memfd, h->imem_size, (unsigned long) h->imem);

	easynmc_init_default_filters(h);

	return h;

errclosememfd:
	close(h->memfd);
errcloseiofd:
	close(h->iofd);
errfreenmc:
	free(h);
	err("Device was: %s\n", path);
	return NULL;
}

/**
 * \brief Bring up an NMC core, optionally with a debug IPL.
 *
 * This function is called internally by easynmc_open(), so normally
 * you do not need to ever call it.
 *
 * Debug version of IPL comes with some board-specific debugging functionality.
 * On MB77.07 this involves blinking a LED while nmc is running IPL.
 *
 * @param h
 * @param debug set to '1' if you need a 'debug' IPL.
 *
 * @return
 */
int easynmc_boot_core(struct easynmc_handle *h, int debug)
{
	int ret;
	uint32_t ep;
	const char* startupfile = getenv("NMC_STARTUPCODE");
	char name[64];

	if (easynmc_core_state(h) != EASYNMC_CORE_COLD) {
		err("core already started, will not load ipl again\n");
		return 1;
	}

	ret = easynmc_get_core_name(h, name);
	if (ret)
		return ret;

	if (!startupfile)
		startupfile = easynmc_get_default_ipl(name, debug);

	if (!startupfile) {
		err("Didn't find startup code file. Did you install one?\n");
		err("HINT: You can set env variable NMC_STARTUPCODE\n");
		return -1;
	}

	dbg("Booting core using: %s file\n", startupfile);

	ret = easynmc_load_abs(h, startupfile, &ep, 0);
	if (ret!=0)
		return ret;

	if (ep != 0) {
		err("ERROR: Startup code entry point must be 0x0!\n");
		err("ERROR: We have 0x%x! Cowardly refusing to go further\n", ep);
		return 1;
	}

	easynmc_reset_core(h);

	/* We want an HPINT when startup code is running */
	h->imem32[NMC_REG_ISR_ON_START] = 1;

	struct easynmc_token *tok = easynmc_token_new(h, EASYNMC_EVT_HP | EASYNMC_EVT_LP);
	if (!tok)
		return 1;

	ret = easynmc_send_irq(h, NMC_IRQ_NMI);
	if (ret != 0)
		return ret;
	int evt;

	evt = easynmc_token_wait(tok, 5000);
	dbg("Got evt %d\n", evt);
	switch(evt)
	{
	case EASYNMC_EVT_TIMEOUT:
		err("Timeout waiting for initcode to start\n");
		if (easynmc_core_state(h) == EASYNMC_CORE_IDLE) {
			err("But init code reports being ready. \n");
			err("Did you mix up HP and LP interrupts in DeviceTree?\n");
			return 0; /* Okay, nevertheless */
		}
		return 1;
		break;
	case EASYNMC_EVT_HP:
		dbg("Initial code loaded & ready \n");
		return 0;
		break;
	default:
		err("Got event %d (%s) instead of EVT_HP\n", evt, easynmc_evt_name(evt));
		err("Did you mix up HP and LP interrupts in DeviceTree?\n");
		return 1;
		break;
	}

	/* TODO: wait for loader, check version */
	return 0;
}

/**
 * @}
 */


/** \defgroup highlevel Loading, starting and stopping DSP Applications
 * This set of functions controls the execution of DSP programs.
 * Neuromatrix core start in 'cold' (stopped) state. They first need a proper IPL code that occupies 
 * the first few KiB if DSP SRAM. 
 * This IPL code sits there, handles NMI interrupt and performs jumps to user code.
 * IPL sources are located under the ipl/ directory of nmc-utils repository.
 * The simplified state diagram of DSP states is shown below.
 * 
 * \dotfile "nmc-states-simplified.gv"
 * 
 * Normally, you start working by opening a DSP core using easynmc_open(). The open function takes
 * care off all the work to load the ipl code if the core is in 'cold' state, check ipl version
 * compatibility and the rest. 
 * 
 * easynmc_open() guarantees that the core it returns is in IDLE state. If EASYNMC_CORE_ANY is passed as 
 * the core number, easynmc() will search all cores in idle or cold state.   
 *
 * Once you have the core opened, you have a handle, which is pointer to a C struct.
 * Members of this struct are file descriptors for Neuromatrix's stdio (iofd) and memory (memfd).
 * Neuromatrix DSP memory is also mmaped to the application using and can be accessed as a byte array
 * OR a uint32_t array via imem and imem32 pointers respectively.
 *
 * Since all of the memory is exposed to userspace, care must be taken not to overwrite any part of the ipl code
 * (which resides in first ~2KiB of internal ram).
 *
 * After opening the device you can add any section filters using easynmc_register_section_filter().
 *
 * N.B.: Each section filter can only be associated with one nmc handle.
 *
 * After all the section filters have been registered you can load an abs file using easynmc_load_abs().
 *
 * If the loading succeeds - you will get the valid entry point for your application.
 *
 * At this point you can set up one or more arguments for you application with easynmc_set_args(), these will
 * arrive in argc and argv on the nmc side.
 *
 * Next, it's time to start your application using the obtained entry point passed to easynmc_start_app();
 *
 * At this point you can interact with your application via stdio, listen for events, (See. \ref token_api and/or \ref poll_api) or
 * just wait for the app to stop and fetch the exit code using easynmc_exitcode()
 *
 * Closing the handle with easynmc_close() automatically terminates the application, unless application persistence
 * has been explicitly enabled. (See \ref app_persist)
 *	\addtogroup highlevel
 *	@{
 *
 */

/**
 * Setup arguments for an NMC program. This function takes care of reformatting
 * strings and putting them in the relevant memory places.
 *
 * Note: The compiled binary must have enough space allocated for the arguments during
 * compilation with EASYNMC_ARGS macro (Normally done in easyconf.asm).
 *
 * @param h easynmc handle
 * @param self argv[0]
 * @param argc argc
 * @param argv Array of arguments. Starting at what would be argv[1] on nmc.
 *
 * @return 0 if everything is OK; -1 - current handle has no argument offset.
 * 		   -2 - arguments size exceed the space available in the relevant section
 * 		   of the DSP program.
 */
int easynmc_set_args(struct easynmc_handle *h, char* self, int argc, char **argv)
{
	int i;
	int len;
	int needspace = argc + strlen(self) + 1; 

	if (!h->argoffset) { 
		err("No argument offset found for this handle\n");
		return -1;
	}

	for (i=0; i<argc; i++)
		needspace += strlen(argv[i]) + 1;
	
	if (needspace > h->argdatalen) {
		err("Arguments exceed available space.\n");
		return -2;
	}
	
	/* Let's make some black magic */

	h->imem32[h->argoffset]   = argc + 1 ;
	h->imem32[h->argoffset+1] = h->argoffset + 2;

	uint32_t *ptroff  = &h->imem32[h->argoffset + 2]; 
	uint32_t *dataoff = &h->imem32[h->argoffset + 2 + argc + 1];
	
	*ptroff = dataoff - h->imem32;
	len = str2nmc(dataoff, self, strlen(self)+1); 
	dataoff += len;
	
	for (i=0; i<argc; i++) {
		ptroff[i+1] = dataoff - h->imem32;
		len = str2nmc(dataoff, argv[i], strlen(argv[i])+1); 
		dataoff += len;
	}
	
	return 0;
}


/**
 * Load an abs file into DSP memory and get a reference to the entry point.
 * The entry point can only e considered valid if loading succeeds. You can later
 * use the obtained entry point to start program execution.
 *
 * Make sure you have set up any abs filters you need BEFORE calling this function.
 *
 * For a reasonable set of default flags use ABSLOAD_FLAG_DEFAULT. This should be
 * good for 99% of cases.
 *
 * Normally, you can only load code when the core is not running (cold or idle).
 * However, in some weird cases you may want to override this check. This can be
 * done via ABSLOAD_FLAG_FORCE. Just don't shoot yourself in the knee.
 *
 * @param h device handle
 * @param path file path
 * @param ep a pointer to uint32_t, will be used to store entry point if loading succeeds.
 * @param flags one or more ABSLOAD_FLAG_*
 * @return
 */
int easynmc_load_abs(struct easynmc_handle *h, const char *path, uint32_t* ep, int flags) 
{
	int i;
	char *id;
	GElf_Ehdr ehdr; 
	GElf_Shdr shdr;
	Elf *elf;
	int fd; 
	Elf_Scn *scn = NULL;

	const char *state = easynmc_state_name(easynmc_core_state(h));

	if (!(flags & ABSLOAD_FLAG_FORCE))
		if ((easynmc_core_state(h) == EASYNMC_CORE_RUNNING)  ||
		    (easynmc_core_state(h) == EASYNMC_CORE_KILLABLE) ||
		    (easynmc_core_state(h) == EASYNMC_CORE_INVALID))
		{
			err("ERROR: Attempt to load abs when core is '%s'\n", state);
			err("ERROR: Will not do that unless --force'd\n");
			return 1;
		}
	
	FILE* rfd = fopen(path, "rb");
	if (NULL==rfd) { 
		perror("fopen");
		return -1;
	}
	if ((fd = open(path, O_RDONLY)) == -1) {
		perror("open");
		goto errfclose;
	}
	

	if(elf_version(EV_CURRENT) == EV_NONE)
	{
		err("WARNING: Elf Library is out of date!\n");
	}

	if ((elf = elf_begin(fd, ELF_C_READ , NULL)) == NULL) {
		err("elf_begin() failed: %s.",
		    elf_errmsg(-1));
		goto errclose;
	}
	
	if (gelf_getehdr(elf, &ehdr) == NULL)  {
		err( "getehdr() failed: %s.",
		     elf_errmsg(-1));
		goto errclose;
	}
	
	if ((i = gelf_getclass(elf)) == ELFCLASSNONE) { 
		err( "getclass() failed: %s.",
		     elf_errmsg(-1));
		goto errclose;
	}
	
	dbg("ELF Looks like a %d-bit ELF object\n",
	       i == ELFCLASS32 ? 32 : 64);

	if ((id = elf_getident(elf, NULL)) == NULL) {
		err( "getident() failed: %s.",
		       elf_errmsg(-1));
		goto errclose;
	}
	
	dbg("ELF Machine id: 0x%x\n", ehdr.e_machine);
	
	*ep = ehdr.e_entry;
	
	elf = elf_begin(fd, ELF_C_READ , NULL);
		
	h->argoffset = 0;

	while((scn = elf_nextscn(elf, scn)) != 0)
	{
		char* why_skip = NULL;
		gelf_getshdr(scn, &shdr);
		char* name = elf_strptr(elf, ehdr.e_shstrndx, shdr.sh_name);
		int addr = shdr.sh_addr << 2;

		if (shdr.sh_size == 0)  /* Skip empty sections */
			why_skip = "(empty section)";

		if (0==strcmp(name,".memBankMap")) {
			why_skip = "(not needed)";
		}

		if (0==strcmp(name,".shstrtab")) {
			why_skip = "(not needed)";
		}
		
		if (shdr.sh_type == SHT_NOBITS) { 
			why_skip = "(nobits)";
			memset(&h->imem[addr], 0x0, shdr.sh_size);
		}

		if (0==strcmp(name,".bss")) {
			why_skip = "(cleansing)";
			memset(&h->imem[addr], 0x0, shdr.sh_size);
		}
		
		
		dbg("%s section %s %s %ld bytes @ 0x%x\n", 
		    why_skip ? "Skipping" : "Uploading", 
		    name, 
		    why_skip ? why_skip : "",
		    (unsigned long) shdr.sh_size,
		    addr	);
		

		size_t ret; 

		if (!why_skip) { 
			ret = fseek(rfd, shdr.sh_offset, SEEK_SET);
			if (ret !=0 ) { 
				err("Seek failed, bad elf\n");
				goto errclose;
			}	
			dbg("read to %d size %ld\n", addr, (unsigned long) shdr.sh_size);
			ret = fread(&h->imem[addr], 1, shdr.sh_size, rfd);
			if (ret != shdr.sh_size ) { 
				err("Ooops, failed to read all data: want %ld got %zd\n", 
				    (unsigned long) shdr.sh_size, ret);
				perror("fread");
				goto errclose;
			}
		}

		/* Now call the section filter chain */
		struct easynmc_section_filter *f = h->sfilters;		
		int handled = 0;

		while (!handled && f) {
			dbg("Aplying section filter %s\n", f->name);
			handled = f->handle_section(h, name, rfd, shdr);
			f = f->next;
		}
		
	}
	close(fd);
	fclose(rfd);
	dbg("Elvish loading done!\n");
	return 0;

errclose:
	close(fd);

errfclose:
	fclose(rfd);
	return -1;
}


/**
 * Start application execution at the specified entry point.
 * Use the entry point obtained from easynmc_load_abs()
 *
 * @param h
 * @param entry
 * @return
 */
int easynmc_start_app(struct easynmc_handle *h, uint32_t entry)
{
	enum easynmc_core_state s = easynmc_core_state(h);
	if (s != EASYNMC_CORE_IDLE) { 
		err("Core is in state %s, must be idle\n", easynmc_state_name(s));
		return 1;
	}
	
	h->imem32[NMC_REG_PROG_ENTRY] = entry;
	h->imem32[NMC_REG_CORE_START] = 1; 
	return 0; 
}


/**
 * Fetch the exit code of the last executed app.
 *
 * @param h
 * @return exit code from nmc application
 */
int easynmc_exitcode(struct easynmc_handle *h)
{
	return h->imem32[NMC_REG_PROG_RETURN];
}

/**
 * Terminate a running application and return to IPL.
 * This function may block for a little while.
 *
 * Notes about termination: If the application overrides the NMI handler - this call will never succeed. This is normal.
 * The application should never touch the NMI handler.
 * IPL takes care of cleaning up any leftovers in vector FIFOs.
 * Right now the only to fix if this function doesn't succeed - reboot the board.
 *
 * @param h
 * @return
 */
int easynmc_stop_app(struct easynmc_handle *h)
{
	int ret; 
	int state = easynmc_core_state(h);
	
	if ((state != EASYNMC_CORE_RUNNING) && 
	    (state != EASYNMC_CORE_KILLABLE)) {
		err("App not running, won't stop it\n");
		return 1;
	}

	ret = easynmc_send_irq(h, NMC_IRQ_NMI);
	if (ret!=0) {
		perror("send-irq");
		return 1;
	}

	int timeout = 100;
	while ((easynmc_core_state(h) != EASYNMC_CORE_IDLE) && --timeout) { 
		usleep(1000);
	}

	dbg("timeout remaining %d\n", timeout);

	return timeout ? 0 : 1;
}

/**
 * \brief Register a section filter.
 *
 * Section filters are a convenient way to attach any custom code interfacing with DSP.
 * If you need to implement your own way of talking to the DSP - this is how you can do it.
 *
 * @param h
 * @param f
 */
void easynmc_register_section_filter(struct easynmc_handle *h, struct easynmc_section_filter *f)
{
	if (!h->sfilters) {
		h->sfilters = f;
		f->next = NULL;
		return;
	}

	struct easynmc_section_filter *tail = h->sfilters; 		
	while (tail->next != NULL) 
		tail = tail->next;
	tail->next = f;
	f->next = NULL;
}



/**
 * Open a Neuromatix core and return a handle.
 * This function returns a handle, that should be used for all operations with this core.
 * The handle itself is just a pointer to a structure that contains a few file descriptors and mmaped DSP memory.
 * You can pass EASYNMC_CORE_ANY as coreid parameter to open the first available core. 
 * On multicore DSP systems easynmc_open() with EASYNMC_CORE_ANY searches for cores in the following order: 
 * - Cores that are in states EASYNMC_CORE_IDLE or EASYNMC_CORE_COLD are searched first
 * - Cores that are in states EASYNMC_CORE_KILLABLE 
 *
 * Opening a NeuroMatrix core with easynmc_open() always acquires an exclusive lock
 * for that core, so that no other app can use it. Killing the userspace application that has
 * the lock automatically releases the core. 
 *
 * @param coreid
 * @return
 */
struct easynmc_handle *easynmc_open(int coreid)
{
	int ret =0;
	struct easynmc_handle *h = NULL;



	h = easynmc_open_noboot(coreid, 1);

	if (!h)
		return NULL;

	enum easynmc_core_state s = easynmc_core_state(h);
	
	if (s == EASYNMC_CORE_COLD)
		ret = easynmc_boot_core(h, 0);
	
	if (s == EASYNMC_CORE_KILLABLE)
		ret = easynmc_stop_app(h);

	easynmc_persist_set(h, EASYNMC_PERSIST_DISABLE);

	if (ret != 0) 
		goto errfreeh;

	if (ret != 0) 
		goto errfreeh;
	
	return h;

errfreeh:
	easynmc_close(h);
	return NULL;	
}


/**
 *   Helper function. Interate over all available cores and do the following: 
 * - Open the core (if exclusive is '1' - attempt to get exclusive core access
 * - Run the specified callback with nmc core handle as the argument
 * - Parse callback return code. 
 * The return code can be 0 or one of the following bit flags: 
 * - EASYNMC_ITERATE_STOP - break the loop 
 * - EASYNMC_INTERATE_NOCLOSE - Do not call easynmc_close after the callback has returned
 *
 * Note: This function doesn't attempt to boot cores or alter the core state 
 * in any smart way, like easynmc_open() does. If you just need a free core - see 
 * easynmc_open() 
 *
 * @param core_cb The callback to run 
 * @param exclusive Lock core for exclusive access
 * @param udata Userdata pointer to pass to the callback
 */

int easynmc_for_each_core(int (*core_cb)(struct easynmc_handle *h, void *udata), int exclusive, void *udata)
{
	FILE *fd = fopen("/proc/nmc", "r"); 
	if (!fd) { 
		err("/proc/nmc open failed\n");
		return -EIO;
	}
	char tmp[512];
	int ret = 0; 
	while (fgets(tmp, 512, fd)) { 
		int coreid; 
		struct easynmc_handle *h; 
		if (1 == sscanf(tmp, "/dev/nmc%d", &coreid)) { 
			h = easynmc_open_noboot(coreid, exclusive);
			if (h) { 
				ret++; 
				int result = core_cb(h, udata);
				if (result & EASYNMC_ITERATE_STOP)
					break; 
				if (!(result & EASYNMC_ITERATE_NOCLOSE))
					easynmc_close(h); 
			}
		}
	}
	return ret;
}

/**
 * Close and free easynmc handle. Unless app persistance has been enabled
 * for this core, any running application will be killed by the kernel
 * driver.
 * This function releases the exclusive lock (if any) held by the process
 * on this core. 
 *
 * @param hndl
 */
void easynmc_close(struct easynmc_handle *hndl)
{
	/* Mark the application as killable. 
	   N.B. There's no race condition here, since if the app exit()s after
	        the check, but before updating the NMC_REG_CORE_STATUS, init code
		will overwrite it.
		If persistance is not enabled - app will be killed by kernel on
		close. 
	 */

	dbg("close core id %d \n", hndl->id);
	if ((hndl->persistent) && (easynmc_core_state(hndl) == EASYNMC_CORE_RUNNING))
		hndl->imem32[NMC_REG_CORE_STATUS] = EASYNMC_CORE_KILLABLE;

	/* Unlock */
	flock(hndl->memfd, LOCK_UN);
	/* Cleanup */
	close(hndl->iofd);
	close(hndl->memfd);
	munmap(hndl->imem, hndl->imem_size);
	free(hndl);
}


/**
 * @}
 * \defgroup token_api Handling events: token API
 * Tokens are a simple way to process events from Neuromatrix Core. You start by allocating a token with easynmc_token_new()
 * and specifying a bitfield of events you are interested in. Starting from the point of allocation all new events will be
 * stored onto the token.
 * You can clear any pending events from the token at any time using easynmc_token_clear().
 * Finally, you can use easynmc_token_wait() to get the next event from the token. This function will block until the next
 * event arrives.
 *
 * You can have any number of tokens simultaneously listening for events, each event will arrive into every listening token.
 * As an alternative, you can use polling. See \ref polling_api
 *
 * \addtogroup token_api
 * @{
 */

/**
 * Clear any pending events from a token
 *
 * @param t
 * @return
 */
int easynmc_token_clear(struct easynmc_token *t)
{
	int ret;
	ret = ioctl(t->h->iofd, IOCTL_NMC3_RESET_TOKEN, &t->tok);
	if (ret != 0)
		perror("ioctl");
	dbg("New token id %d\n", t->tok.id);
	return ret;
}

/**
 * Create a new token. Tokens are a convenient way to wait for different events from
 * the Neuromatrix core. There can be any number of tokens listening for events simultaneously
 * Each event will be dispatched to every listening token.
 *
 * @param h
 * @param events
 * @return
 */
struct easynmc_token *easynmc_token_new(struct easynmc_handle *h, uint32_t events)
{
	struct easynmc_token *t = calloc(1, sizeof(struct easynmc_token));
	if (!t)
		return NULL;
	t->h = h;

	t->tok.events_enabled = events;

	if ( 0 != easynmc_token_clear(t) ) {
		free(t);
		return NULL;
	}

	return t;
}

/**
 * Get a human-readable name of the event arrived.
 *
 * @param evt
 * @return
 */
const char* easynmc_evt_name(int evt) 
{
	switch(evt) {
	case EASYNMC_EVT_LP:
		return "LP";
	case EASYNMC_EVT_HP:
		return "HP";
	case EASYNMC_EVT_NMI:
		return "NMI";
	case EASYNMC_EVT_ERROR:
		return "ERROR";
	case EASYNMC_EVT_CANCELLED:
		return "CANCELLED";
	case EASYNMC_EVT_TIMEOUT:
		return "TIMEOUT";
	default:
		return "WTF!?";
	}
}

/**
 * Block until a new event arrives onto the token or until timeout expires.
 *
 * @param t
 * @param timeout timeout in ms
 * @return next event number
 */
int easynmc_token_wait(struct easynmc_token *t, uint32_t timeout) {
	int ret; 
	t->tok.timeout = timeout; 
	ret = ioctl(t->h->iofd, IOCTL_NMC3_WAIT_ON_TOKEN, &t->tok);
	if (ret != 0) {
		err("ioctl returned %d\n", ret);
		perror("ioctl");
		return EASYNMC_EVT_ERROR;
	}
	return t->tok.event;
}

/**
 * @}
 */

/**
 * \defgroup poll_api Handling events: select/epoll
 * As an alternative to tokens you can use system epoll or select calls to handle events from DSP.
 * To do that, you have to call select/epoll with memfd field of easynmc handle.
 * The following convention is used:
 *
 *  -  POLLIN - An LP interrupt has been received
 *  -  POLLPRI - An HP interrupt has been received
 *  -  POLLHUP - Someone sent an NMI interrupt to the NMC core.
 *
 * easynmc.h declares convenience POLLLP, POLLHP and POLLNMI constants as well as
 * EPOLLLP, EPOLLHP and EPOLLNMI
 *
 * The driver will dispatch only one event at a time.
 * You can use easynmc_pollmark() to clean any pending events. You are advised to do that once before
 * starting to process any events.
 *
 * \addtogroup poll_api
 * @{
 */

/**
 * Reset any pending events for poll/epoll interface
 * @param h
 * @return
 */
int easynmc_pollmark(struct easynmc_handle *h)
{
	return ioctl(h->iofd, IOCTL_NMC3_POLLMARK, NULL);
}


/**
 * @}
 * \defgroup app_persist DSP Application as a service
 * Sometimes you need to keep the DSP core running in background to provide some kind of 'service' 
 * to userspace application(s) or run completely independently. 
 * Mostly it is needed if userspace applications using this 'service' start and stop often and 
 * you don't want to waste time restarting DSP application again and again for some reason.
 * 
 * Please note, that thread creation is a slow process by itself and can be the bottleneck by itself, 
 * therefore do not expect the application persistence mechanism to magically boost performance. 
 *
 * If you are using DSP app persistence API the state diagram you've seen in \ref highlevel is a little 
 * bit more complex. 
 *
 * \dotfile "nmc-states.gv"
 *
 * A DSP application that doesn't have an associated userspace process is in KILLABLE state. 
 * If someone needs a free DSP core and easynmc_open() doesn't find any cores in COLD or IDLE states
 * it will search for cores in KILLABLE state, kill the application and return the handle in IDLE state. 
 * 
 * You can make your app transition into KILLABLE state by enabling DSP app persistence via 
 * easynmc_persist_set(). 
 * When enabled, easynmc_close() doesn't kill the application, but leaves it in KILLABLE state. 
 *
 * Running DSP apps are identified via a string of up to 8 characters long including the 
 * terminating NULL character. After the application is started its appid can be set and 
 * retrieved using easynmc_appid_set() and easynmc_appid_get() calls respectively. 
 * 
 * If you want to 'connect' to a running background DSP app you need to know its appid and supply it to
 * easynmc_connect() which attempts to locate the core with the app and returns the handle to it.
 * 
 * Sometimes you need to store useful pieces of data that describe the current state of the aplication  
 * e.g. adresses obtained via absfilters during app loading. Starting with libeasynmc version 0.1.1 you can
 * can call easynmc_appdata_set() and easynmc_appdata_get() to get and set your application-specific data 
 * respectively. 
 * 
 * The appdata buffer is copied by the library to the kernel driver. DSP app termination invalidates the data 
 * stored and the DSP application ID.
 * 
 * 
 * \addtogroup app_persist
 * @{
 */

#ifndef __DOXYGEN__
struct easynmc_connect_info {
	struct easynmc_handle *desth;
	const char* appid; 
};
#endif

static int connect_core_cb(struct easynmc_handle *h, void *udata)
{
	struct easynmc_connect_info *i = udata; 
	if ((easynmc_core_state(h) == EASYNMC_CORE_KILLABLE) && 
	    (strcmp(easynmc_appid_get(h), i->appid) == 0)) { 
		
		if (flock(h->memfd, LOCK_EX | LOCK_NB) != 0) 
			return 0;

		i->desth = h;
		return EASYNMC_ITERATE_STOP | EASYNMC_ITERATE_NOCLOSE;
	}
	return 0; 
}
	
/**
 * Connect to a running DSP application via a supplied appid.
 * 
 * This function iterates over available DSP cores and returns the handle to first core
 * that meets the following criteria:
 * - It is NOT exclusively used by anyone.
 * - It has a matching appid label
 * - It is in a KILLABLE state. 
 * 
 * This call obtains the exclusive lock on the core. 
 * NOTE: This function will NEVER try to connect to any cores in RUNNING state, even if they are not
 * used by anyone. This might happen the userspace application that started the DSP code crashed prior 
 * to calling easynmc_close() but after easynmc_persist_set(). If this is the case, connecting to 
 * such apps can lead to big problems.
 * 
 * You have to kill these stale DSP apps manually with nmctl or with your own tool.
 * 
 * @param appid Application identifier
 * @return Handle for the core or NULL if no running, unlocked cores with matching appid found 
 */
struct easynmc_handle *easynmc_connect(const char *appid)
{
	struct easynmc_connect_info i; 
	i.desth = NULL; 
	i.appid = appid;
	easynmc_for_each_core(connect_core_cb, 1, &i);
	if (i.desth)
		i.desth->persistent = 1;
	return i.desth;
}

#ifndef __DOXYGEN__
struct easynmc_appdata {
	char appid[EASYNMC_APPID_LEN];
	char userdata[];
};
#endif

static inline size_t raw_appdata_size(struct easynmc_handle *h)
{
	return h->imem32[NMC_REG_APPDATA_SIZE];
}

/**
 * Returns curent appdata size in bytes. 
 * @param h The easynmc handle
 * @return appdata size in bytes
 */
size_t easynmc_appdata_size(struct easynmc_handle *h)
{
	if (!raw_appdata_size(h))
		return 0;
	/* The actual size, minus extra information data */ 
	return raw_appdata_size(h) - sizeof(struct easynmc_appdata);
}

static int kernel_appdata_get(struct easynmc_handle *h, struct nmc_ioctl_buffer *buf)
{
	if (!raw_appdata_size(h))
		return -ENODATA;
	
	int ret = ioctl(h->iofd, IOCTL_NMC3_GET_APPDATA, buf);
	dbg("kernel appdata get: %d len %zu\n", ret, buf->len);	
	return ret;
}

static int kernel_appdata_set(struct easynmc_handle *h, struct nmc_ioctl_buffer *buf)
{
	int ret = ioctl(h->iofd, IOCTL_NMC3_SET_APPDATA, buf);

	/* HACK: Is it a good idea to store appdata len here ? */
	if (ret == 0) 
		h->imem32[NMC_REG_APPDATA_SIZE] = buf->len;

	dbg("kernel appdata set: %d len %zu\n", ret, buf->len);	
	return ret;
}


/**
 * Setup current application-specific data.
 * Associate some arbitary data with the running easynmc application. 
 * The application data is copied internally and can be freed after this call.
 * 
 *
 * @param h The easynmc handle
 * @return 0 if everything went fine
 */
int easynmc_appdata_set(struct easynmc_handle *h, void *data, size_t len)
{
	size_t dlen =  len + sizeof(struct easynmc_appdata);
	struct easynmc_appdata *adata = alloca(dlen);
	struct nmc_ioctl_buffer buf; 
	buf.data = adata;
	buf.len = len;
	int ret;
	
	ret = kernel_appdata_get(h, &buf);
	if (ret == -ENODATA) { 
		adata->appid[0]=0x0; /* NULL AppID */
	} else if (ret)
		return ret;

	memcpy(adata->userdata, data, len);
	return kernel_appdata_set(h, &buf);

}

/**
 * Retrieve current application-specific data from kernel.
 * The atmost len bytes of application-specific data is copied to the supplied buffer 
 *
 * @param h The easynmc handle
 * @param data pointer to user buffer
 * @param len The length of app data in bytes
 * @return The actual number of bytes copied to user buffer
 */
size_t easynmc_appdata_get(struct easynmc_handle *h, void *data, size_t len)
{
	int ret;
	size_t dlen = raw_appdata_size(h);
	if (!dlen)
		return 0; /* Appdata is invalid */

	size_t actual_length  = raw_appdata_size(h) - 
		sizeof(struct easynmc_appdata);
	
	struct easynmc_appdata *adata = alloca(dlen);
	struct nmc_ioctl_buffer buf; 
	buf.data = adata;
	buf.len = dlen;
	
	ret = kernel_appdata_get(h, &buf);
	if (ret)
		return 0;	
	size_t tocopy = (len < actual_length) ? len : actual_length;
	memcpy(data, adata->userdata, tocopy);
	return tocopy;	
}

/**
 * Set the current application identifier.
 *
 * @param h The easynmc handle
 * @param appid pointer to user buffer
 * @return 0 if everything went fine
 */
int easynmc_appid_set(struct easynmc_handle *h, char appid[])
{
	int ret;

	if (strlen(appid) > EASYNMC_APPID_LEN)
		return -EIO;

	if (h->appid)
		free(h->appid);
	h->appid = strdup(appid);

	size_t rawsize = raw_appdata_size(h);
	
	if (!rawsize)
		rawsize = sizeof(struct easynmc_appdata);

	struct easynmc_appdata *adata = alloca(rawsize);
	struct nmc_ioctl_buffer buf; 
	buf.data = adata;
	buf.len = rawsize;

	ret = kernel_appdata_get(h, &buf);
	if (ret && (ret != -ENODATA))
		return ret;

	buf.len = rawsize;
	
	strncpy(adata->appid, appid, EASYNMC_APPID_LEN);
	adata->appid[EASYNMC_APPID_LEN-1] = 0x0;
	
	return kernel_appdata_set(h, &buf);
}

/**
 * Retrieve tihe current application identifier. 
 * The memory for the identifier if manages by the library and should not 
 * be freed or altered by the caller.
 *
 * @param h The easynmc handle
 * @return application id
 */
const char *easynmc_appid_get(struct easynmc_handle *h)
{
	int ret;
	if (h->appid)
		return (const char*) h->appid;

	size_t dlen = h->imem32[NMC_REG_APPDATA_SIZE];
	if (!dlen)
		return NULL; /* AppId is invalid */

	size_t sz = sizeof(struct easynmc_appdata);
	struct easynmc_appdata *adata = alloca(sz);
	struct nmc_ioctl_buffer buf;
	buf.data = adata;
	buf.len = sz;
	ret = kernel_appdata_get(h, &buf);
	if (ret)
		return NULL;
	h->appid = strdup(adata->appid);
	return h->appid;
}

/**
 * Enable application persistence.
 * By default, any running nmc application is terminated by the driver when the device 
 * is closed. In rare cases where this is not required you can call this function to disable this
 * for the current handle, call this function with EASYNMC_PERSIST_ENABLE. 
 * After this call, calling easynmc_close() on the handle will place the core in 
 * EASYNMC_CORE_KILLABLE state. 
 * 
 * 
 *
 * @param h The easynmc handle
 * @param appid pointer to user buffer
 * @return 0 if everything went fine
 */
int easynmc_persist_set(struct easynmc_handle *h, enum easynmc_persist_state status)
{
	h->persistent = (status == EASYNMC_PERSIST_ENABLE) ? 1 : 0;
	int ret = ioctl(h->iofd, IOCTL_NMC3_NMI_ON_CLOSE, &status);
	if (ret != 0) {
		perror("ioctl");
	}
	return ret;
}

/**
 * @}
 */
