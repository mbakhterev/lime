#include "atomtab.h"
#include "array.h"
#include "rune.h"
#include "heapsort.h"
#include "util.h"

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

AtomTable mkatomtab(void) {
	return (AtomTable) {
		.temp = NULL,
		.templen = 0,
// 		.count = 0,
		.atoms = mkarray(sizeof(Atom *)),
		.index = mkarray(sizeof(unsigned))
	};
}

Atom tabatoms(const AtomTable *const t, const unsigned n) {
	assert(n < t->atoms.count);
	return ((Atom *)t->atoms.buffer)[n];
}

Atom tabindex(const AtomTable *const t, const unsigned i) {
	assert(i < t->index.count);
	const unsigned *const I = t->index.buffer;
	assert(I[i] < t->atoms.count);
	return ((Atom *)t->atoms.buffer)[I[i]];
}

unsigned tabcount(const AtomTable *const t) {
	return t->atoms.count;
}

unsigned atomhint(const Atom a) {
	return ((const unsigned char *)a)[0];
}

unsigned atomlen(const Atom a) {
	unsigned n;
	readrune((const unsigned char *)a + 1, &n);

	DBG(DBGALEN, "n: %u", n);

	return n;
}

unsigned atomid(const Atom a) {
	const unsigned char *const p = a;
	unsigned n;
	readrune(p + 1 + runerun(p + 1), &n);
	return n;
}

const unsigned char * atombytes(const Atom a) {
	return (const unsigned char *)a - atomlen(a);
}

void resetatomtab(AtomTable *const t) {
	for(unsigned i = 0; i < t->atoms.count; i += 1) {
		free((void *)atombytes(tabatoms(t, i)));
	}

	t->atoms.count = 0;
	t->index.count = 0;
}

enum {
	MAXHINT = 255,
	MAXLEN = (1U << (sizeof(unsigned) * 8 - 1)) - 1,
	TEMPDEFAULTLEN = 64
};

static unsigned atomstorelen(const unsigned len, const unsigned num) {
	// Объём необходимой для хранения атома памяти:
	//	len + 1(hint) + runelen(len) + runelen(num).

	return len + 1 + runelen(len) + runelen(num);
}

static void newtemp(AtomTable *const t, const unsigned len) {
	t->temp = malloc(len);
	assert(t->temp);
	t->templen = len;
}

static void renewtemp(AtomTable *const t, const unsigned len) {
	if(t->temp) { free(t->temp); }
	newtemp(t, len);
}

static void tunetemp(AtomTable *const t, const unsigned len) {
	void *const p = realloc(t->temp, len);
	assert(p);
	t->temp = p;
	t->templen = len;
}

// static void growtab(AtomTable *const t) {
// 	t->count += 1;
// 
// 	assert(t->index.capacity == t->atoms.capacity);
// 
// 	if(t->count * sizeof(Atom) <= t->atoms.capacity) {
// 		return;
// 	}
// 
// // 	unsigned k = t->count;
// // 	t->capacity = t->count;
// // 	t->index = exporesize(t->index, &k, sizeof(Atom));
// // 	t->atoms = exporesize(t->atoms, &t->capacity, sizeof(Atom));
// 
// 	DBG(DBGGT, "%s", "resizing");
// 
// 	exporesize(&t->index, t->count);
// 	exporesize(&t->atoms, t->count);
// }

static int cmpatombuff(const Atom,
	const unsigned char, const unsigned, const unsigned char *const);

static int cmpatoms(const void *const D, const unsigned i, const unsigned j) {
	const Atom *const A = (const Atom *)D;
	const Atom a = A[i];
	const Atom b = A[j];
	return cmpatombuff(a, atomhint(b), atomlen(b), atombytes(b));
}

static void grabtemp(AtomTable *const t,
	const unsigned char hint, const unsigned len, const unsigned pos) {
//	growtab(t);
//	assert(t->count > pos);

	tunetemp(t, atomstorelen(len, pos));
	
	// Формируем атом
	unsigned char *const a = t->temp + len;
	writerune(writerune(a + 1, len), pos);

	a[0] = 0;
	DBG(DBGGT, "atom: %s", atombytes(a));
	a[0] = hint;

	// temp нужно перезарядить
	newtemp(t, TEMPDEFAULTLEN);

// 	((Atom*)t->atoms.buffer)[pos] = a;
// 	((Atom*)t->index.buffer)[pos] = a;

//	append(&t->atoms, &a);
//	append(&t->index, &a);

	const unsigned k = t->atoms.count;
	append(&t->atoms, &a);
//	append(&t->index, &t->atoms.count);
	append(&t->index, &k);

	assert(t->atoms.count == t->index.count);
//	assert(t->atoms.capacity == t->index.capacity);
	assert(t->atoms.count * t->atoms.itemlength <= t->atoms.capacity);
	assert(t->index.count * t->index.itemlength <= t->index.capacity);

//	heapsort((const void **)t->index.buffer, t->index.count, cmpatoms);

	heapsort((const void *const)t->atoms.buffer,
		(unsigned *const)t->index.buffer, t->index.count, cmpatoms);
}

unsigned loadatom(AtomTable *const t, FILE *const f) {
	int n;
	unsigned k;
	unsigned l;

	if(fscanf(f, "%2x.%u", &k, &l) == 2) { } else {
		ERROR("%s", "can't detect hint.length");
	}

	// Если атом новый, то его порядковый номер будет t->count
	const unsigned pos = t->atoms.count;
	const unsigned len = l;
	const unsigned hint = k;

	if(hint <= MAXHINT && len <= MAXLEN) { } else {
		ERROR("hint.length is out of limit 0x%x.0x%x", MAXHINT, MAXLEN);
	}

	if(fscanf(f, ".\"%n", &n) == 0 && n == 2) { } else {
		ERROR("%s", "can't detect .\"-leading");
	}

	DBG(DBGRDA, "len: %u; t->templen: %u", len, t->templen);

	if(t->templen >= len) {
		// В t->temp достаточно места для чтения байтов атома, и
		// достаточна вероятность, что атом есть в таблице. Поэтому, не
		// делаем здесь realloc
	}
	else {
		// Увеличение t->temp под загружаемые данные и служебную
		// информацию. Последнее в надежде, что tunetemp в grabtemp
		// сработает быстрее
		l = atomstorelen(len, pos);
		DBG(DBGRDA, "l: %u", l);
		renewtemp(t, l);
	}

	if(fread(t->temp, 1, len, f) == len) { } else {
		ERROR("can't load %u bytes", len);
	}

	if(fscanf(f, "\"%n", &n) == 0 && n == 1) { } else {
		ERROR("%s", "can't detect \"-finishing");
	}

	if((k = lookbuffer(t, hint, len, t->temp)) != -1) {
		return k;
	}

	DBG(DBGRDA, "k: %d", k);

	grabtemp(t, hint, len, pos);

	return pos;
}

static unsigned min(const unsigned a, const unsigned b) {
	return a < b ? a : b;
}

static int cmpatombuff(const Atom a,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff) {
	// Сначала порядок по оттенку. Из области отличных от hint окрасок
	// двоичный поиск выйдет быстро, поэтому return в менее вероятной
	// else-ветке
	const unsigned ah = *a;
	if(ah == hint) { } else {
		return 1 - ((ah < hint) << 1);
	}

	const unsigned al = atomlen(a);
	const int t = memcmp(atombytes(a), buff, min(al, len));
	if(t) {
		return t;
	}

	// Остаётся сравнить длины
	return 1 - (al == len) - ((al < len) << 1);
}

// static unsigned middle(const unsigned a, const unsigned b) {
// 	return a + (b - a) / 2;
// }

unsigned lookbuffer(AtomTable *const t,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff) {
	assert(len <= MAXLEN);

	DBG(DBGLOOK, "t->atoms.count: %u; t->index.count: %u",
		t->atoms.count, t->index.count);

	if(t->atoms.count) { } else {
		return -1;
	}

	// Подготовка к поиску. A - отсортированный массив ссылок на атомы
	const unsigned *const I = t->index.buffer;
	const Atom *const A = t->atoms.buffer;
	unsigned r = t->index.count - 1;
	unsigned l = 0;

	while(l < r) {
		const unsigned m = middle(l, r);

		if(cmpatombuff(A[I[m]], hint, len, buff) < 0) {
			l = m + 1;
		}
		else {
			r = m;
		}

		DBG(DBGLOOK, "l: %u; r: %u", l, r);
	}

	if(l == r && cmpatombuff(A[I[l]], hint, len, buff) == 0) {
		DBG(DBGLOOK, "found: %u", l);
		return atomid(A[I[l]]);
	}

	return -1;
}

unsigned readbuffer(AtomTable *const t,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff) {
	const unsigned pos = t->atoms.count;
	const unsigned k = lookbuffer(t, hint, len, buff);
	if(k != -1) { return k; }

	const unsigned l = atomstorelen(len, pos);
	if(t->templen >= l) { } else {
		renewtemp(t, l);
	}

	memcpy(t->temp, buff, len);
	grabtemp(t, hint, len, pos);

	return pos;
}

unsigned loadtoken(AtomTable *const t, FILE *const f,
	const unsigned char hint, const char *const format) {
	assert(strlen(format) <= 32);

	// С учётом места под последний '\0'
	const unsigned chunklen = TEMPDEFAULTLEN - 1;
	char fmt[64];
	sprintf(fmt, "%%%u%s%%n", chunklen, format);
	
	DBG(DBGLDT, "fmt: %s", fmt);

	if(t->templen >= chunklen) { } else {
		renewtemp(t, TEMPDEFAULTLEN);
	}

	unsigned loaded = 0;
	unsigned l;

	while(scanf(fmt, t->temp + loaded, &l) == 1) {
		loaded += l;
		if(t->templen < loaded + chunklen) {
			tunetemp(t, t->templen + TEMPDEFAULTLEN);
		}
	}

	if(loaded > 0) { } else {
		ERROR("can't detect token. format: %s", fmt);
	}

	DBG(DBGLDT, "tmp: %s", t->temp);

	const unsigned len = loaded;

	if((l = lookbuffer(t, hint, len, t->temp)) != -1) {
		return l;
	}

	const unsigned pos = t->atoms.count;
	grabtemp(t, hint, len, pos);
	return pos;
}
