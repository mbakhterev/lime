#include "construct.h"

#include <assert.h>

Ref forknode(const Ref node, Array *const M)
{
	assert(node.code == NODE && isnode(node));

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
