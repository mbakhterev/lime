#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

#define DBGFREE	1
#define DBGKM 2
#define DBGPLU 4
#define DBGDM 8

// #define DBGFLAGS (DBGFREE)
// #define DBGFLAGS (DBGKM)
// #define DBGFLAGS (DBGPLU)

#define DBGFLAGS (DBGDM)

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

//	if(k.code <= MAP)
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
	return r.code == MAP && r.u.array && r.u.array->code == MAP;
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

static const char *codetostr(const unsigned code)
{
	switch(code)
	{
	case DMAP:
		return "/";
	
	case DTYPE:
		return "@";
	
	case DSYM:
		return "$";
	
	case DFORM:
		return "#";
	
	case DIN:
		return "-";
	
	case DOUT:
		return "+";
	
	case DREACTOR:
		return "?";
	}

	assert(0);
	return NULL;
}

Ref decoatom(Array *const U, const unsigned code)
{
	switch(code)
	{
	case DMAP:
	case DTYPE:
	case DSYM:
	case DIN:
	case DOUT:
	case DREACTOR:
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

static Binding *allocate(Array *const map, const Ref key)
{
	assert(map && map->code == MAP);

	const Binding b =
	{
//		.key = forkref(key, NULL),
		.key = key,
		.ref = { .code = FREE, .u.pointer = NULL }
	};

	const unsigned k = readin(map, &b);

	return (Binding *)map->u.data + k;
}

const Binding *maplookup(const Array *const map, const Ref key)
{
	if(map)
	{
		return look(map, key);
	}

	return NULL;
}

Binding *mapreadin(Array *const map, const Ref key)
{
	if(!look(map, key))
	{
		return allocate(map, key);
	}

	return NULL;
}

typedef struct
{
	Array *const U;
	List *L;
	Array *current;
	const Ref path;
	const Ref M;
	unsigned ok;
} PState;

Binding *bindkey(Array *const map, const Ref key)
{
	Binding *b = (Binding *)maplookup(map, key);
	if(b)
	{
		// Binding нашлась, ничего не нужно делать кроме как вернуть
		// ссылку

		return b;
	}

	// Здесь необходимо выделить новый Binding. И сохранить в нём копию
	// ключа. FIXME: копирование - накладные расходы, но это плата за
	// текущий вариант управления память. В следующей версии в этом не будет
	// необходимости

	assert(map);
	b = mapreadin(map, forkref(key, NULL));
	assert(b);

	return b;
}

static int makeone(List *const k, void *const ptr)
{
	PState *const st = ptr;
	assert(st);
	assert(k);

	// Поиск в текущем окружении по ключу ("/" (path name)). path и ref
	// пропускаются через forkref, чтобы ключ можно было удалить без
	// повреждения оригинала. FIXME: не самая оптимальная логика. В
	// следующей версии это не понадобится

	const Ref key
		= decorate(
			reflist(RL(
				forkref(st->path, NULL),
				forkref(k->ref, NULL))),
			st->U, 
			MAP);

	Binding *const b = bindkey(st->current, key);

	// В случае, когда key копируется в новый Binding отображения, bindkey
	// выставит key.external, и freekey ничего с этим key не будет делать

	freeref(key);

	if(b->ref.code != FREE)
	{
		// Найденное должно быть keymap-ой
		assert(iskeymap(b->ref));

		// В любом случае вернуть надо ссылку на эту keymap-у
		st->current = b->ref.u.array;

		// Но если k - это последнее имя в списке, то нужно убедится,
		// что (M.code == FREE)

		st->ok = k != st->L || st->M.code == FREE;

		return 0;
	}

	// Если ничего не нашлось, то в зависимости от k и st->L занимаемся
	// инициализацией

	if(k != st->L)
	{
		// Речь идёт не о последнем имени, поэтому нужно создать пустую
		// новую keymap. В ней и будем работать на следующей итерации

		b->ref = refkeymap(st->current = newkeymap());
		return 0;
	}

	if(st->M.code == FREE)
	{
		// Речь идёт о последнем имени, но ссылка на таблицу не
		// предоставлена. Поэтому создаём свежую. Всё будет в порядке,
		// если инициализация корректная

		b->ref = refkeymap(st->current = newkeymap());
		st->ok = iskeymap(b->ref);

		return 0;
	}

	// Ничего не нашлось, имя последнее и передан указатель на таблицу для
	// инициализации. Поэтому надо записать st->M в созданную ячейку b.
	// Подготовится к возврату этой таблицы и проверить, что всё ОК. То, что
	// st->M корректная ссылка проверено в самой makepath

	b->ref = st->M;
	st->current = st->M.u.array;

	st->ok = st->M.code == MAP;

	return 0;
}

Array *makepath(
	Array *const map,
	Array *const U, const Ref path, const List *const names,
	const Ref M)
{
	assert(map && map->code == MAP);
	assert(M.code == FREE || iskeymap(M));

	PState st =
	{
		.U = U,
		.L = (List *)names,
		.M = M,
		.current = map,
		.path = path,
		.ok = 0
	};

	forlist((List *)names, makeone, &st, 0);

	if(st.ok)
	{
		return st.current;
	}

	return NULL;
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

	// dynamark для того, чтобы при удалении ключей не удалить path и name
	// на стороне пользователя. look ключи не копирует, поэтому не
	// оптимизация

	const Ref pathkey
		= decorate(reflist(RL(dynamark(path), dynamark(name))), U, MAP);

	const Ref thiskey
		= decorate(
			reflist(RL(
				dynamark(path),
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
		dumpkeymap(1, stdout, 1, NULL, e->ref.u.array);
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
// 	Binding *const b = keymap(map, key);
// 	assert(b->ref.code == FREE);

	Binding *const b = mapreadin(map, key);
	assert(b);
	return b;
}

void tunerefmap(Array *const map, const Ref key, const Ref val)
{
	Binding *const b = maptofree(map, dynamark(key));
	b->ref = dynamark(val);
}

Ref refmap(const Array *const map, const Ref key)
{
	const Binding *const b = maplookup(map, dynamark(key));
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

// unsigned enummap(Array *const map, const Ref key)
// {
// 	assert(map);
// 	const unsigned n = map->count;
// 
// 	const Ref r = refmap(map, key);
// 	switch(r.code)
// 	{
// 	case NUMBER:
// 		return r.u.number;
// 
// 	case FREE:
// 	{
// 		tunerefmap(map, key, refnum(n));
// 		assert(n + 1 == map->count);
// 		return n;
// 	}
// 
// 	default:
// 		assert(0);
// 	}
// 
// 	return -1;
// }

unsigned enummap(Array *const map, const Ref key)
{
	assert(map);
	const Binding *b = maplookup(map, dynamark(key));
	if(!b)
	{
		b = mapreadin(map, dynamark(key));
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

// 	const Binding *const b = keymap(map, key);
	const Binding *const b = bindkey(map, key);

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
// 	case NODE:
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

void walkbindings(Array *const map, WalkBinding wlk, void *const ptr)
{
	assert(map && map->code == MAP);
	
	Binding *const B = map->u.data;
	const unsigned *const I = map->index;
	
	for(unsigned i = 0; i < map->count; i += 1)
	{
		Binding *const b = B + I[i];

		if(wlk(b, ptr) && iskeymap(b->ref) && !b->ref.external)
		{
			walkbindings(b->ref.u.array, wlk, ptr);
		}
	}
}
