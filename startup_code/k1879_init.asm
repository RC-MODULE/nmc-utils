// Init processor

import from k1879_def.mlb;

// Define constants for loader commands and sync functions
K1879_DEF();

extern int_start_prog: label;

global start: label;

begin ".text_init"

// Start + 0h
<start>
	goto init;

// Start + 4h
PlaceForIntVecInt: word[100h - 4h]; // Place for interrupts vectors
	
__init_INTR_CLEAR: label;
	
/* Magic area */
MAGIC_AREA:	
	word[NMC_MAGIC_AREA_LEN];

<init>	
	pswr set 0h;
	pswr clear 0h;
	
	// Disable external interrupts 
	intr_mask = -1;
	.wait;
	intr_req clear 0FFFFh; // Clear interrupt requests
	pc = __init_INTR_CLEAR; // Очистка конвейера по Косорукову
	vnul;
	vnul;
<__init_INTR_CLEAR>
	intr clear 3C0h; // Clean requests #2

	goto int_start_prog;

	
end ".text_init";
