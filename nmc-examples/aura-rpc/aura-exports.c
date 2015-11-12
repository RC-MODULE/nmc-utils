#include <easynmc/easynmc.h>
#include <easynmc/aura-exportfile.h>


AURA_METHOD(echo_u32, echo32, 
	    U32, 
	    U32);

AURA_METHOD(echo_u64, echo64, 
	    U64, 
	    U64);

AURA_METHOD(echo_bin, echobin, 
	    BIN(64), 
	    BIN(64));

AURA_METHOD(echo_u32u32, echou32u32, 
	    U32 U32, 
	    U32 U32);

struct aura_object g_aura_eof;
