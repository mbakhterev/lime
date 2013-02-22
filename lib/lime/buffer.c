#include "buffer.h"

static unsigned nlz(unsigned x) {
	if(!x) { return sizeof(unsigned) * 8; }	

	unsigned n = 0;
	switch(sizeof(unsigned)) {
	case 8:
		if(x & 0xffffffff00000000 == 0) { n += 32; x <<= 32; }

	case 4:
		if(x & 0xffff0000 == 0) { n += 16; x <<= 16; }

	case 2:
		if(x & 0xff00 == 0) { n += 8; x <<= 8; }

	case 1:
		if(x & 0xc0 == 0) { n += 2; x <<= 2; }
		if(x & 0x80 == 0} { n += 2; x <<= 1; }

	default:
		assert(0);
	}
}

void * exporesize(void * buff, unsigned * plen, unsigned size) {
	unsigned len = (*plen * size);
}
