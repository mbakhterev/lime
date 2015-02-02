#include "rune.h"

#include <stdlib.h>
#include <assert.h>

unsigned runerun(const unsigned char *const p)
{
	unsigned t = ~*p;

	if(t & 0x80) { return 1; }

	// в utf-8 не бывает первого байта вида ff или 10xxxxxx
	assert(t && (t & 0xc0) != 0x40);

	// двоичный поиск, аналогичный nlz
	unsigned n = 0;
	if((t & 0xf0) == 0) { n += 4; t <<= 4; }
	if((t & 0xc0) == 0) { n += 2; t <<= 2; }
	if((t & 0x80) == 0) { n += 1; }

	return n;
}

unsigned runelen(unsigned v)
{
	if(v < 1 << 07) { return 1; }
	if(v < 1 << 11) { return 2; }
	if(v < 1 << 16) { return 3; }
	if(v < 1 << 21) { return 4; }
	if(v < 1 << 26) { return 5; }
	if(v < 1 << 31) { return 6; }

	assert(0);
}

static unsigned bitzero(const unsigned v, const unsigned n)
{
	return !(v & (1 << n));
}

static unsigned append(const unsigned v, const unsigned n)
{
	assert((n & 0xc0) == 0x80);
	return (v << 6) | (n & 0x3f);
}

const unsigned char * readrune(const unsigned char * p, unsigned *const vp)
{
	unsigned v = *p;
	p += 1;

	// быстрый вариант
	if(v < 1 << 7) { *vp = v; return p; }

	assert((v & 0xc0) != 0x80);

	// проанализировали бит 7, можно сбрасывать
	v ^= 0x80;

	// немного паранойи
	assert(sizeof(unsigned) >= 2);

	// номер анализируемого бита
	unsigned shift = 6;

	if(bitzero(v, shift))
	{
		// 2nd байт должен быть
		assert(0);	
	}

	v ^= 1 << shift;
	v = append(v, *p);
	p += 1;
	shift += 5;

	if(bitzero(v, shift))
	{
		*vp = v;
		return p;
	}

	// 3rd байт

	v ^= 1 << shift;
	v = append(v, *p);
	p += 1;
	shift += 5;

	if(bitzero(v, shift))
	{
		*vp = v;
		return p;
	}

	assert(sizeof(unsigned) >= 4);

	for(unsigned i = 4; i <= 6; i += 1)
	{
		v ^= 1 << shift;
		v = append(v, *p);
		p += 1;
		shift += 5;

		if(bitzero(v, shift))
		{
			*vp = v;
			return p;
		}
	}

	assert(0);
}

unsigned char * writerune(unsigned char *const p, unsigned v)
{
	if(v < 1 << 7)
	{
		*p = v;
		return p + 1;
	}

	const unsigned l = runelen(v);
	assert(l > 1);

	unsigned char * t = p + l;

	while(p != (t -= 1))
	{
		*t = 0x80 | (v & 0x3f);
		v >>= 6;
	}

	assert(v == (v & 0xff >> (l + 1)));

	*t = (0xff << (8 - l)) | v;

	return p + l;
}
