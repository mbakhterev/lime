#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGRONE	1
#define DBGIDX	2

// #define DBGFLAGS (DBGRONE)

#define DBGFLAGS (DBGIDX)

#define LNODE	0U
#define LNTH	1U
#define FIN	2U
#define TAIL	3U

// Expand State - структура для сбора информации о процессе переписывания
// атрибутов

typedef struct
{
	List *rewritten;
	const Array *verbs;
} EState;

static List *rewritelistrefs(List *const, const Array *const verbs);

// FIXME: По уму здесь надо бы накапливать куски списка аттриботов, в которах
// нет ссылок на L-узлы и целыми такими кусками цеплять всё в expanded.

static int rewriteref(List *const l, void *const ptr)
{
	EState *const st = ptr;
	assert(st);

	// В результат будет дописан либо l - текущий элемент списка (возможно,
	// с переписанным содержимым), либо список атрибутов L-узла, на который
	// ссылается l->ref (и тогда l будет удалён). В любом случае l
	// необходимо закольцевать. Результат в списке r

	List *r = l;
	r->next = r;

	switch(l->ref.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// Дописываем в результат сам элемент списка
		break;
	
	case LIST:
		// Дописываем в результат сам элемент списка, но с переписанным
		// содержимым

		r->ref.u.list = rewritelistrefs(r->ref.u.list, st->verbs);
		break;

	case NODE:
	{
		const Node *const n = l->ref.u.node;
		const unsigned key = uireverse(st->verbs, n->verb);
		switch(key)
		{
		case LNODE:
		case LNTH:
			freelist(l);
			r = forklist(n->u.attributes);
			break;
		}

		break;
	}

	default:
		assert(0);
	}

	st->rewritten = append(st->rewritten, r);
	return 0;
}

static List *rewritelistrefs(List *const l, const Array *const verbs)
{
	EState st = { .rewritten = NULL, .verbs = verbs };
	forlist(l, rewriteref, &st, 0);
	return st.rewritten;
}

typedef struct
{
	List *L;
	const Array *const verbs;
} DCState;

// Чисто вспомогательная функция, которая анализирует список l. Если он из
// одного элемента и этот элемент LIST, то надо раскрыть скобки. В других
// случаях, пользователь сам знает, что делает

static List *relist(List *const l)
{
	if(l->next == l && l->ref.code == LIST)
	{
		// Чтобы freelist рекурсивно не потёр всё, нужна небольшая
		// манипуляция

		List *t = l->ref.u.list;
		l->ref.u.list = NULL;
		freelist(l);
		return t;
	}

	return l;
}

// Превращаем Ref в ссылку для .LNth. Устанавливаем флаг, если что не так, чтобы
// обработку ошибок вынести на более высокий уровень

static unsigned reftoidx(
	const Ref r, const Array *const verbs, unsigned *const correct)
{
	assert(r.code <= ATOM);
	*correct = 0;

	switch(r.code)
	{
	case NUMBER:
		DBG(DBGIDX, "number: %u", r.u.number);
		*correct = 1;
		return r.u.number;
	
	case ATOM:
	{
		const unsigned key = uireverse(verbs, r.u.number);

		DBG(DBGIDX, "atom: %u -> %u; tail: %u",
			r.u.number, key, (unsigned)TAIL);

		if(key == TAIL)
		{
			*correct = 1;
			return -1;
		}

		break;
	}
	}

	assert(0);
	return -1;
}

static int indexone(List *const i, void *const ptr)
{
	assert(i);
	assert(ptr);

	DCState *const st = ptr;

	{
		const char *const c = dumplist(st->L);
		DBG(DBGIDX, "current list: %s", c);
		free((void *)c);
	}

	unsigned from = -1;
	unsigned to = -1;

	// Начинаем разбирать очередной элемент индекса. errmsg - это сообщение
	// об ошибки

	const char *errmsg = NULL;

	switch(i->ref.code)
	{
// 	case NUMBER:
// 		DBG(DBGIDX, "number: %u", i->ref.u.number);
// 		from = i->ref.u.number;
// 		to = from;
// 		break;
// 
// 	case ATOM:
// 	{
// 		const unsigned key
// 			= uireverse(st->verbs, i->ref.u.number);
// 
// 		DBG(DBGIDX, "atom: %u -> %u; tail: %u",
// 			i->ref.u.number, key, (unsigned)TAIL);
// 
// 		if(key == TAIL)
// 		{
// 			from = -1;
// 			to = -1;
// 		}
// 		else
// 		{
// 			errmsg = "incorrect index structure";
// 		}
// 
// 		break;
// 	}

	case NUMBER:
	case ATOM:
	{
		unsigned correct = 0;

		from = reftoidx(i->ref, st->verbs, &correct);
		to = from;

		if(!correct)
		{
			errmsg = "incorrect index structure";
		}

		break;
	}
		
	case LIST:
	{
		// Должно быть два элемента. writerefs(L, R, N) запишет N
		// элементов с учётом последнего FREE (если длина списка <= len)

		const unsigned len = 2;
		Ref R[len + 1];
		writerefs(i->ref.u.list, R, len + 1);

		if(R[2].code == FREE && R[0].code <= ATOM && R[1].code <= ATOM)
		{
			unsigned correct[2] = { 0, 0 };
			from = reftoidx(R[0], st->verbs, correct + 0);
			to = reftoidx(R[1], st->verbs, correct + 1);

			if(!(correct[0] && correct[1]))
			{
				errmsg = "incorrect index structure";
			}
		}
		else
		{
			errmsg = "incorrect index structure";
		}

		break;
	}

	default:
		errmsg = "incorrect index structure";
	}

	if(!errmsg)
	{
		unsigned correct = 0;
		List *const t = forklistcut(st->L, from, to, &correct);

		if(correct)
		{
			free(st->L);
			st->L = relist(t);
			return 0;
		}
		else
		{
			errmsg = "out of bound";
		}
	}

	assert(errmsg);
	ERR(".LNth processing: %s", errmsg);
	return -1;
} 

// FIXME: (1) хотелось бы избежать дополнительного копирования списка; (2)
// хотелось бы вместо вот такого сложного ok иметь относительно простой массив.
// Ок (2) сделаем сейчас

static List *deconstructlist(List *const l, const Array *const verbs)
{
	// Формат у начального l должен быть верным. Напоминание: -1 -- самое
	// большое unsigned

	const unsigned ok

		// Список не должен быть пустым, а на первом месте должна быть
		// ссылка на узел со списокм: (.L, .LNth, .FIn)

		 = (l != NULL
			&& tip(l)->ref.code == NODE 
			&& uireverse(verbs, tip(l)->ref.u.node->verb) < TAIL)

		// На второй позиции должна быть ссылка на список с индексами.
		// Здесь проверяем, что там стоит подсписок

		&& tip(l)->next->ref.code == LIST;

	if(!ok)
	{
		ERR("%s", "wrong .LNth attributes format");
	}

	// FIXME: список копируется для упрощения алгоритма выдирания элементов
	// по индексу из списка, который после очередного шага надо освобождать

	DCState st =
	{
		.verbs = verbs,
		.L = forklist(tip(l)->ref.u.node->u.attributes)
	};

	forlist(tip(l)->next->ref.u.list, indexone, &st, 0);

	return st.L;
}

static void rewriteone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);

	// В этой функции уже известно, что l содержит ссылку на узел
	Node *const n = l->ref.u.node;

	const Array *const verbs = ptr;
	const unsigned key = uireverse(verbs, n->verb);

	DBG(DBGRONE, "%u", n->verb);

	switch(key)
	{
	case -1:
	case LNODE:
		n->u.attributes = rewritelistrefs(n->u.attributes, verbs);
		break;
	
	case LNTH:
		n->u.attributes = deconstructlist(n->u.attributes, verbs);
		break;
	
	case FIN:
		break;
	
	default:
		assert(0);
	}
}

static const char *listverbs[] =
{
	[LNODE] = "L",
	[LNTH] = "LNth",
	[FIN] = "FIn",
	[TAIL] = "T",
	NULL
};

List *evallists(
	Array *const U,
	List **const dag,
// 	const Array *const M, const Array *const dive)
	const DagMap *const M)
{
	const Array verbs = keymap(U, 0, listverbs);
//	walkdag(*dag, M, dive, rewriteone, (void *)&verbs);
	walkdag(*dag, M, rewriteone, (void *)&verbs);
	freeuimap((Array *)&verbs);

// 	const Array nonroots = keymap(U, 0, ES("L"));
// 	gcnodes(dag, M, &nonroots);
// 	freeuimap((Array *)&nonroots);

	return *dag;
}
