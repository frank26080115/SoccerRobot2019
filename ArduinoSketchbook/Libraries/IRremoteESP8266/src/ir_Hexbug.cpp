#include <algorithm>
#include "IRsend.h"

//    H    H   EEEEEE   X   X   BBBBB    U    U    GGGGG
//    H    H   E         X X    B    B   U    U   G     
//    HHHHHH   EEEEE      X     BBBB     U    U   G   GG
//    H    H   E         X X    B    B   U    U   G    G
//    H    H   EEEEEE   X   X   BBBBB     UUUU     GGGGG

#if SEND_HEXBUG

void  IRsend::sendHexbug(uint64_t data, uint16_t nbits, uint16_t repeat)
{
	sendGeneric(
		1750, 320,   // headermark, headerspace,
		1020, 320,   // onemark,    onespace,
		340, 320,    // zeromark,   zerospace,
		0, 2000,      // footermark, gap,
		data, nbits, // data,       nbits,
		38, true,    // frequency,  MSBfirst,
		0, 50        // repeat,     dutycycle
	);
}

#endif