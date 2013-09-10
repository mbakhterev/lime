#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGRONE	1
#define DBGIDX	2

// #define DBGFLAGS (DBGRONE)
// #define DBGFLAGS (DBGIDX)

#define DBGFLAGS 0

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
		case FIN:
		{
			const List *const attr = n->u.attributes;

			assert(attr
				&& attr->ref.code == LIST
				&& attr->ref.external);

			freelist(l);
			r = forklist(attr->ref.u.list);

			break;
		}

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
		const char *const c = strlist(NULL, st->L);
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

// Извлечение списка из LNODE, LNTH или FIN, в зависимости от типа узла

static const List *extractlist(const List *const attr, const unsigned verb)
{
	switch(verb)
	{
	case LNODE:
	case LNTH:
		return attr;
	
	case FIN:
		assert(attr
			&& attr->ref.code == LIST && attr->ref.external);
		
		return attr->ref.u.list;

	default:
		assert(0);
	}

	return NULL;
}

// FIXME: (1) хотелось бы избежать дополнительного копирования списка; (2)
// хотелось бы вместо вот такого сложного ok иметь относительно простой массив.
// Ок (2) сделаем сейчас

static List *deconstructlist(List *const l, const Array *const verbs)
{
	const unsigned len = 2;
	Ref R[len + 1];
	const unsigned t = writerefs(l, R, len + 1);
	
	unsigned verb = -1;

	const unsigned ok
		 = (t == len + 1 && R[len].code == FREE)
		&& (R[0].code == NODE && R[0].u.node
			&& (verb = uireverse(verbs, R[0].u.node->verb)) < TAIL)
		&& (R[1].code == LIST);

	if(!ok)
	{
		ERR("%s", "wrong .LNth attributes format");
	}

	// Извлекаем исходный список.

	// FIXME: список копируется для упрощения алгоритма выдирания элементов
	// по индексу из списка, который после очередного шага надо освобождать

	DCState st =
	{
		.verbs = verbs,
//		.L = forklist(R[0].u.node->u.attributes)
		.L = forklist(extractlist(R[0].u.node->u.attributes, verb))
	};

	forlist(R[1].u.list, indexone, &st, 0);

	return st.L;
}

// Argument state

typedef struct
{
	const Array *const verbs;
	List *const L;
} AState;

static void rewriteone(List *const l, void *const ptr)
{
	assert(ptr);
	assert(l && l->ref.code == NODE);

	const AState *const st = ptr;

	// В этой функции уже известно, что l содержит ссылку на узел
	Node *const n = l->ref.u.node;

	const Array *const verbs = st->verbs;
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
		if(n->u.attributes)
		{
			ERR("%s", ".FIn node argument list should be NULL");
		}

// 		// FIXME: мягко говоря, не самое эффективное решение. Вообще,
// 		// можно по всему алгоритму при встрече с .FIn смотреть на
// 		// st->L, но это дорогое исправление. Пока просто копируем
// 		// список. Это важно при сборке мусора. st->L нельзя удалять,
// 		// это список извне текущего кусочка графа
// 
// 		n->u.attributes = forklist(st->L);

		// Создаём в атрибутах .FIn список, содержащий
		// external-подсписок st->L. Это должно защитить последний от
		// очистки при сборке мусора:

		n->u.attributes = RL(markext(reflist(st->L)));
		break;
	
	default:
		assert(0);
	}
}

static const char *const listverbs[] =
{
	[LNODE] = "L",
	[LNTH] = "LNth",
	[FIN] = "FIn",
	[TAIL] = "T",
	NULL
};

List *evallists(
	Array *const U,
	List **const dag, const Array *const map, const Array *const go,
	const List *const arguments)
{
	const Array verbs = keymap(U, 0, listverbs);

	const AState st =
	{
		.verbs = &verbs,
		.L = (List *)arguments
	};

	walkdag(*dag, map, go, rewriteone, (void *)&st);
	freeuimap((Array *)&verbs);

	return *dag;
}
