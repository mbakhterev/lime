#include "util.h"
#include "construct.h"

#include <assert.h>

#define DBGVAL 1

// #define DBGFLAGS DBGVAL

#define DBGFLAGS 0

unsigned splitnode(const Ref N, const Ref *parts[])
{
	if(N.code != NODE)
	{
		return 0;
	}

	return splitlist(N.u.list, parts, NODELEN);
}

static unsigned verbandline(const Ref *const n[])
{
	DBG(DBGVAL, "%u %u %u %u",
		n[0]->code, n[1]->code, n[2]->code, n[3]->code);

	return n[VERB]->code == ATOM
		&& n[LINE]->code == NUMBER && n[FILEATOM]->code == ATOM;
}

unsigned isnodelist(const List *const l)
{
	const Ref *R[NODELEN];
	return splitlist(l, R, NODELEN) && verbandline(R);
}

unsigned isnode(const Ref node)
{
	return node.code == NODE && isnodelist(node.u.list);
}

unsigned issinglenode(const List *const l)
{
	return l->next == l && !l->ref.external && isnode(l->ref);
}

unsigned nodeverb(const Ref n, const Array *const map)
{
	const Ref *R[NODELEN];

	assert(splitnode(n, R) && verbandline(R));

	return map ? verbmap(map, R[VERB]->u.number) : R[VERB]->u.number;
}

const Ref *nodeattributecell(const Ref n)
{
	const Ref *R[NODELEN];

	assert(splitnode(n, R) && verbandline(R));

	return (Ref *)R[ATTR];
}

Ref nodeattribute(const Ref n)
{
	return *nodeattributecell(n);
}

unsigned nodefileatom(const Ref N)
{
	const Ref *R[NODELEN];

	assert(splitnode(N, R) && verbandline(R));

	return R[FILEATOM]->u.number;
}

const char *const nodefilename(const Array *const U, const Ref N)
{
	return (const char *)atombytes(atomat(U, nodefileatom(N)));
}

Ref newnode(
	const unsigned verb, const Ref attribute,
	const unsigned fileatom, const unsigned line)
{
	return cleanext(refnode(RL(refatom(verb), attribute, 
		refatom(fileatom), refnum(line))));
}

// void freenode(const Ref n)
// {
// 	assert(isnode(n));
// 	
// 	if(!n.external)
// 	{
// // FIXME: Полное освобождение узлов отключено, но список атрибутов имеет смысл
// // чистить
// // 		freelist(n.u.list);
// 
// 		freeref(nodeattribute(n));
// 	}
// }

void freenode(const Ref n)
{
	assert(n.code == NODE);

	if(n.external)
	{
		// Сделать больше ничего не можем. Нельзя проверять при помощи
		// isnode, потому что ссылка может указывать на уже
		// освобождённый узел

		return;
	}

	// Если это определение узла, то убеждаемся в корректности о освобождаем
	// всё

	assert(isnode(n));
	freelist(n.u.list);
}

unsigned nodeline(const Ref n)
{
// 	assert(n.code == NODE);
	const Ref *R[NODELEN];
// 	splitnode(n.u.list, R);

	assert(splitnode(n, R) && verbandline(R));

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
		// Смотрим на то, куда отображается эта ссылка. Отображение
		// может знать о node, что говорит о том, что был скопирован
		// узел, на который ведёт оригинальная ссылка. Если так, то мы
		// должны вернуть ссылку на копию для сохранения согласовенности

		const Ref n = refmap(M, node);
		if(isnode(n))
		{
			return markext(n);
		}
		
		// Если отображение не знает об узле, соответствующем node,
		// значит, надо проверить, что оно ничего не знает о нём, что
		// ссылка действительно внешняя и вернуть её саму

		assert(n.code == FREE);
		return markext(node);
	}

	// Наконец, определение узла. Сначала создаём копию
	// выражения. Делаем это рекурсивно. На n атрибуты ссылаться не могут

	const Ref n
		= newnode(
			nodeverb(node, NULL),
			forkref(nodeattribute(node), M),
			nodefileatom(node), nodeline(node));
	
	// Процедура tunerefmap сама за-assert-ит попытку
	// сделать не-уникальное отображение

	tunerefmap(M, node, n);

	// Процедура newnode вернёт верную Ref-у. Можно её вернуть выше
	return n;
}

unsigned knownverb(const Ref n, const Array *const map)
{
	return map != NULL && nodeverb(n, map) != -1;
}

const unsigned char *nodename(const Array *const U, const Ref N)
{
	return atombytes(atomat(U, nodeverb(N, NULL)));
}
