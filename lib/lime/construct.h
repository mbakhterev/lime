#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "atomtab.h"

#include <stdio.h>

typedef struct ListStruct List;

typedef struct {
	unsigned code;
	unsigned suffix;
	union {
		unsigned number;
		List * sources;
	} u;
	unsigned nrefs;
} Node;

enum { NUMBER, ATOM, TYPE, NODE, LIST, FREE = -1 };

struct ListStruct {
	List * next;
	union {
		List * list;
		Node * node;
		unsigned number;
	} u;
	int code;
};

extern List * newlist(const unsigned code);
extern List * extend(List *const, List *const);

typedef struct {
	const List * key;
	union {
		Node * node;
	} u;
	unsigned code;
	unsigned id;
} Binding;

typedef struct {
	Array bindings;
	Array index;
} Environment;

extern Environment mkenvironment();
extern void freeenvironment(Environment *const);

extern unsigned readbinding(
	Environment *const, const List *const key, const Binding value);

extern unsigned lookbinding(Environment *const, const List *const key);

// Загрузить сырой список из файла (сырой - такой, в котором не подставлены типы
// и атомы) используя универсальную таблицу (надо куда-то складывать прочитанные
// "зюквочки".

extern List * loadrawlist(AtomTable * universe, List *const env, FILE *);

#endif
