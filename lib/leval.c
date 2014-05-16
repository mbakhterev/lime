#include "construct.h"
#include "util.h"

#include <assert.h>

void dolnode(Marks *const M, const Ref N, const Core *const C)
{
	const Array *const U = C->U;
	const Array *const V = C->verbs.system;

	Array *const marks = M->marks;

	const Ref val = simplerewrite(nodeattribute(N), marks, V);
	if(val.code == FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	// Сохраняем !external ссылку на полученное значение в marks. Чтобы
	// стереть выражение вместе с marks

	Binding *const b
		= (Binding *)bindingat(marks, mapreadin(marks, markext(N)));
	assert(b);

	b->ref = val;
}
