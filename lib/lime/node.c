#include "construct.h"

#include <assert.h>

enum { NODELEN = 3, VERB = 0, LINE = 2, ATTR = 1 };

static unsigned splitnode(const List *const l, const Ref *parts[])
{
	unsigned nmatch = 0;
	const Ref F = reffree();
	const Ref n = reflist((List *)l);

	DL(pattern, RS(F, F, F));

	if(keymatch(pattern, &n, parts, NODELEN, &nmatch))
	{
		assert(nmatch == NODELEN);
		return !0;
	}

	return 0;
}

static unsigned verbandline(const Ref *const n[])
{
	return n[VERB]->code == ATOM && n[LINE]->code == NUMBER;
}

unsigned isnodelist(const List *const l)
{
	const Ref *R[NODELEN];
	return splitnode(l, R) && verbandline(R);
}

unsigned isnode(const Ref node)
{
	return node.code == NODE && isnodelist(node.u.list);
}

unsigned nodeverb(const Ref n, Array *const map)
{
	assert(n.code == NODE);

	const Ref *R[NODELEN];
	splitnode(n.u.list, R);

	assert(verbandline(R));

	return map ? verbmap(map, R[VERB]->u.number) : R[VERB]->u.number;
}

const Ref *nodeattributecell(const Ref n)
{
	assert(n.code == NODE);
	const Ref *R[NODELEN];
	splitnode(n.u.list, R);

	assert(verbandline(R));

	return (Ref *)R[ATTR];
}

Ref nodeattribute(const Ref n)
{
	return *nodeattributecell(n);
}

Ref newnode(const unsigned verb, const Ref attribute, const unsigned line)
{
	return cleanext(refnode(RL(refatom(verb), attribute, refnum(line))));
}

unsigned nodeline(const Ref n)
{
	assert(n.code == NODE);
	const Ref *R[NODELEN];
	splitnode(n.u.list, R);

	assert(verbandline(R));

	return R[LINE]->u.number;
}

Ref forknode(const Ref node, Array *const M)
{
	assert(isnode(node));

	// Есть два варианта по M. Когда M != NULL нас просят от-fork-ать узлы и
	// в новом корректно расставить на них ссылки. Если M == NULL, то нас
	// простят скопировать ссылки

	if(!M)
	{
		// Ссылка должна быть ссылкой

		assert(node.external);
		return node;
	}

	// Сложный вариант. Может быть два варианта по Ref.external.
	// Если Ref.external, то речь идёт о ссылке и надо копировать её образ в
	// M

	if(node.external)
	{
		// Смотрим на то, куда отображается эта ссылка. Надо
		// быть уверенными, что отображение знает о node. Иначе
		// ошибка. Это всё проверит isnode

		const Ref n = refmap(M, node);
		assert(isnode(n));

		return markext(n);
	}

	// Наконец, определение узла. Сначала создаём копию
	// выражения. Делаем это рекурсивно. На n атрибуты ссылаться не могут

	const Ref n
		= newnode(
			nodeverb(node, NULL),
			forkref(nodeattribute(node), M),
			nodeline(node));
	
	// Процедура tunerefmap сама за-assert-ит попытку
	// сделать не-уникальное отображение

	tunerefmap(M, node, n);

	// Процедура newnode вернёт верную Ref-у. Можно её вернуть выше
	return n;
}

unsigned knownverb(const Ref n, Array *const map)
{
	return map != NULL && nodeverb(n, map) != -1;
}


