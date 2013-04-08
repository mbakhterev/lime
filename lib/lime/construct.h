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
extern Array makearray(const int code, const unsigned itemlen,
	const ItemCmp, const KeyCmp);

extern void freearray(Array *const);

extern unsigned readin(Array *const, const void *const val);
extern unsigned lookup(const Array *const, const void *const key);
extern void *itemat(const Array *const, const unsigned);

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

extern Array makeatomtab(void);
extern void freeatomtab(Array *const);

extern unsigned readpack(Array *const, const AtomPack *const);
extern unsigned lookpack(Array *const, const AtomPack *const);
extern unsigned loadatom(Array *const, FILE *const);

extern unsigned loadtoken(Array *const, FILE *const,
	const unsigned char hint, const char *const format);

// Узлы

struct NodeTag {
	union {
		Node *nextfree;
		List *sources;
	} u;

	unsigned code;
	
	// Некая дополнительная информация, которая может быть специально
	// проинтерпретирована пользователем. Рассчёт на то, что extra -- это
	// индекс в некотором массиве
	unsigned extra;

	// Отметка о посещении для различных алгоритмов обхода dag-а. Например,
	// для mark-and-sweep сборщика мусора
	unsigned mark:1;
};

extern Node *newnode(void);
extern void freenode(Node *const);

// Списки

enum { NUMBER, ATOM, TYPE, LIST, NODE, ENV, MAP, FREE = -1 };

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

// allocate - флаг, указывающий на то, следует ли выделять ту структуру, на
// которую будет ссылаться новый элемент списка
extern List * newlist(const int code, const unsigned allocate);

extern List * extend(List *const, List *const);
extern List * forklist(const List *const);
extern void freelist(List *const);
extern char *dumplist(const List *const);

// Функция forlist применяет другую функцию типа Oneach к каждому элементу
// списка, пока последняя возвращает значение, равное key. foreach устроена так,
// что позволяет менять ->next в обрабатываемом элементе списка.
typedef int (*Oneach)(List *const, void *const);
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

extern Array makeenvironment(void);
extern void freeenvironment(Array *const);
extern unsigned readbinding(Array *const, const Binding *const);
extern const Binding *lookbinding(const List *const env, const List *const key);

// Биективное unsigned -> unsigned отображение. Предназначение двойное.

// 1. Нужно для того, чтобы создавать локальные "карты" атомов и типов. Когда
// нужно загружать модуль поверх уже загруженных (или инициализированных)
// структур данных для атомов и типов, то ссылки на атомы и типы по номерам в
// загружаемом модуле не будут соответствовать накопленной информации. Нужно эти
// локальные номера отображать в глобальные. Это прямое отображение.

// 2. Иногда нужно выделять особые типы и атомы, чтобы специально их
// обрабатывать. Тогда можно перенумеровать эти атомы при помощи обратного
// отображения. Например, узнать локальный порядковый номер атома должно помочь
// выражение: reverse(&map, lookpack(&atoms, &atompack));

extern Array makeuimap(void);
extern void freeuimap(Array *const);

extern unsigned uimap(Array *const, const unsigned);
extern unsigned direct(const Array *const, const unsigned);
extern unsigned reverse(const Array *const, const unsigned);

// Семантические функции

// Обобщённая загрузка dag-а (списка узлов, со списками ссылок на другие узлы в
// каждом) из файла. Загрузка происходит с узнаванием "ключевых узлов".  Узлы
// описываются атомами, ключевые заданы специальным uimap. Когда loadrawdag
// замечает такой узел, он вызывает соответствующую (по reverse(atomnum))
// функцию типа LoadAction для специальной обработки узла. Это необходимо для
// специальной логики обработки специальных узлов.

typedef struct LoadContextTag LoadContext;
typedef List *(*LoadAction)(LoadContext *const, void *const state);

struct LoadContextTag {
	FILE *file;
	void *state;
	
	List *env;			// окружение из имён узлов
	Array *universe;		// общая таблица атомов
	Array *keymap;			// ключевые атомы в ней
	const LoadAction *actions;	// специальные действия

	unsigned keyonly:1;		// допускать узлы только в keymap
};

extern List *loadrawdag(LoadContext *const, void *const state);

// Создать согласованную с таблицей атомов keymap по списку атомов в массиве из
// строк.

extern Array keymap(Array *const universe, const unsigned hint,
	const char *const atoms[], const unsigned N);

// Некоторые стандартные LoadAction

// 'ANum x = Num -- особенность в том, что Num не является списоком
// 'TNum -- аналогично
extern const LoadAction onatomnum;
extern const LoadAction ontypenum;

// 'ALook x = 01.2."34" -- описание атома не является списоком
extern const LoadAction onatomlook;

// 'F x = (...) -- список узлов формы должен быть загружен в новом окружении
extern const LoadAction onform;

#endif

