#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

#define DBGFREE	1
#define DBGKM	(1 << 1)
#define DBGPLU	(1 << 2)
#define DBGDM	(1 << 3)
#define DBGCK	(1 << 4)
#define DBGMO	(1 << 5)
#define DBGBL	(1 << 6)
#define DBGUL	(1 << 7)
#define DBGULC	(1 << 8)

// #define DBGFLAGS (DBGFREE)
// #define DBGFLAGS (DBGKM)
// #define DBGFLAGS (DBGPLU)
// #define DBGFLAGS (DBGDM | DBGCK | DBGPLU)
// #define DBGFLAGS (DBGCK)
// #define DBGFLAGS (DBGMO | DBGBL)

#define DBGFLAGS (DBGUL | DBGULC)

// #define DBGFLAGS 0

// Проверка компонент ключа на сравниваемость. Есть два типа ключей: базовые, в
// которых могут быть только ATOM, TYPE, NUMBER, и общие, в которых могут быть
// PTR и NODE. Эти все коды упорядочены так, что проверку на соответствие типа
// можно свести к сравнению. Базовые ключи - это Ref.code <= TYPE. Обобщённые -
// это Ref.code <= NODE. Всё это регулируется через limit, чтобы не плодить
// много разных функций. В LIST уходим рекурсивно

static unsigned iskey(const Ref, const unsigned limit);

static int iskeyitem(List *const k, void *const ptr)
{
	assert(k);
	assert(ptr);

	const unsigned limit = *(unsigned *)ptr;
	return iskey(k->ref, limit);	
}

unsigned iskey(const Ref k, const unsigned limit)
{
	switch(k.code)
	{
	case LIST:
		// !0 - это числовое значение для True. Оно, вроде, должно быть
		// равным 1. Но что-то я уже ни в чём не уверен, когда речь
		// заходит о GCC

		return forlist(k.u.list, iskeyitem, (void *)&limit, !0) == !0;
	}

	return k.code <= limit;
}

unsigned isbasickey(const Ref r)
{
	return iskey(r, ATOM);
}

unsigned issignaturekey(const Ref r)
{
	return iskey(r, TYPE);
}

unsigned istypekey(const Ref r)
{
	return iskey(r, MAP);
}

static int cmplists(const List *const, const List *const);

static int cmpkeys(const Ref k, const Ref l)
{
	if(DBGFLAGS & DBGCK)
	{
		char *const kstr = strref(NULL, NULL, k);
		char *const lstr = strref(NULL, NULL, l);
		DBG(DBGCK, "(k.code l.kode = %u %u) (k = %s) (l = %s)",
			k.code, l.code, kstr, lstr);
		
		free(lstr);
		free(kstr);
	}

	assert(k.code <= LIST);
	assert(l.code <= LIST);

	const unsigned r = cmpui(k.code, l.code);
	if(r)
	{
		return r;
	}

	if(k.code <= TYPE)
	{
		return cmpui(k.u.number, l.u.number);
	}

	if(k.code < LIST)
	{
		// Ограничения на структуру ключа с указателями для
		// самоконтроля. Важно, потому что такие ключи попадут потом в
		// окружения. Чтобы не стереть потом лишнее

		assert(k.external);
		assert(l.external);

		return cmpptr(k.u.pointer, l.u.pointer);
	}

	return cmplists(k.u.list, l.u.list);
}

// Для поиска и сортировки важно, чтобы порядок был линейным. Построить его
// можно разными способами. Например, можно считать, что списки большей длины
// всегда > списков меньшей длинны, а внутри списков длины равной порядок
// устанавливается лексикографически. Но это не удобно для пользователя, ибо,
// скорее всего, близкие по смыслу списки будут и начинаться одинаково. В целях
// отладки полезно было бы ставить рядом друг с дружкой. Поэтому, порядок будет
// просто лексикографический. Это оправданное усложнение

static int cmplists(const List *const k, const List *const l)
{
	if(k == NULL)
	{
		return 1 - (l == NULL) - ((l != NULL) << 1);
	}

	if(l == NULL)
	{
		return -1 + (k == NULL) + ((k != NULL) << 1);
	}

	// current указатели
	const List *ck = k;
	const List *cl = l;

	unsigned r;

	do
	{
		ck = ck->next;
		cl = cl->next;
		r = cmpkeys(ck->ref, cl->ref);
	} while(!r && ck != k && cl != l);

	if(r) { return r; }

	// Если (r == 0), значит какой-то список закончился. Разбираем три
	// варианта:

	return 1 - (ck == k && cl == l) - ((ck == k && cl != l) << 1);
}

static int kcmp(const void *const D, const unsigned i, const void *const key)
{
	DBG(DBGCK, "i = %u", i);
	return cmpkeys(((const Binding *)D)[i].key, *(const Ref *)key);
}

static int icmp(const void *const D, const unsigned i, const unsigned j)
{
	return kcmp(D, i, &((const Binding *)D)[j].key);
}

Array *newkeymap(void)
{
	return newmap(MAP, sizeof(Binding), icmp, kcmp);
}

unsigned iskeymap(const Ref r)
{
	return r.code == MAP && r.external
		&& r.u.array && r.u.array->code == MAP;
}

void freekeymap(Array *const env)
{
	assert(env && env->code == MAP);

	DBG(DBGFREE, "env->count: %u", env->count);

	Binding *const B = env->u.data;

	for(unsigned i = 0; i < env->count; i += 1)
	{
		freeref(B[i].key);
		freeref(B[i].ref);

		DBG(DBGFREE, "i: %u", i);
	}

	freemap(env);
}

static const char *const codestr[] =
{
	[DMAP]	= "/",
	[DTYPE]	= "@",
	[DSYM]	= "$",
	[DFORM]	= "#",
	[DIN]	= "-",
	[DOUT]	= "+",
	[DAREA]	= "*",
	[DUTIL]	= "^"
};

static const char *codetostr(const unsigned code)
{
	assert(code < sizeof(codestr)/sizeof(const char *));
	return codestr[code];
}

Ref decoatom(Array *const U, const unsigned code)
{
	switch(code)
	{
	case DMAP:
	case DTYPE:
	case DSYM:
	case DFORM:
	case DIN:
	case DOUT:
	case DAREA:
	case DUTIL:
		return readpack(U, strpack(0, codetostr(code)));

	default:
		assert(0);
	}

	return reffree();
}

Ref decorate(const Ref key, Array *const U, const unsigned code)
{
	return reflist(RL(decoatom(U, code), key));
}

unsigned decomatch(const Ref r, Array *const U, const unsigned code)
{
	const Ref d = decoatom(U, code);
	const Ref R[2];
	if(!splitpair(r, (Ref *)R))
	{
		return 0;
	}

	return R[0].code == ATOM && R[0].u.number == d.u.number;
}

static const Binding *look(const Array *const map, const Ref key)
{
	assert(map && map->code == MAP);

	const unsigned k = lookup(map, &key);

	if(k != -1)
	{
		return (const Binding *)map->u.data + k;
	}

	return NULL;
}

static unsigned allocate(Array *const map, const Ref key)
{
	assert(map && map->code == MAP);

	const Binding b =
	{
		.key = key,
		.ref = reffree()
	};

	return readin(map, &b);
}

unsigned maplookup(const Array *const map, const Ref key)
{
	if(map)
	{
		return lookup(map, &key);
	}

	return -1;
}

unsigned mapreadin(Array *const map, const Ref key)
{
	assert(map && map->code == MAP);

	if(lookup(map, &key) == -1)
	{
		return allocate(map, key);
	}

	return -1;
}

typedef struct
{
	Array *const U;
	List *L;
	Array *current;
	const Ref path;
	MayPass *const maypass;
	NewTarget *const newtarget;
	NextPoint *const nextpoint;
	const Ref M;
} PState;

unsigned bindkey(Array *const map, const Ref key)
{
	unsigned id = maplookup(map, key);
	if(id != -1)
	{
		return id;
	}

	// Здесь необходимо выделить новый Binding. И сохранить в нём копию
	// ключа. FIXME: копирование - накладные расходы, но это плата за
	// текущий вариант управления память. В следующей версии в этом не будет
	// необходимости

	assert(map);
	id = mapreadin(map, forkref(key, NULL));
	assert(id != -1);
	return id;
}

const Binding *bindingat(const Array *const map, const unsigned N)
{
	if(!map || N == -1)
	{
		return NULL;
	}

	assert(N < map->count);
	return (const Binding *)map->u.data + N;
}
static int makeone(List *const n, void *const ptr)
{
	assert(n);
	assert(ptr);
	PState *const st = ptr;

	assert(st->current);

	// Можем ли мы пройти через этот элемент пути? Структуры, необходимые
	// для этого могут быть уничтожены. Поэтому проверяем

	if(!st->maypass(st->U, st->current))
	{
		return !0;
	}

	// Привязываться будем к некоторой под-структуре текущей области,
	// поэтому

	Array *const curr = st->nextpoint(st->U, st->current);
	assert(curr);

	if(st->M.code == FREE || n != st->L)
	{
		// Нам не передано отображение, с которым надо устроить связь
		// или мы находимся не в конце списка имён. В обоих случаях надо
		// отыскать отображение с подходящим именем или создать новое 

		st->current = linkmap(st->U, curr, st->path, n->ref, reffree());
		if(st->current)
		{
			return 0;
		}

		const Ref m = refkeymap(st->newtarget(st->U, st->current));
		st->current = linkmap(st->U, curr, st->path, n->ref, m);
		assert(st->current == m.u.array);

		return 0;
	}

	// Привязываем указанное отображение к концу цепочки

	assert(st->M.code != FREE && n == st->L);

	st->current = linkmap(st->U, curr, st->path, n->ref, st->M);
	if(st->current)
	{
		assert(st->current == st->M.u.array);
		return 0;
	}

	return !0;
}

Array *makepath(
	Array *const map,
	Array *const U,
	const Ref path,
	const List *const names, const Ref M,
	MayPass maypass, NewTarget newtarget, NextPoint nextpoint)
{
	assert(map && map->code == MAP);
	assert(maypass && newtarget && nextpoint);
	assert(M.code == FREE || iskeymap(M));

	PState st =
	{
		.U = U,
		.L = (List *)names,
		.maypass = maypass,
		.newtarget = newtarget,
		.nextpoint = nextpoint,
		.M = M,
		.current = map,
		.path = path,
	};

	return forlist((List *)names, makeone, &st, 0) ? NULL : st.current;
}

static List *tracekey(List *const acc, const Binding *const b, const Ref key)
{
	if(b == NULL)
	{
		return acc;
	}

	const Ref M = b->ref;
	assert(iskeymap(M));

	return tracekey(append(acc, RL(markext(M))), look(M.u.array, key), key);
}

List *tracepath(
	const Array *const map, Array *const U, const Ref path, const Ref name)
{
	assert(map && map->code == MAP);

	// markext для того, чтобы при удалении ключей не удалить path и name
	// на стороне пользователя. look ключи не копирует, поэтому не
	// оптимизация

	const Ref pathkey
		= decorate(reflist(RL(markext(path), markext(name))), U, MAP);

	const Ref thiskey
		= decorate(
			reflist(RL(
				markext(path),
				readpack(U, strpack(0, "this")))),
			U, MAP);
	
	List *const l = tracekey(NULL, look(map, thiskey), pathkey);

	freeref(thiskey);
	freeref(pathkey);

	return l;
}

typedef struct
{
	const Binding *b;
	const Ref key;
	unsigned depth;	
} PLState;

static int lookone(List *const e, void *const ptr)
{
	assert(e && iskeymap(e->ref));
	assert(ptr);
	PLState *const st = ptr;

	if(DBGFLAGS & DBGPLU)
	{
		dumpkeymap(1, stdout, 1, NULL, e->ref.u.array, NULL);
	}

	const Binding *const b = look(e->ref.u.array, st->key);

	DBG(DBGPLU, "%p", (void *)b);

	if(b)
	{
		st->b = b;
		return 1;
	}

	// Если ничего не найдено, переходим к следующему элементу
	st->depth += 1;
	return 0;
}

const Binding *pathlookup(
	const List *const stack, const Ref key, unsigned *const depth)
{
	DBG(DBGPLU,
		"PLU. (key.code key.number) = (%u %u)", key.code, key.u.number);

	PLState st = { .b = NULL, .key = key, .depth = 0 };

	if(forlist((List *)stack, lookone, &st, 0))
	{
		assert(st.b);

		if(depth)
		{
			*depth = st.depth;
		}

		return st.b;
	}

	return NULL;
}

static Binding *maptofree(Array *const map, const Ref key)
{
	Binding *const b = (Binding *)bindingat(map, mapreadin(map, key));
	assert(b);
	return b;
}

void tunerefmap(Array *const map, const Ref key, const Ref val)
{
	Binding *const b = maptofree(map, markext(key));
	b->ref = markext(val);
}

Ref refmap(const Array *const map, const Ref key)
{
	const Binding *const b = bindingat(map, maplookup(map, markext(key)));
	if(b)
	{
		return b->ref;
	}

	return reffree();
}

void tunesetmap(Array *const map, const Ref key)
{
	tunerefmap(map, key, refnum(0));
}

unsigned setmap(const Array *const map, const Ref key)
{
	const Ref r = refmap(map, key);

	switch(r.code)
	{
	case FREE:
		return 0;

	case NUMBER:
		assert(r.u.number == 0);
		return 1;
	
	default:
		assert(0);
	}

	return 0;
}

void tuneptrmap(Array *const map, const Ref key, void *const ptr)
{
	tunerefmap(map, key, refptr(ptr));
}

void *ptrmap(const Array *const map, const Ref key)
{
	const Ref r = refmap(map, key);

	switch(r.code)
	{
	case FREE:
		return NULL;

	case PTR:
		return r.u.pointer;	
	
	default:
		assert(0);
	}

	return NULL;
}

void tuneenvmap(Array *const map, const Ref key, Array *const km)
{
	assert(km && km->code == MAP);
	tunerefmap(map, key, refkeymap(km));
}

Array *envmap(const Array *const map, const Ref key)
{
	const Ref r = refmap(map, key);

	switch(r.code)
	{
	case FREE:
		return NULL;
	
	case MAP:
		return r.u.array;
	
	default:
		assert(0);
	}

	return NULL;
}

Array *newverbmap(
	Array *const U, const unsigned hint, const char *const atoms[])
{
	assert(atoms);
	assert(hint <= MAXHINT);

	Array *const map = newkeymap();

	for(unsigned i = 0; atoms[i]; i += 1)
	{
		tunerefmap(map,
			readpack(U, strpack(hint, atoms[i])), refnum(i));
	}

	return map;
}

unsigned verbmap(const Array *const map, const unsigned verb)
{
	const Ref r = refmap(map, refatom(verb));

	switch(r.code)
	{
	case FREE:
		return -1;

	case NUMBER:
		return r.u.number;

	default:
		assert(0);
	}

	return -1;
}

unsigned enummap(Array *const map, const Ref key)
{
	assert(map);
	const Binding *b = bindingat(map, maplookup(map, markext(key)));
	if(!b)
	{
		b = bindingat(map, mapreadin(map, markext(key)));
		assert(b);
	}

	const unsigned n = b - (Binding *)map->u.data;
	assert(n < map->count);

	return n;
}

unsigned typeenummap(Array *const map, const Ref key)
{
	assert(map);
	assert(istypekey(key));

	const Binding *const b = bindingat(map, bindkey(map, key));

	const unsigned n = b - (Binding *)map->u.data;
	assert(n < map->count);

	return n;
}

typedef struct
{
	const Ref **const uni;
	const unsigned N;
	unsigned n;
} KMState;

static unsigned listmatch(
	KMState *const st, const List *const k, const List *const l);

static unsigned keymatchone(KMState *const st, const Ref k, const Ref *const l)
{
	assert(st);
	assert(st->n <= st->N);

	DBG(DBGKM, "(k.code l.code) = (%u %u)", k.code, l->code);

	if(k.code == FREE)
	{
		DBG(DBGKM, "match = %u", !0);

		if(st->n < st->N)
		{
			assert(st->uni);

			st->uni[st->n] = l;
			st->n += 1;
		}

		return !0;
	}

	if(k.code != l->code)
	{
		DBG(DBGKM, "match = %u", 0);
		return 0;
	}

	unsigned match = 0;

	switch(k.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		match = k.u.number == l->u.number;
		break;
	
	case PTR:
		match = k.u.pointer == l->u.pointer;
		break;
	
	case LIST:
	case NODE:
	case FORM:
		match = listmatch(st, k.u.list, l->u.list);
		break;
	
	default:
		assert(0);
	}

	DBG(DBGKM, "match = %u", match);

	return match;
}

static unsigned listmatch(
	KMState *const st, const List *const k, const List *const l)
{
	if(k == NULL || l == NULL)
	{
		return k == NULL && l == NULL;
	}

	const List *ck = k;
	const List *cl = l;

	unsigned r;

	do
	{
		ck = ck->next;
		cl = cl->next;
		r = keymatchone(st, ck->ref, &cl->ref);
	} while(r && ck != k && cl != l);

	// Всё должно совпадать

	return r && ck == k && cl == l;
}

unsigned keymatch(
	const Ref k, const Ref *const l, const Ref *uni[], const unsigned N,
	unsigned *const pmatched)
{
	assert(!N || uni);

	KMState st =
	{
		.n = 0,
		.N = N,
		.uni = uni
	};

	const unsigned r = keymatchone(&st, k, l);

	if(pmatched)
	{
		*pmatched = st.n;
	}

	return r;
}

static void walkbindingscore(
	Array *const U,
	Array *const map, const Array *const escape,
	WalkBinding wlk, void *const ptr, Array *const V)
{
	assert(map && map->code == MAP);

	// Отмечаемся, что были здесь
	tunesetmap(V, refkeymap(map));
	
	const Binding *const B = map->u.data;
	const unsigned *const I = map->index;
	
	for(unsigned i = 0; i < map->count; i += 1)
	{
		const Binding *const b = B + I[i];

		const unsigned go
			= wlk(b, ptr)
				&& iskeymap(b->ref)
				&& decomatch(b->ref, U, DMAP)
				&& !setmap(escape, b->key)
				&& !setmap(V, b->ref);

		if(go)
		{
			walkbindingscore(U,
				b->ref.u.array, escape, wlk, ptr, V);
		}
	}
}

void walkbindings(
	Array *const U,
	Array *const map, const Array *const escape,
	WalkBinding wlk, void *const ptr)
{
	Array *const visited = newkeymap();
	walkbindingscore(U, map, escape, wlk, ptr, visited);
	freekeymap(visited);
}

static unsigned linkup(Array *const U, Array *const map)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "LNKCNT")));
	Binding *const b = (Binding *)bindingat(map, bindkey(map, key));
	assert(b);

	if(b->ref.code == NUMBER)
	{
		assert(b->ref.u.number < MAXNUM);
		b->ref.u.number += 1;
	}
	else
	{
		// Это должна быть свободная ячейка
		assert(b->ref.code == FREE);

		// Сразу записываем единицу, считая, что всё началось с 0
		b->ref = refnum(1);
	}

	return b->ref.u.number;
}

static unsigned linkdown(Array *const U, Array *const map)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "LNKCNT")));
	const Binding *const b = bindingat(map, maplookup(map, key));

	// Корректно вызывать linkdown можно только после linkup, поэтому должно
	// выполняться
	assert(b && b->ref.code == NUMBER && b->ref.u.number > 0);
	return (((Binding *)b)->ref.u.number -= 1);
}

static unsigned nlinks(Array *const U, const Array *const map)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "LNKCNT")));
	const Binding *const b = bindingat(map, maplookup(map, key));
	if(b)
	{
		assert(b->ref.code == NUMBER);
		return b->ref.u.number;
	}

	return 0;
}

void markactive(Array *const U, Array *const map, const unsigned flag)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "ACTIVE")));
	Binding *const b = (Binding *)bindingat(map, bindkey(map, key));
	assert(b);

	if(flag)
	{
		// Мы можем установить флаг активности только в том случае, если
		// его вообще не было. Иначе это повторная попытка или попытка
		// оживить неактивную сущность. Пока семантика такого поведения
		// не понятна, не допускаем её

		assert(b->ref.code == FREE);
		b->ref = refnum(!0);
		return;
	}

	// Сбросить флаг активности можно только с активной сущности
	assert(b->ref.code == NUMBER && b->ref.u.number != 0);
	b->ref = refnum(0);
}

unsigned isactive(Array *const U, const Array *const map)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "ACTIVE")));
	const Binding *const b = bindingat(map, maplookup(map, key));
	assert(!b || b->ref.code == NUMBER);

	return b != NULL && b->ref.u.number != 0;
}

Array *linkmap(
	Array *const U,
	Array *const map,
	const Ref path, const Ref name, const Ref target)
{
	assert(map && map->code == MAP);

	DL(pair, RS(path, name));
	DL(key, RS(decoatom(U, DMAP), pair));

	if(target.code == FREE)
	{
		// Нас просят найти подходящую связь
		const Binding *const b
			= bindingat(map, maplookup(map, key));

		if(b)
		{
			// Нечто нашлось и оно должно быть keymap-ой
			assert(iskeymap(b->ref));
			return b->ref.u.array;
		}

		// target.code == FREE, поэтому больше сделать ничего не можем
		return NULL;
	}

	assert(iskeymap(target));

	const unsigned bid = bindkey(map, key);
	const Binding *const b = bindingat(map, bid);

	if(b->ref.code != FREE)
	{
		// Нечто нашлось, но нас просят зацепить map и target, поэтому
		return NULL;
	}

	// Цепляем
	linkup(U, target.u.array);
	((Binding *)bindingat(map, bid))->ref = target;

	return target.u.array;
}

static void modelunlink(
	Array *const U, Array *const model, const Array *const map)
{
	Binding *const b
		= (Binding *)bindingat(model,
			bindkey(model, refkeymap((Array *)map)));
	assert(b);
	
	// Мы моделируем операцию удаления ссылки, поэтому тут мы должны её
	// выполнить на модели. Были мы в этой области ли не были, тут не важно.
	// Мы не должны в ней побывать больше, чем LNKCNT раз. А это значение
	// должно быть в модели. Модель может видеть эту область первый раз,
	// поэтому нужна инициализации

	if(b->ref.code == FREE)
	{
		b->ref = refnum(nlinks(U, map));
	}

	// Теперь надо вычеркнуть одну связь из общего их количества

	assert(b->ref.code == NUMBER && b->ref.u.number > 0);
	if((b->ref.u.number -= 1) == 0)
	{
		// Если счётчик упал до 0, то эта область должна быть удалена и
		// все link-и ведущие из неё должны быть так же вычеркнуты

		const Binding *const B = (Binding *)map->u.data;
		for(unsigned i = 0; i < map->count; i += 1)
		{
			if(iskeymap(B[i].ref) && decomatch(B[i].key, U, DMAP))
			{
				// Если видим связь, нужно моделировать её
				// удаление

				modelunlink(U, model, B[i].ref.u.array);
			}
		}
	}
}

static unsigned checkunlinkmodel(Array *const U, const Array *const model)
{
	for(unsigned i = 0; i < model->count; i += 1)
	{
		const Binding *const b = bindingat(model, i);
		assert(iskeymap(b->key) && b->ref.code == NUMBER);

		if(b->ref.u.number == 0 && isactive(U, b->key.u.array))
		{
			return 0;
		}
	}

	return !0;
}

static void unlinkcore(Array *const U, Binding *const B)
{
	// FIXME: вообще-то вместо assert можно написать if, чтобы не
	// повторяться. Но пока так

	assert(B && iskeymap(B->ref) && decomatch(B->key, U, DMAP));
	Array *const map = B->ref.u.array;

	// Теперь ref можно безусловно убрать
	B->ref = reffree();

	// Нужно выкинуть одну связь. Корректность проверяется в linkdown.
	// linkdown при этом не меняет набор binding-ов в окружении, поэтому
	// можно не пересчитывать B

	if(linkdown(U, map))
	{
		// Ссылка была не последней, можно не продолжать
		return;
	}

	// Надо отцепить все ссылки в этой области видимости

	for(unsigned i = 0; i < map->count; i += 1)
	{
		Binding *const b = (Binding *)bindingat(map, i);
		assert(b);
		if(iskeymap(b->ref) && decomatch(b->key, U, DMAP))
		{
			unlinkcore(U, b);
		}
	}

	// Саму область теперь можно освободить

	assert(!isactive(U, map) && !nlinks(U, map));
// 	B->ref = reffree();
	freekeymap(map);
}

static unsigned aftercheckunlink(Array *const U, Array *const model)
{
	for(unsigned i = 0; i < model->count; i += 1)
	{
		const Binding *const b = bindingat(model, i);
		assert(b && iskeymap(b->key) && b->ref.code == NUMBER);

		if(b->ref.u.number)
		{
			// Если число ссылок в модели не равно 0, то область ещё
			// жива и можно проверить соответствие счётчиков

			if(b->ref.u.number != nlinks(U, b->key.u.array))
			{
				return 0;
			}
		}
	}

	return !0;
}

unsigned unlinkmap(
	Array *const U, Array *const map, const Ref path, const Ref name)
{
	DL(pair, RS(path, name));
	DL(key, RS(decoatom(U, DMAP), pair));

	Binding *const b = (Binding *)bindingat(map, maplookup(map, key));
	if(!b)
	{
		return 0;
	}
	assert(iskeymap(b->ref));

	if(DBGFLAGS & DBGUL)
	{
		char *const skey = strref(U, NULL, b->key);
		DBG(DBGUL, "unlinking from %p key %s map %p",
			(void *)map, skey, (void *)b->ref.u.array);
		free(skey);
	}

	Array *const model = newkeymap();

	modelunlink(U, model, b->ref.u.array);
	if(!checkunlinkmodel(U, model))
	{
		return 0;
	}

	DBG(DBGUL, "%s", "model clean. Going to core");

	unlinkcore(U, b);
	assert(aftercheckunlink(U, model));

	freekeymap(model);

	return !0;
}

Array *stdupstreams(Array *const U)
{
	Array *const map = newkeymap();

	DL(pair, RS(readtoken(U, "ENV"), readtoken(U, "parent")));
	DL(key, RS(decoatom(U, DMAP), pair));

	Binding *const b = (Binding *)bindingat(map, bindkey(map, key));
	assert(b);
	b->ref = refnum(0);

	return map;
}
