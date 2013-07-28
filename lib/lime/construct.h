#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "heap.h"

#include <stdio.h>
#include <limits.h>

enum { MAXHINT = 255, MAXLEN = (unsigned)-1 >> 1, CHUNKLEN = 32 };

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

typedef struct
{
	const Array *array;
	unsigned position;
} GDI; // Global Datum Index :)

extern Array makearray(const int code, const unsigned itemlen,
	const ItemCmp, const KeyCmp);

extern void freearray(Array *const);

extern unsigned readin(Array *const, const void *const val);
extern unsigned lookup(const Array *const, const void *const key);
extern void *itemat(const Array *const, const unsigned);

// Таблицы атомов

typedef const unsigned char *Atom;

typedef struct
{
	const unsigned char *bytes;
	unsigned length;
	unsigned char hint;
} AtomPack;

extern unsigned atomlen(const Atom);
extern unsigned atomhint(const Atom);
extern AtomPack atompack(const Atom);
extern const unsigned char *atombytes(const Atom);

extern AtomPack strpack(const unsigned hint, const char *const str);

extern Array makeatomtab(void);
extern void freeatomtab(Array *const);

extern unsigned readpack(Array *const, const AtomPack);
extern unsigned lookpack(Array *const, const AtomPack);

extern unsigned loadatom(Array *const, FILE *const);
extern unsigned loadtoken(Array *const, FILE *const,
	const unsigned char hint, const char *const format);

extern Atom atomat(const Array *const, const unsigned id);

// Узлы

struct NodeTag
{
	union
	{
		Node *nextfree;
		List *attributes;
	} u;

	unsigned verb;
};

extern Node *newnode(const unsigned verb, const List *const attributes);
extern void freenode(Node *const);

// Списки

// Основное предназначение списков - быть структурированными (имеется в виду,
// что с под-списками) ссылки на другие объекты: числа, атомы, типы, узлы. Числа
// могут быть заданы прямо в элементе списка (это ссылки на объективную
// численную реальность), атомы и типы тоже могут быть заданы прямо в элементе
// своими номерами в таблицах. Узлы задаются ссылкой на соответствующую
// структуру данных.

// Списки могут быть использованы и для специальных случаев: стэк областей
// видимости, например (Environment).

// Имеет смысл отдельно вынести описание вариантов ссылок, которые могут быть в
// элементе списка

typedef struct {
	unsigned code;
	union {
		void *pointer;
		List *list;
		Node *node;
		Array *environment;
		unsigned number;
	} u;
} Ref;

// Конструкторы для Ref

extern Ref refnat(const unsigned code, const unsigned);

extern Ref refnum(const unsigned);
extern Ref refatom(const unsigned);
extern Ref refptr(void *const);

extern Ref refenv(Array *const);
extern Ref reflist(List *const);
extern Ref refnode(Node *const);

enum { NUMBER, ATOM, TYPE, LIST, NODE, ENV, MAP, PTR, FREE = -1 };

struct ListTag {
	List * next;
	Ref ref;
};

// r - это Ref, которая определяет, как будет сконструирован список. Если по
// семантике ссылка должна быть числом (NUMBER, ATOM, TYPE), то копируется поле
// r.number. Если списком (LIST), то копируется поле n.list, даже если оно NULL
// (пустые подсписки бывают). Если узлом (NODE), то ссылка копируется, если
// r.node != NULL; если же r.node == NULL, то создаётся пустой узел с кодом FREE

// newlist - неудобный вариант конструктора
// extern List *newlist(const int code, Ref r);

// Слепить несколько ссылок в массив вида Ref[], воспринимаемый readrefs.
// Необходимо, чтобы он заканчивался ссылкой с кодом FREE. RS означает Refs

#define RS(...) ((const Ref[]) { __VA_ARGS__, (Ref) { .code = FREE } })

// Конструктор списка из массива ссылок. Чтобы сконструировать список из одной
// ссылки нужно написать: list = readrefs(RS(refnum(NUMBER, 42)))

extern List *readrefs(const Ref refs[]);

// Для упрощения синтаксиса RL - Ref List

#define RL(...) (readrefs(RS(__VA_ARGS__)))

// Записывает ссылки из списка (не рекурсивно, и не спускаясь в подсписки) в
// массив. Записывает не более N элементов, с учётом последнего с кодом FREE.
// Возвращает количество записанных элементов. При этом, если
//
//	R[writerefs(L, R, N) - 1].code == FREE
//
// то список выдан полностью.

extern unsigned writerefs(const List *const, Ref refs[], const unsigned N);

extern List *append(List *const, List *const);
extern List *tipoff(List **const);
extern List *tip(const List *const);

// Копирование списка с заменой ссылок на узлы (NODE). Если nodemap - это NULL,
// то должно быть: nodes == NULL && bound == NULL. И в этом случае получается
// реализация forklist. nodemap (M) и nodes (N) задают отображение ссылок в
// исходном списке на новые узлы таким образом: m -> N[ptrreverse(M, m)]. При
// этом должно выполняться: ptrreverse(M, m) < bound. Условие нужно для контроля
// корректности в процедуре forkdag.

extern List *transforklist(
	const List *const,
	const Array *const nodemap, const Ref nodes[], const unsigned bound);

extern List *forklistcut(
	const List *const, const unsigned from, const unsigned to,
	unsigned *const correct);

extern List *forklist(const List *const);


extern void freelist(List *const);
extern char *dumplist(const List *const);

// Функция forlist применяет другую функцию типа Oneach к каждому элементу
// списка, пока последняя возвращает значение, равное key. foreach устроена так,
// что позволяет менять ->next в обрабатываемом элементе списка.

typedef int (*Oneach)(List *const, void *const);
extern int forlist(List *const, Oneach, void *const, const int key);

extern unsigned listlen(const List *const);

// Окружения

extern Array makeenvironment(void);
extern void freeenvironment(Array *const);
extern GDI readbinding(Array *const, const Ref, const List *const key);
extern GDI lookbinding(const List *const, const List *const key);

extern Ref gditoref(const GDI);
extern Ref *gditorefcell(const GDI);

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
extern unsigned uidirect(const Array *const, const unsigned);
extern unsigned uireverse(const Array *const, const unsigned);

extern Array makeptrmap(void);
extern void freeptrmap(Array *const);

extern unsigned ptrmap(Array *const, const void *const);
extern unsigned ptrreverse(const Array *const, const void *const);
extern const void *const ptrdirect(const Array *const, const unsigned);

// Семантические функции

// Загрузка dag-а. Атомы загружаются в universe. dagmap отмечает узлы, которые
// должны содержать другие dag-и в своих атрибутах. У каждого dag-а своя область
// видимости по ссылкам, они не могут явно ссылаться на узлы друг друга.

extern List *loaddag(
	FILE *const, Array *const universe, const Array *const dagmap);

// Выгрузка dag-а. tabs - для красивой печати с отступами.

extern void dumpdag(
	FILE *const, const unsigned tabs, const Array *const universe,
	const List *const dag, const Array *const dagmap);

// Создать согласованную с таблицей атомов keymap по списку строк. Список строк,
// оканчивающийся NULL

extern Array keymap(Array *const universe,
	const unsigned hint, const char *const atoms[]);


// "Карта" для прохода по графу. В DagMap M оба поля - uimap-ы. M.map содержит
// verb-ы узлов, атрибутами которых являются подграфы. M.go - описывает узлы,
// в подграфы которых следует проходить рекурсивно. Некоторые процедуры
// игнорируют M.go

// Например, зайти во все под-dag-и:
// 	walkdag(dag, makedagmap(U, 0, verbs, verbs), walkone, ptr);

typedef struct
{
	Array map;
	Array go;
} DagMap;

extern DagMap makedagmap(
	Array *const universe, const unsigned hint,
	const char *const dagverbs[], const char *const dive[]);

extern void freedagmap(DagMap *const);

extern unsigned isdag(const DagMap *const, const unsigned verb);
extern unsigned isgodag(const DagMap *const, const unsigned verb);

// fork и free dag не используют DagMap.go

extern List *forkdag(const List *const dag, const DagMap *const);
extern void freedag(List *const dag, const DagMap *const);

// Сборка мусорных не корневых узлов. Не корневые узлы определяются
// uimap-отображением nonroots.

extern List *gcnodes(
	List **const dag,
// 	const Array *const dagmap,
	const DagMap *const dagmap, const Array *const nonroots);

typedef void (*WalkOne)(List *const, void *const);

extern void walkdag(
	const List *const dag,
//	const Array *const dagmap, const Array *const divedag,
	const DagMap *const, const WalkOne, void *const);

extern List *evallists(
	Array *const U,
	List **const dag, const DagMap *const,
	const List *const arguments);

#endif
