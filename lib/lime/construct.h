#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "heap.h"

#include <stdio.h>

// Имена для основных типов

typedef struct NodeTag Node;
typedef struct ListTag List;
typedef struct ArrayTag Array;

// Индексированные массивы


struct ArrayTag {
	KeyCmp keycmp;
	ItemCmp itemcmp;

	void *data;
	unsigned *index;
	unsigned capacity;
	unsigned itemlength;
	unsigned count;

	int code;
};

// mk - это make; rl - release
extern Array mkarray(const int code, const unsigned itemlen,
	const ItemCmp, const KeyCmp);

extern void rlarray(Array *const);

extern void *itemat(const Array *const, const unsigned);
extern void *readin(Array *const, const void *const val);
extern unsigned lookup(const Array *const, const void *const key);

// Таблицы атомов

typedef const unsigned char *Atom;

typedef struct {
	const unsigned char *bytes;
	unsigned length;
	unsigned char hint;
} AtomPack;

extern unsigned atomlen(const Atom);
extern unsigned atomhint(const Atom);
extern AtomPack atompack(const Atom);
extern const unsigned char *atombytes(const Atom);

extern Array mkatomtab(void);
extern void rlatomtab(Array *const);

extern unsigned readpack(Array *const, const AtomPack *const);
extern unsigned lookpack(Array *const, const AtomPack *const);
extern unsigned loadatom(Array *const, FILE *const);

extern unsigned loadtoken(Array *const, FILE *const,
	const unsigned char hint, const char *const format);

// Узлы

struct NodeTag {
	unsigned nrefs;
	unsigned code;
	union {
		Node *nextfree;
		List *sources;
	} u;

	// Некая дополнительная информация, которая может быть специально
	// проинтерпретирована пользователем. Рассчёт на то, что extra -- это
	// индекс в некотором массиве
	unsigned extra;
};

extern Node *newnode(const unsigned code);
extern void freenode(Node *const);

// Списки

enum { NUMBER, ATOM, TYPE, NODE, ENV, LIST, FREE = -1 };

struct ListTag {
	List * next;
	union {
		List *list;
		Node *node;
		Array *environment;
		unsigned number;
	} u;
	int code;
};

// withsubstructure - флаг, указывающий на то, следует ли выделять ту структуру,
// на которую будет ссылаться новый элемент списка
extern List * newlist(const int code, const unsigned withsubstructure);

extern List * extend(List *const, List *const);

// Создание нового списка копированием списка-ссылки. При копировании ссылок на
// узлы счётчик ссылок в них увеличвается. Новые узлы не создаются
extern List * forklist(const List *const);

extern void rllist(List *const);

extern char *dumplist(const List *const);

// Функция forlist применяет другую функцию типа Oneach к каждому элементу
// списка, пока последняя возвращает значение, равное key. foreach устроена так,
// что позволяет менять ->next в обрабатываемом элементе списка.
typedef int (*Oneach)(List *const, const unsigned, void *const);
extern int forlist(List *const, Oneach, void *const, const int key);

// Окружения

typedef struct {
	const List *key;
	union {
		Node *node;
		void *generic;
	} u;
	int code;
} Binding;

extern Array mkenv(void);
extern void rlenv(Array *const);

extern unsigned attachbinding(Array *const, const List *const key, const int code,
const void *const);

extern const Binding *lookbinding(const List *const env, const List *const key);

// Семантические функции

// Загрузить сырой список из файла (сырой - такой, в котором не подставлены типы
// и атомы) используя универсальную таблицу (надо куда-то складывать прочитанные
// "зюквочки"
extern List * loadrawlist(Array *const universe, List *const env, FILE *);

#endif
