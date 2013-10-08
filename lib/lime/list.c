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
	case LIST:
	case FORM:
	case ENV:
	case PTR:
	case CTX:
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

List *readrefs(const Ref R[])
{
	List *l = NULL;

	for(unsigned i = 0; R[i].code != FREE; i += 1)
	{
		l = append(l, newlist(R[i]));
	}
	return l;
}

List * append(List *const k, List *const l)
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

static List *forknode(const Ref node, const List *const M)
{
	assert(node.code == NODE && isnode(node.u.list));

	// Есть два варианта по M. Когда M != NULL нас просят от-fork-ать узлы и
	// в новом корректно расставить на них ссылки. Если M == NULL, то нас
	// простят просто скопировать ссылки

	if(!M)
	{
		// Простой вариант: просто копировать ссылки. И ссылка должна
		// быть ссылкой

		assert(node.external);
		return newlist(node);
	}

	// Сложный вариант. Может быть два варианта по Ref.external.
	// Если Ref.external, то речь идёт о ссылке и её отображение
	// надо просто скопировать

	if(node.external)
	{
		// Смотрим на то, куда отображается эта ссылка. Надо
		// быть уверенными, что отображение знает о node. Иначе
		// ошибка. Это всё проверит isnode

		const Ref n = refmap(M, node);
		assert(n.code == NODE && isnode(n.u.list));

		return newlist(markext(n));
	}

	// Наконец, определение узла. Сначала создаём копию
	// выражения. Делаем это рекурсивно

	const Ref n
		= newnode(
			nodeverb(node, NULL),
			forkref(nodeattribute(node), M),
			nodeline(node));

	// Процедура tunerefmap сама за-assert-ит попытку
	// сделать не-уникальное отображение

	tunerefmap(M, node, n);

	// Процедура newnode вернёт верную Ref-у
	return newlist(n);
}

// FState - fork/free state

typedef struct 
{
	List *list;
	const List *const nodemap;
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

	List *l = NULL;

	switch(k->ref.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		l = newlist(k->ref);
		break;

	case NODE:
		l = forknode(k->ref, fs->nodemap);
		break;

	case LIST:
	{
		const Ref M = fs->nodemap;

		// from и to работают для списка верхнего уровня. На подсписки
		// это не распространяется. Однако, есть тонкости. Если (M !=
		// NULL), то список надо транслировать, поэтому должно быть
		// (!k->ref.external). В случае (M == NULL) ничего не
		// транслируется, и можно ориентироваться на (k->ref.external)

		if(M)
		{
			assert(!k->ref.external);
			l = newlist(reflist(transforklist(k->ref.u.list, M)));
		}
		else if(!k->ref.external)
		{
			l = newlist(reflist(forklist(k->ref.u.list)));
		}
		else
		{
			l = newlist(k->ref);
		}

		break;
	}
	
	default:
		assert(0);
	}

	fs->list = append(fs->list, l);

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
	const List *const map, unsigned *const correct);

List *forklist(const List *const k)
{
	return transforklist(k, NULL);
}

List *transforklist(const List *const k, const List *const map)
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
	const List *const map, unsigned *const correct)
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

	switch(l->ref.code)
	{
	case LIST:
		// Если ссылка не на внешний список, то чистим его рекурсивно

		if(!l->ref.external)
		{
			freelist(l->ref.u.list);
		}

		break;
	
	case FORM:
		// freeform сама реагирует на external-бит

		freeform(l->ref);
		break;
	}

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
	Ref *refs;

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

	assert(N == st.N);
	assert(N >= st.n);

	if(st.n < N)
	{
		refs[st.n].code = FREE;

	 	// Обнуляем весь union
		memset(&refs[st.n].u, 0, sizeof(refs->u));
		return st.n + 1;
	}

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
	assert(R[N].code == FREE);

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
