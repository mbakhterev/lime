#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DBGFE 1
#define DBGPOOL 2
#define DBGMF 4
#define DBGFL 8

// #define DBGFLAGS (DBGFE)
// #define DBGFLAGS (DBGPOOL)
// #define DBGFLAGS (DBGMF)
// #define DBGFLAGS (DBGFL)

#define DBGFLAGS 0

// Будем держать пулл звеньев для списков и пулл узлов. Немного улучшит
// эффективность. Списки будут кольцевые, поэтому храним только один указатель.
// Это указатель (ptr) на конец списка. На начало указывает ptr->next

static List * freeitems = NULL;

List *tip(const List *const l)
{
	assert(l);
	return l->next;
}

// Откусить первый элемент
List *tipoff(List **const lptr)
{
	assert(lptr);

	List *const l = *lptr;
	assert(l);

	// Первый элемент здесь
	List *const n = l->next;

	if(l != n)
	{
		// Список из >1 элементов
		l->next = n->next;
	}
	else
	{
		*lptr = NULL;
	}

	// Закольцовываем в одноэлементный список
	n->next = n;

	return n;
}

static List *newlist(const Ref r)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case NODE:
	case PTR:
	case LIST:
	case FORM:
	case MAP:
	case AREA:
	case FREE:
		break;
	
	default:
		assert(0);
	}

	List *l = NULL;

	if(freeitems)
	{
		DBG(DBGPOOL, "tipping off: %p", (void *)freeitems);
		
		l = tipoff(&freeitems);
		assert(l->ref.code == FREE && l->next == l);
	}
	else
	{
		l = malloc(sizeof(List));
		assert(l);
		l->next = l;
	}

	l->ref = r;
	return l;
}

List *readrefs(const Ref R[], const unsigned N)
{
	List *l = NULL;

	for(unsigned i = 0; i < N; i += 1)
	{
		l = append(l, newlist(R[i]));
	}
	return l;
}

List *append(List *const k, List *const l)
{
	if(k == NULL)
	{
		return l;
	}

	if(l == NULL)
	{
		return k;
	}

	List *const h = k->next;

	// За последним элементом списка k должен следовать первый элемент l
	k->next = l->next;

	// А за последним элементом l должен идти первый элемент k;
	l->next = h;

	// конец нового списка - это l;
	return l;
}

int forlist(List *const k, Oneach fn, void *const ptr, const int key)
{
	if(k == NULL)
	{
		return key;
	}

	List *p = k;
	List *q = k->next;
	int r;
	do
	{
		DBG(DBGFE, "k: %p; p: %p", (void *)k, (void *)p);

		p = q;
		q = p->next;
		r = fn(p, ptr);
	} while(r == key && p != k);

	return r;
}

// FState - fork/free state

typedef struct 
{
	List *list;
	Array *const nodemap;
	const unsigned bound;
	const unsigned from;
	const unsigned to;
	unsigned count;
} FState;

static int forkitem(List *const k, void *const ptr)
{
	assert(k);

	// Проверка того, что всё аккуратно со структурой списка и
	// external-битом. Внутри списков им могут быть помечены только списки и
	// узлы

	switch(k->ref.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		assert(!k->ref.external);
		break;
	
	case MAP:
		assert(k->ref.external);
		break;
	
	case NODE:
	case LIST:
		break;
	
	default:
		assert(0);
	}

	FState *const fs = ptr;
	assert(fs);

	// Текущая позиция в списке
	const unsigned cur = fs->count;

	if(cur < fs->from)
	{
		// Не добрались до начала отрезка списка, продолжаем
		fs->count += 1;
		return 0;
	}

	fs->list = append(fs->list, newlist(forkref(k->ref, fs->nodemap)));

	if(cur < fs->to)
	{
		// Если не добрались до последнего элемента, переходим к
		// следующему

		fs->count += 1;
		return 0;
	}

	// Дошли до последнего
	return 1;
}

static List *megafork(
	const List *const k, const unsigned from, const unsigned to,
	Array *const map, unsigned *const correct);

List *forklist(const List *const k)
{
	return transforklist(k, NULL);
}

List *transforklist(const List *const k, Array *const map)
{
	unsigned correct = 0;
	List *const l = megafork(k, 0, -1, map, &correct);
	assert(correct);

	return l;
}

List *forklistcut(
	const List *const k, const unsigned from, const unsigned to,
	unsigned *const correct)
{
	return megafork(k, from, to, NULL, correct);
}

List *megafork(
	const List *const k, const unsigned from, const unsigned to,
	Array *const map, unsigned *const correct)
{
	// Диапазон должен быть задан корректно. -1 -- самый большой unsigned

	if(from <= to && (to < MAXLEN || to == -1))
	{
	}
	else
	{
		*correct = 0;
		return NULL;
	}

	DBG(DBGMF, "diapason OK: %u -> %u", from, to);

	// Если k == NULL, то надо вернуть NULL. Но при этом всё будет корректно
	// только если (from; to) == (0; -1) -- только весь NULL-евой список
	// можно взять.

	if(k == NULL)
	{
		*correct = (from == 0) && (to == -1);
		return NULL;
	}

	// Здесь уже некоторая активность

	DBG(DBGMF, "list is sound: %p", (void *)k);

	// Может понадобится вырезать последний элемент из списка. Диапазон уже
	// проверен и известно, что список не NULL

	if(from == -1)
	{
		*correct = 1;
		return newlist(k->ref);
	}

	FState fs =
	{
		.list = NULL,
		.nodemap = map,
		.from = from,
		.to = to,
		.count = 0
	};

	const unsigned rv = forlist((List *)k, forkitem, &fs, 0);

	// Успешными нужно считать два варианта:
	// 1.	to == -1 и rv == 0 -- это означает, что пройден весь список.
	// 2.	count == to и rv == -1 -- это означает, что прошли отрезок.

	DBG(DBGMF, "from: %u; to: %u; count: %u; rv: %u",
		fs.from, fs.to, fs.count, rv);

	if((fs.to == -1 && rv == 0) || (fs.count == to && rv == 1))
	{
		*correct = 1;
		return fs.list;
	}
	else
	{
		*correct = 0;
		freelist(fs.list);
		return NULL;
	}

	assert(0);
}

static int releaser(List *const l, void *const p)
{
	assert(l);

	freeref(l->ref);

	l->ref.code = FREE;

	DBG(DBGPOOL, "pooling: %p", (void *)l);

	l->next = l;
	freeitems = append(freeitems, l);

	return 0;
}

void freelist(List *const l)
{
	FState fs = { .list = NULL };
	forlist(l, releaser, &fs, 0);
}

// Состояние для прохода со счётчиками

typedef struct
{
	Ref *const refs;

	// N - ограничение сверху; n - пройдено элементов
	const unsigned N;
	unsigned n;
} CState;

static int writer(List *const k, void *const ptr)
{
	CState *const st = ptr;

	if(st->n < st->N)
	{
		st->refs[st->n] = k->ref;
		st->n += 1;
		return 0;
	}
	
	return 1;
}

unsigned writerefs(const List *const l, Ref refs[], const unsigned N)
{
	CState st = { .refs = refs, .N = N, .n = 0 };

	forlist((List *)l, writer, &st, 0);
	assert(N >= st.n);

	return st.n;
}

static int counter(List *const l, void *const ptr)
{
	CState *const st = ptr;
	assert(st->n < st->N);
	st->n += 1;
	return 0;
}

unsigned listlen(const List *const l)
{
	CState st = { .refs = NULL, .N = MAXNUM, .n = 0 };
	forlist((List *)l, counter, &st, 0);
	assert(st.n < MAXNUM);
	return st.n;
}

void formlist(List L[], const Ref R[], const unsigned N)
{
	assert(L && R);
	assert(N < MAXNUM);

	if(N > 0)
	{
		// Чтобы на создаваемый список можно было ссылаться по адресу
		// первого элемента массива L, этот элемент (по семантике)
		// должен быть последней ссылкой в списке ссылок

		for(unsigned i = 1; i <= N - 1; i += 1)
		{
			DBG(DBGFL, "linking: %u -> %u; ref: %u",
				i - 1, i, R[i - 1].u.number);

			L[i - 1].next = &L[i];
			L[i].ref = R[i - 1];
		}

		L[N - 1].next = &L[0];
		L[0].ref = R[N - 1];
	}
}

static int skipone(List *const l, void *const ptr)
{
	CState *const st = ptr;

	if(st->n < st->N)
	{
		// Ещё не дошли до цели. Пропускаем
		st->n += 1;
		return 0;
	}

	// Добрались до цели, копируем текущую Ref из списка

	*(st->refs) = l->ref;
	return 1;
}

Ref listnth(const List *const l, const unsigned N)
{
	assert(N < MAXNUM);

	Ref R = reffree();

	CState st =
	{
		.N = N,
		.n = 0,
		.refs = &R
	};

	// Небольшая проверка на целостность
	assert(forlist((List *)l, skipone, &st, 0) != 0 || R.code == FREE);

	return R;
}

unsigned splitpair(const Ref p, Ref R[])
{
	if(p.code != LIST)
	{
		return 0;
	}

	const unsigned len = listlen(p.u.list);
	if(len != 2)
	{
		return 0;
	}

	return writerefs(p.u.list, R, len) == len;
}

typedef struct
{
	const Ref **const R;
	unsigned n;
	const unsigned N;
} SState;

static int splitone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	SState *const st = ptr;
	
	if(st->n < st->N)
	{
		st->R[st->n] = &l->ref;
		st->n += 1;

		return 0;
	}

	return 1;
}

unsigned splitlist(const List *const l, const Ref *R[], const unsigned len)
{
	SState st =
	{
		.R = R,
		.N = len,
		.n = 0
	};

	if(forlist((List *)l, splitone, &st, 0))
	{
		// Значит, вышли раньше, чем дошли до конца списка, пройдя len
		// элементов

		return 0;
	}

	// Если вышли в конце списка, то совпадение определяется количеством
	// пройденных звеньев

	return st.N == st.n;
}
