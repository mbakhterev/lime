#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "atomtab.h"

#include <stdio.h>

typedef struct ListStruct List;
typedef struct EnvironmentStruct Environment;

typedef struct {
	List * sources;
	unsigned code;
	unsigned nrefs;

	// Некая дополнительная информация, которая может быть специально
	// проинтерпретирована пользователем. Рассчёт на то, что extra -- это
	// индекс в некотором массиве
	unsigned extra;
} Node;

enum { NUMBER, ATOM, TYPE, NODE, ENV, LIST, FREE = -1 };

struct ListStruct {
	List * next;
	union {
		List *list;
		Node *node;
		Environment *environment;
		unsigned number;
	} u;
	int code;
};

// withsubstructure - флаг, указывающий на то, следует ли выделять ту структуру,
// на которую будет ссылаться новый элемент списка
extern List * newlist(const unsigned code, const unsigned withsubstructure);

extern List * extend(List *const, List *const);

// Создание нового списка копированием списка-ссылки. При копировании ссылок на
// узлы счётчик ссылок в них увеличвается. Новые узлы не создаются
extern List * forklist(const List *const);

extern void releaselist(List *const);

extern char *dumplist(const List *const);

// Функция forlist применяет другую функцию типа Oneach к каждому элементу
// списка, пока последняя возвращает значение, равное key. foreach устроена так,
// что позволяет менять ->next в обрабатываемом элементе списка.
typedef int (*Oneach)(List *const, const unsigned, void *const);
extern int forlist(List *const, Oneach, void *const, const int key);

typedef struct {
	const List * key;
	union {
		Node * node;
	} u;
	unsigned code;
	unsigned id;
} Binding;

struct EnvironmentStruct {
	Array bindings;
	Array index;
};

extern Environment mkenvironment();
extern void freeenvironment(Environment *const);

extern unsigned readbinding(
	Environment *const, const List *const key, const Binding value);

extern const Binding *lookbinding(const List *const env, const List *const key);

// Загрузить сырой список из файла (сырой - такой, в котором не подставлены типы
// и атомы) используя универсальную таблицу (надо куда-то складывать прочитанные
// "зюквочки"
extern List * loadrawlist(AtomTable * universe, List *const env, FILE *);

#endif
