#include "construct.h"
#include "util.h"
#include "rune.h"

#include <assert.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define DBGRDA	1
#define DBGALEN	2
#define DBGGRAB	4
#define DBGLOOK	8
#define DBGLDT	16
#define DBGGT	32	

// #define DBGFLAGS (DBGRDA | DBGALEN | DBGGRAB | DBGLOOK)
// #define DBGFLAGS DBGLDT
// #define DBGFLAGS (DBGRDA | DBGGRAB | DBGLOOK | DBGGT)

#define DBGFLAGS 0

static int kcmp(const void *const D, const unsigned i, const void *const key)
{
	const Atom a = ((Atom *)D)[i];
	const AtomPack *const ap = key;

	const unsigned len = ap->length;
	const unsigned char hint = ap->hint;
	const unsigned char *const buff = ap->bytes;

	// Сначала порядок по оттенку. Из области отличных от hint окрасок
	// двоичный поиск выйдет быстро, поэтому return в менее вероятной
	// else-ветке

	const unsigned ah = *a;
	if(ah != hint)
	{
		return 1 - ((ah < hint) << 1);
	}

	const unsigned al = atomlen(a);
	const int t = memcmp(atombytes(a), buff, min(al, len));
	if(t)
	{
		return t;
	}

	// Остаётся сравнить длины
	return 1 - (al == len) - ((al < len) << 1);
}

static int icmp(const void *const D, const unsigned i, const unsigned j)
{
	const AtomPack ap = atompack(((Atom *)D)[j]);
	return kcmp(D, i, &ap);
}

Array *newatomtab(void)
{
	return newmap(ATOM, sizeof(Atom), icmp, kcmp);
}

void freeatomtab(Array *const t)
{
	assert(t && t->code == ATOM);

	if(t->u.data)
	{
		assert(t->count && t->index);

		Atom *const D = t->u.data;
		for(unsigned i = 0; i < t->count; i += 1)
		{
			free((void *)atombytes(D[i]));
		}
	}

	freemap(t);
}

// Структура атома: byte \0 hint length. length - это длина массива байтов, без
// дополнительного \0, который нужен для удобства работы со всяческими
// printf-ами.

unsigned atomhint(const Atom a)
{
	assert(a);
	return ((const unsigned char *)a)[0];
}

unsigned atomlen(const Atom a)
{
	assert(a);

	unsigned n;
	readrune((const unsigned char *)a + 1, &n);

	DBG(DBGALEN, "n: %u", n);

	return n;
}

const unsigned char * atombytes(const Atom a)
{
	assert(a);

	// - 1 -- это поправка на дополнительный \0
	return (const unsigned char *)a - atomlen(a) - 1;
}

AtomPack atompack(const Atom a)
{
	return (AtomPack)
	{
		.bytes = atombytes(a),
		.length = atomlen(a),
		.hint = atomhint(a)
	};
}

static unsigned atomstorelen(const unsigned len)
{
	// Объём необходимой для хранения атома памяти:
	//	len + 1(\0) + 1(hint) + runelen(len).

	return len + 1 + 1 + runelen(len);
}

static unsigned grabpack(Array *const t, const AtomPack *const ap)
{
	// Формируем атом
	const unsigned len = ap->length;
	const unsigned hint = ap->hint;

	// С поправкой на дополнительный \0
	unsigned char *const a = (unsigned char *)ap->bytes + len + 1;

	writerune(a + 1, len);
	
	a[0] = hint;
	a[-1] = 0;
	DBG(DBGGT, "atom: %s", atombytes(a));

	return readin(t, &a);
}

Ref loadatom(Array *const t, FILE *const f)
{
	assert(t && t->code == ATOM);

	int n;
	unsigned k;
	unsigned l;

	if(fscanf(f, "%2x.%u", &k, &l) != 2)
	{
		ERR("%s", "can't detect hint.length");
	}

	const unsigned len = l;
	const unsigned hint = k;

	if(hint > MAXHINT || len > MAXLEN)
	{
		ERR("hint.length is out of limit 0x%x.0x%x", MAXHINT, MAXLEN);
	}

	if(fscanf(f, ".\"%n", &n) != 0 || n != 2)
	{
		ERR("%s", "can't detect .\"-leading");
	}

	l = atomstorelen(len);
	DBG(DBGRDA, "l: %u", l);

	unsigned char *const tmp = malloc(l);
	assert(tmp);

	if(fread(tmp, 1, len, f) != len)
	{
		ERR("can't load %u bytes", len);
	}

	if(fscanf(f, "\"%n", &n) != 0 || n != 1)
	{
		ERR("%s", "can't detect \"-finishing");
	}

	const AtomPack ap = { .bytes = tmp, .length = len, .hint = hint };

	if((k = lookup(t, &ap)) != -1)
	{
		free(tmp);
		return refatom(k);
	}

	DBG(DBGRDA, "k: %d", k);

	return refatom(grabpack(t, &ap));
}

static unsigned lookpack(Array *const t, const AtomPack ap)
{
	assert(t && t->code == ATOM);
	assert(ap.bytes && ap.length <= MAXLEN && ap.hint <= MAXHINT);

	DBG(DBGLOOK, "t->count: %u", t->count);

	return lookup(t, &ap);
}

Ref readpack(Array *const t, const AtomPack ap)
{
	assert(t && t->code == ATOM);

	const unsigned k = lookpack(t, ap);
	if(k != -1) { return refatom(k); }

	const unsigned len = ap.length;
	unsigned char *const tmp = malloc(atomstorelen(len));	
	memcpy(tmp, ap.bytes, len);

	const AtomPack a = { .bytes = tmp, .hint = ap.hint, .length = len };

	return refatom(grabpack(t, &a));
}

Ref readtoken(Array *const t, const char *const str)
{
	return readpack(t, strpack(0, str));
}

AtomPack strpack(const unsigned hint, const char *const str)
{
	assert(hint < MAXHINT);
	assert(str);

	size_t len = strlen(str);
	assert(len < MAXLEN);

	return (AtomPack)
	{
		.length = len,
		.bytes = (const unsigned char *)str,
		.hint = hint
	};
}

Ref loadtoken(
	Array *const t, FILE *const f,
	const unsigned char hint, const char *const format)
{
	assert(f);
	assert(t && t->code == ATOM);
	assert(strlen(format) <= 32);

	// С учётом места под последний '\0'
	const unsigned strchunklen = CHUNKLEN - 1;
	char fmt[64];
	sprintf(fmt, "%%%u%s%%n", strchunklen, format);
	
	DBG(DBGLDT, "fmt: %s", fmt);

	unsigned chunkcnt = 0;
	unsigned loaded = 0;
	unsigned l;

	unsigned char *tmp = expogrow(NULL, CHUNKLEN, chunkcnt);
	chunkcnt += 1;

	while(fscanf(f, fmt, tmp + loaded, &l) == 1)
	{
		loaded += l;
		if(loaded + CHUNKLEN >= chunkcnt * CHUNKLEN)
		{
			tmp = expogrow(tmp, CHUNKLEN, chunkcnt);
			chunkcnt += 1;
		}
	}

	if(loaded <= 0)
	{
		ERR("can't detect token. format: %s", fmt);
	}

	DBG(DBGLDT, "tmp: %s", tmp);

	AtomPack ap = { .bytes = tmp, .length = loaded, .hint = hint };
	if((l = lookpack(t, ap)) != -1)
	{
		free(tmp);
		return refatom(l);
	}

	const void *const p
		= realloc((void *)ap.bytes, atomstorelen(ap.length));

	assert(p);
	ap.bytes = p;

	return refatom(grabpack(t, &ap));
}

Atom atomat(const Array *const A, const unsigned id)
{
	assert(A && A->code == ATOM);
	assert(id < A->count);

	return ((const Atom *const)A->u.data)[id];
}
