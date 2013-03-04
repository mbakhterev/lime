#include "atomtab.h"
#include "buffer.h"
#include "rune.h"
#include "heapsort.h"

#include <assert.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define ERROR(fmt, ...) \
	error(EXIT_FAILURE, errno, "%s(%u:%u) error: " fmt, \
		unitname, item, field, __VA_ARGS__)

#define DBGRDA 1
#define DBGALEN 2
#define DBGGRAB 4
#define DBGLOOK 8
#define DBGLDT 16

// #define DBGFLAGS 0
// #define DBGFLAGS (DBGRDA | DBGALEN | DBGGRAB | DBGLOOK)
#define DBGFLAGS DBGLDT

#define DBG(f, fmt, ...) \
	(void)((f & DBGFLAGS) \
		&& fprintf(stderr, \
			__FILE__ ":%u\t" fmt "\n", __LINE__, __VA_ARGS__))

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
	for(unsigned i = 0; i < t->count; i += 1) {
		free((void *)atombytes(t->atoms[i]));
	}

	t->count = 0;
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

static void growtab(AtomTable *const t) {
	t->count += 1;

	if(t->count <= t->capacity) {
		return;
	}

	unsigned k = t->count;
	t->capacity = t->count;
	t->index = exporesize(t->index, &k, sizeof(Atom));
	t->atoms = exporesize(t->atoms, &t->capacity, sizeof(Atom));
	assert(t->capacity == k);
}

static int cmpatombuff(const Atom,
	const unsigned char, const unsigned, const unsigned char *const);

static int cmpatoms(const void *const a, const void *const b) {
	return cmpatombuff(a, atomhint(b), atomlen(b), atombytes(b));
}

static void grabtemp(AtomTable *const t,
	const unsigned char hint, const unsigned len, const unsigned pos) {
	growtab(t);
	assert(t->count > pos);

	tunetemp(t, atomstorelen(len, pos));
	
	// Формируем атом
	unsigned char *const a = t->temp + len;
	writerune(writerune(a + 1, len), pos);

	a[0] = 0;
	DBG(DBGGRAB, "atom: %s", atombytes(a));
	a[0] = hint;

	// temp нужно перезарядить
	newtemp(t, TEMPDEFAULTLEN);

	t->atoms[pos] = a;
	t->index[pos] = a;
	heapsort((const void **)t->index, t->count, cmpatoms);
}

unsigned loadatom(AtomTable *const t, FILE *const f) {
	int n;
	unsigned k;
	unsigned l;

	if(fscanf(f, "%2x.%u", &k, &l) == 2) { } else {
		ERROR("%s", "can't detect hint.length");
	}

	// Если атом новый, то его порядковый номер будет t->count
	const unsigned pos = t->count;
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

static unsigned middle(const unsigned a, const unsigned b) {
	return a + (b - a) / 2;
}

unsigned lookbuffer(AtomTable *const t,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff) {
	assert(len <= MAXLEN);

	DBG(DBGLOOK, "t->count: %u", t->count);

	if(t->count) { } else {
		return -1;
	}

	// Подготовка к поиску. A - отсортированный массив ссылок на атомы
	const Atom * A = t->index;
	unsigned r = t->count - 1;
	unsigned l = 0;

	while(l < r) {
		const unsigned m = middle(l, r);

		if(cmpatombuff(A[m], hint, len, buff) < 0) {
			l = m + 1;
		}
		else {
			r = m;
		}

		DBG(DBGLOOK, "l: %u; r: %u", l, r);
	}

	if(l == r && cmpatombuff(A[l], hint, len, buff) == 0) {
		DBG(DBGLOOK, "found: %u", l);
		return atomid(A[l]);
	}

	return -1;
}

unsigned readbuffer(AtomTable *const t,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff) {
	const unsigned pos = t->count;
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
	char fmt[64];
	sprintf(fmt, "%%%u%s%%n", TEMPDEFAULTLEN, format);
	
	DBG(DBGLDT, "fmt: %s", fmt);

	return 0;
}
