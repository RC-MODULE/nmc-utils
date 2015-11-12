#include <easynmc/aura.h>

#pragma data_section ".aura_rpc_syncbuf"
struct aura_sync_buffer g_aura_syncbuf = { 
	.state = SYNCBUF_IDLE
};

#pragma data_section ".aura_rpc_exports"

/* We make it global to cause link errors if this 
 * file is included anywhere else 
 */

struct aura_header g_aura_header = {
	.magic = AURA_MAGIC_HDR,
	.strlen = AURA_STRLEN,
};
