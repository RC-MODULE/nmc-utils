#ifndef LIBEASYNMC_H_H
#define LIBEASYNMC_H_H

#include <stdint.h>
#include <libelf.h>
#include <gelf.h>
#include <linux/easynmc.h>

#define  NMC_REG_CODEVERSION  (0x100)
#define  NMC_REG_ISR_ON_START (0x101)
#define  NMC_REG_CORE_STATUS  (0x102)
#define  NMC_REG_CORE_START   (0x103)
#define  NMC_REG_PROG_ENTRY   (0x104)
#define  NMC_REG_PROG_RETURN  (0x105)


extern int g_libeasynmc_debug;
extern int g_libeasynmc_errors;


struct easynmc_handle;

struct easynmc_section_filter {
	const char* name;
	int (*handle_section)(struct easynmc_handle *h, char* name, FILE *rfd, GElf_Shdr shdr);
	struct easynmc_section_filter *next;
};

struct easynmc_handle {
	int       id;
	int       iofd;
	int       memfd;
	char*     imem;
	uint32_t *imem32;
	uint32_t  imem_size;
	/* Private data */
	struct easynmc_section_filter *sfilters; 
	int       argoffset;
	int       argdatalen;
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)                               \
	(sizeof(a) / sizeof(*(a)))                     
#endif

enum easynmc_core_state {
	EASYNMC_CORE_COLD, 
	EASYNMC_CORE_IDLE,
	EASYNMC_CORE_RUNNING,
	EASYNMC_CORE_PAUSED, /* Reserved, needs bootcode support */
	EASYNMC_CORE_INVALID
};


#define ABSLOAD_FLAG_FORCE    (1<<0)
#define ABSLOAD_FLAG_STDIO    (1<<1)
#define ABSLOAD_FLAG_ARGS     (1<<2)
#define ABSLOAD_FLAG_SYNCLIB  (1<<3)

#define ABSLOAD_FLAG_DEFAULT  \
	(ABSLOAD_FLAG_STDIO | ABSLOAD_FLAG_ARGS)

struct easynmc_token {
	struct nmc_irq_token tok;
	struct easynmc_handle *h;
};


#define EASYNMC_CORE_ALL   -1
#define EASYNMC_CORE_ANY   -2


struct easynmc_handle *easynmc_open(int coreid);
struct easynmc_handle *easynmc_open_noboot(int coreid);
void easynmc_close(struct easynmc_handle *hndl);

int easynmc_boot_core(struct easynmc_handle *h, int debug);

int easynmc_reset_stats(struct easynmc_handle *h);

enum easynmc_core_state easynmc_core_state(struct easynmc_handle *h);
const char* easynmc_state_name(enum easynmc_core_state state);

int easynmc_load_abs(struct easynmc_handle *h, const char *path, uint32_t* ep, int flags);
int easynmc_set_args(struct easynmc_handle *h, char* self, int argc, char **argv);
int easynmc_start_app(struct easynmc_handle *h, uint32_t entry);
char *easynmc_get_default_ipl(char* name, int debug);

int easynmc_stop_app(struct easynmc_handle *h);
int easynmc_exitcode(struct easynmc_handle *h);


struct easynmc_token *easynmc_token_new(struct easynmc_handle *h, uint32_t events);
int easynmc_token_clear(struct easynmc_token *t);
int easynmc_token_wait(struct easynmc_token *t, uint32_t timeout);
int easynmc_token_cancel_wait(struct easynmc_token *t);

int easynmc_pollmark(struct easynmc_handle *h);

/* Section filters are a quick way to add your own ways of handling stuff */

/* Low-level stuff, normally you won't need those */
int easynmc_send_irq(struct easynmc_handle *h, enum nmc_irq irq);
int easynmc_startupcode_is_compatible(uint32_t codever);
int easynmc_get_core_name(struct easynmc_handle *h, char* str);
int easynmc_get_core_type(struct easynmc_handle *h, char* str);
const char* easynmc_evt_name(int evt);


void easynmc_init_default_filters(struct easynmc_handle *h);
void easynmc_register_section_filter(struct easynmc_handle *h, struct easynmc_section_filter *f);

#endif
