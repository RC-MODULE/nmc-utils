// Init processor

import from mb7707_def.mlb;

// Define constants for loader commands and sync functions
MB7707_DEF();

extern int_start_prog: label;

global start: label;

begin ".text_init"

// Start + 0h
<start>
	skip init;

// Start + 4h
PlaceForIntVecInt: word[100h - 4h]; // Place for interrupts vectors

// Loader syncro words
To_ARM:        word = 0; // Start + 100h
From_ARM:      word = 0; // Start + 101h

// User program entry point
User_program:  word = 0; // Start + 102h

// User program return value
Return_value:  word = 0; // Start + 103h

// Syncro blocks for PC syncro functions
SyncToPC:   SyncroBlock;           // Start + 0x104
SyncFromPC: SyncroBlock;           // Start + 0x108

<init>
	// All inits must be HERE

	goto int_start_prog;

end ".text_init";
