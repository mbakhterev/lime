#include "util.h"
#include "construct.h"

#include <setjmp.h>
#include <stdlib.h>
#include <assert.h>

static List *points = NULL;

jmp_buf *checkpoint(void)
{
	assert(points == 0 || points->ref.code == PTR);

	// Не можем оставить jmp_buf в стеке, потому что выйдем из этой функции.
	// Поэтому:

	jmp_buf *const buf = malloc(sizeof(jmp_buf));
	assert(buf);

	points = append(RL(refptr(buf)), points);

	// Было бы клёва, если бы можно было написать: return setjmp(*buf); но
	// невозможно вернуться в функцию, которой нет

	return buf;
}

void checkout(const int code)
{
	// Есть два варианта развития событий: points NULL-евой или нет. Если
	// NULL-евой, то возвращаться в случае сбоя некуда, просто выходим

	if(points == NULL)
	{
		if(code == 0)
		{
			return;
		}

		exit(EXIT_FAILURE);
	}

	// В любом случае надо освободить контекст, но перед этим надо сохранить
	// информацию из него, на случай code != 0

	assert(points->ref.code == PTR && tip(points)->ref.u.pointer);

	List *const p = tipoff(&points);
	jmp_buf buf = { (*(jmp_buf *)p->ref.u.pointer)[0] };
	free(p->ref.u.pointer);
	freelist(p);

	switch(code)
	{
	case 0: return;
	case STRIKE: longjmp(buf, STRIKE);
	default: assert(0);
	}
}

unsigned therearepoints(void)
{
	return points != NULL;
}
