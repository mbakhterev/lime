#include "construct.h"
#include "util.h"
#include "nodeutil.h"

#define DBGDONE 1

// #define DBGFLAGS (DBGDONE)

#define DBGFLAGS 0

#include <assert.h>

static unsigned maypass(Array *const U, const Array *const map)
{
	return isactive(U, map);
}

static Array *newtarget(
	Array *const U, const Array *const map, const Ref id, void *const ptr)
{
	return newarea(U, readtoken(U, "INTERNAL"), areaenv(U, map));

// 	Array *const T = newarea(U, readtoken(U, "INTERNAL"), areaenv(U, map));
// 	assert(T);
// 
// 	Array *const links = arealinks(U, T);
// 	assert(links);
// 
// // FIXME: оторвать за такое руки
// // 	assert(linkmap(U, T,
// // 		readtoken(U, "CTX"), readtoken(U, "SELF"), refkeymap(T)) == T);
// 
// 	assert(linkmap(U, links,
// 		readtoken(U, "CTX"), readtoken(U, "SELF"), refkeymap(T)) == T);
// 	
// 	return T;
}

static Array *nextpoint(Array *const U, const Array *const map)
{
	return arealinks(U, map);
}

void dornode(
	Core *const C, Array *const area, Marks *const M, const Ref N)
{
	Array *const U = C->U;
	const Array *const V = C->verbs.system;
	const Array *const marks = M->marks;
	Array *const formmarks = M->areamarks;

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(U, N));
		return;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	const Ref key = len > 0 ? simplerewrite(R[0], marks, V) : reffree();
	const Ref target = len > 1 ? refmap(formmarks, R[1]) : reffree();

	if(len < 1 || 2 < len
		|| key.code != LIST || !isbasickey(key)
		|| (len == 2 && target.code != MAP))
	{
		freeref(key);

// 		item = nodeline(N);
// 		ERR("node \"%s\": wrong attribute structure", nodename(U, N));

		ERRNODE(U, N, "%s", "wrong attribute structure");

		return;
	}

	Array *const T
		= makepath(area, U, readtoken(U, "CTX"), key.u.list,
			target, maypass, newtarget, nextpoint, NULL);

	if(!T)
	{
		char *const kstr = strref(U, NULL, key);
		freeref(key);

		ERR("node \"%s\": can't %s area with key: %s",
			nodename(U, N),
			len == 1 ? "trace" : "bind",
			kstr);

		free(kstr);
		return;
	}

	assert(target.code == FREE || T == target.u.array);
	
	freeref(key);

	tunerefmap(formmarks, N, refkeymap(T));
}

void dodone(Array *const U, Array *const area, const Ref N)
{
	DBG(DBGDONE, "done with area: %p", (void *)area);

	const Ref r = nodeattribute(N);
	if(r.code != LIST || listlen(r.u.list) != 0)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	if(!isactive(U, area))
	{
		item = nodeline(N);
		ERR("node \"%s\": can't kill already inactive area",
			nodename(U, N));
		return;
	}

	if(!unlinkarealinks(U, area))
	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't kill interlinks", nodename(U, N));

		ERRNODE(U, N, "%s", "can't kill interlinks");

		return;
	}

	unlinkareareactor(U, area, 0);
	unlinkareaenv(U, area);

	markactive(U, area, 0);
}

unsigned dogo(
	Array *const U,
	const Ref N, const Array *const area,
	const Marks *const M, const unsigned envtogo)
{
	const Array *const marks = M->marks;

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting attribute list", nodename(U, N));

		ERRNODE(U, N, "%s", "expecting attribute list");
		return -1;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);
	
	const Ref target = len > 0 ? refmap(marks, R[0]) : reffree();

	if(len != 1 || target.code != ENV)
	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": wrong attribute structure", nodename(U, N));

		ERRNODE(U, N, "%s", "wrong attribute structure");
		return -1;
	}

	if(envtogo != -1)
	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": Go already pointed", nodename(U, N));

		ERRNODE(U, N, "%s", "Go already detected");
		return -1;
	}

	if(!isontop(U, area))
	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can Go only from stack top", nodename(U, N));

		ERRNODE(U, N, "%s", "can Go only from stack top");
		return -1;
	}

	return target.u.number;
}

List *dorip(Array *const U, const Ref N, const Marks *const M)
{
	const Array *const formmarks = M->areamarks;

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(U, N));
		return NULL;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	const Ref A = len == 1 ? refmap(formmarks, R[0]) : reffree();
	
	if(len != 1 || !isarea(A))
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return NULL;
	}

	Array *const area = A.u.array;

	if(isareaconsumed(U, area))
	{
		item = nodeline(N);
		ERR("node \"%s\": area is already ripped", nodename(U, N));
		return NULL;
	}

	if(isactive(U, area))
	{
		item = nodeline(N);
		ERR("node \"%s\": can't rip active area", nodename(U, N));
		return NULL;
	}

	const Ref trace;
	const Ref body;
	riparea(U, area, (Ref *)&body, (Ref *)&trace);
	assert(isdag(body) && isdag(trace));
	freeref(body);

	return trace.u.list;
}
