#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "heap.h"

#include <stdio.h>
#include <limits.h>

enum { MAXHINT = 255, MAXLEN = (unsigned)-1 >> 1, CHUNKLEN = 32 };

// Имена для основных типов

typedef struct List List;
typedef struct Node Node;
typedef struct Form Form;
typedef struct Array Array;
typedef struct Context Context;

// Типы различных значений, используемых в алгоритмах

enum
{
	// Основные значения, используемые в процессе вывода. ATOM и TYPE
	// отмечают и соответствующие Array-и

	NUMBER, ATOM, TYPE, PTR, NODE, LIST, FORM,

	// Метка для представляющих окружения Array-ев
	ENV,

	// Метки, которые в будущем не понадобятся
	CTX,

	FREE = -1
};

typedef struct
{
	union
	{
		void *pointer;
		List *list;
		Node *node;
		Array *array;
		Form *form;
		Context *context;
		unsigned number;
	} u;

	unsigned code;
	
	// Метка для указания на то, что Ref ссылается на некий объект из
	// окружения, в котором работает алгоритм

	unsigned external:1;
} Ref;

// Конструкторы для Ref

extern Ref reffree(void);

extern Ref refnat(const unsigned code, const unsigned);

extern Ref refnum(const unsigned);
extern Ref refatom(const unsigned);
extern Ref refptr(void *const);

extern Ref refenv(Array *const);
extern Ref reflist(List *const);
extern Ref refnode(Node *const);

extern Ref refform(Form *const);

extern Ref refctx(Context *const);

extern Ref markext(const Ref);

// Ориентируясь на Ref.code вызвать соответствующее freexxx из доступных, если
// необходимо

extern void freeref(const Ref);

// Автоматически индексируемые массивы

struct Array
{
	const KeyCmp keycmp;
	const ItemCmp itemcmp;

	union
	{
		void *data;
		struct Array *nextfree;
	} u;

	unsigned *index;
	unsigned capacity;
	unsigned count;

	const unsigned itemlength;
	const int code;
};

extern Array *newarray(
	const int code, const unsigned itemlen, const ItemCmp, const KeyCmp);

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

extern Array *newatomtab(void);
extern void freeatomtab(Array *const);

extern unsigned readpack(Array *const, const AtomPack);
extern unsigned lookpack(const Array *const, const AtomPack);

extern unsigned loadatom(Array *const, FILE *const);

extern unsigned loadtoken(
	Array *const, FILE *const,
	const unsigned char hint, const char *const format);

extern Atom atomat(const Array *const, const unsigned id);

// Узлы

struct Node 
{
	union
	{
		Node *nextfree;
		List *attributes;
	} u;

	const unsigned verb;

	// В какой строке начинается описание узла. Важно для отладки
	const unsigned line;

	// Признак того, что в атрибутах узла записан dag со своими внутренними
	// ссылками

	const unsigned dag:1;
};

// Вроде, уже понятно, что узлы чаще бывают под Ref-ами. Для упрощения
// синтаксиса разумно возвращать здесь Ref

extern Ref newnode(
	const unsigned line,
	const unsigned verb, const List *const attributes, const unsigned dag);

extern void freenode(const Ref);

// Списки. Наши списки - это на деле s-выражения. Они связывают в структуры
// различные значения задаваемые Ref-ами

struct List 
{
	List * next;
	Ref ref;
};

// Слепить несколько ссылок в массив вида Ref[], воспринимаемый readrefs.
// Необходимо, чтобы он заканчивался ссылкой с кодом FREE. RS означает Refs

#define RS(...) ((const Ref[]) { __VA_ARGS__, (Ref) { .code = FREE } })

// Конструктор списка из массива ссылок. Чтобы сконструировать список из одной
// ссылки нужно написать: list = readrefs(RS(refnum(NUMBER, 42)))

extern List *readrefs(const Ref refs[]);

// Для упрощения синтаксиса RL - Ref List

#define RL(...) (readrefs(RS(__VA_ARGS__)))

// В работе с Environment-ами могут быть полезны списки в стеке, то есть,
// массивы из структур List, которые через поле .next связаны в правильный
// список.

extern void formlist(List listitems[], const Ref refs[], const unsigned len);

// Для упрощения синтаксиса DL - Define List (не особо удачное название)

#define DL(DLNAME, REFS) \
	const List DLNAME[sizeof(REFS)/sizeof(Ref) - 1]; \
	formlist((List *)DLNAME, REFS, sizeof(DLNAME)/sizeof(List))

// Записывает ссылки из списка (не рекурсивно, и не спускаясь в подсписки) в
// массив. Записывает не более N элементов, с учётом последнего с кодом FREE.
// Возвращает количество записанных элементов. При этом, если
//
//	R[writerefs(L, R, N) - 1].code == FREE
//
// то список выдан полностью.

extern unsigned writerefs(const List *const, Ref refs[], const unsigned N);

extern void freelist(List *const);

extern List *append(List *const, List *const);
extern List *tipoff(List **const);
extern List *tip(const List *const);
extern Ref *listnth(const List *const, const unsigned);
extern unsigned listlen(const List *const);

// Различные варианты копирования списков. Самый простой вариант. Все ссылки
// будут повторены, под-списки будут скопированы, если для них не установлен
// external-бит

extern List *forklist(const List *const);

// Копирование списка с заменой узлов по отображению map. В скопированном списке
// вместо узла n будет узел (map n). Отображение конструируется как список из
// keytab-ов (см. ниже). Под-списки рекурсивно копируются, если для них не
// установлен external-бит

extern List *transforklist(const List *const, const Ref map);

// Копирование кусочка списка, с позиции from до позиции to. -1 означает
// последний элемент в списке

extern List *forklistcut(
	const List *const, const unsigned from, const unsigned to,
	unsigned *const correct);

// Процедура forlist применяет другую функцию типа Oneach к каждому элементу
// списка, пока последняя возвращает значение, равное key. foreach устроена так,
// что позволяет менять ->next в обрабатываемом элементе списка.

typedef int (*Oneach)(List *const, void *const);
extern int forlist(List *const, Oneach, void *const, const int key);

extern char *strlist(const Array *const universe, const List *const list);

extern void dumplist(
	FILE *const, const Array *const universe, const List *const list);

// Базовая конструкция для окружений - это ассоциативная по ключам таблица. Эти
// таблицы, как и пингвины из Мадагаскара, в одиночку ходить не любят, а любят
// группировки в списки, чаще в LIFO, поэтому конструируются сразу в таком виде

extern List *pushkeytab(List *const stack);
extern List *popkeytab(List *const stack);

// Для массовой зачистки окружений в списках имеет смысл открыть доступ к
// процедуре. freekeytab реагирует на external-бит

extern void freekeytab(const Ref);

extern void dumpkeytab(
	FILE *const, const unsigned tabs, const Array *const U, 
	const Array *const env);

// keytobind - основная процедура для поиска ассоциаций в стеке таблиц с
// ключами.  Ассоциации устроены так

typedef struct
{
	const Ref key;
	Ref ref;
} Binding;

// Поиск может быть глубоким или поверхностным (то есть, только на вершине
// стека), сама найденная ассоциация может быть в глубине или на поверхности

enum { DEEP, SHALLOW };

// Параметр depth на входе указывает глубину поиска. На выходе в него
// записывается глубина (в терминах DEEP и SHALLOW) результата. Если в стеке на
// указанной глубине нет соответствующего key Binding-а, то он создаётся в
// таблице на вершине стека и инициируется с ref.code == FREE.
// 
// Параметр key описывает ключ для поиска. Ключами для поиска могут быть
// одиночные значения с Ref.code из { NUMBER, ATOM, TYPE, NODE, PTR } или же
// списки (s-выражения) из таких значений.
//
// Ref-а ключа будет скопирована в соответствующий Binding при его
// создании. При освобождении таблицы для этой Ref-ы ключа будет вызвана
// процедура freeref, которая реагирует на external-бит. Это надо учитывать и
// при помощи fork-ов и markext-ов управлять ответственностью за ключ

extern Binding *keytobind(
	const List *const stack, unsigned *const depth, const Ref key);

// В некоторых случаях необходима уверенность в том, что ключ состоит только из
// { NUMBER, ATOM, TYPE } элементов. Это позволяет проверить процедура

extern unsigned isbasickey(const List *const);

// Получить массив Binding-ов из таблицы на вершине стеков. Это порой может быть
// полезно для прохода по всем

extern Binding *tipbindings(const List *const stack, unsigned *const length);

// Специальные процедуры, которые декорирую ключи определёнными атомами, перед
// осуществлением поиска. Декорация происходит в виде ("some atom" key). Для
// этого нужен параметр U

extern Binding *formkeytobind(
	Array *const U,
	const List *const stack, unsigned *const depth, const Ref);

extern Binding *envkeytobind(
	Array *const U,
	const List *const stack, unsigned *const depth, const Ref);

// Окружения из "деревьев" ассоциативных по ключам таблиц. Нечто вроде
// cactus stack-ов. Всё просто: есть собственная таблица, есть окружение ниже
// (ближе к корню дерева или дну стека), если окружения выше

typedef struct Environment
{
	const List *const keytab;

	Environment *const down;
	List *up;
} Environment;

// Окружения конструируются вверх по дереву (стеку)

extern Environment *upenv(Environment *const down);
extern void freeenv(Environment *const env);

// Поиск в окружениях сводится к поиску в стеке, состоящем из keytab-ов на пути
// от узла дерева к его вершине. Отвечает за keytab-ы само окружение, поэтому
// все Ref-ы в полученном списке-стеке будут отмечены external-битом. Список
// можно будет очистить freelist-ой

extern List *stackenv(Environment *const env);

// Для сбора данных о том, что есть в окружении по нему надо уметь ходить сверху
// вниз. Разумно иметь два варианта прохода. (1) просто по самим окружениям

typedef void WalkEnvironment(
	Environment *const, const List *const envid, void *const ptr);

extern void walkenv(
	Environment *const env, const WalkBinding, void *const ptr);

// (2) по каждому Binding-у в каждом окружении

typedef void WalkBinding(
	Binding *const, const List *const envid, void *const ptr);

extern void walkbind(
	Environment *const env, const WalkEnvironment, void *const ptr);

// Вариант (1) нужен, по крайней мере, для распечатки окружений

extern void dumpenv(
	FILE *const, const unsigned tabs, const Array *const U,
	const Environment *const env);

// Семантические функции

// Некоторые нижеследующие функции содержат параметр go. Он является
// отображением verb -> (set 0 1), задающие verb-ы тех узлов (N) с графами в
// атрибутах (N.dag == 1), в которые алгоритм должен рекурсивно спускаться. Если
// параметр go не задан для процедуры, значит, алгоритм рекурсивно спускается во
// все такие графы. Отображение реализовано Array-ем со структурой ENV, поэтому
// задаётся Ref-ой

// Далее отображения verb -> (set 0 1) будем называть verbmap-ами

// Загрузка dag-а. Атомы загружаются в universe. Отображение map - это verbmap,
// отмечающая узлы, которые должны содержать другие dag-и в своих атрибутах. У
// каждого dag-а своя область видимости по ссылкам, они не могут явно ссылаться
// на узлы друг друга

extern List *loaddag(FILE *const, Array *const universe, const Ref map);

// Выгрузка dag-а. tabs - для красивой печати с отступами. dbg - выдавать ли
// указатели на узлы в выводе графов (это нужно для отладки)

extern void dumpdag(
	const unsigned dbg, FILE *const, const unsigned tabs,
	const Array *const U, const List *const dag);

// Создать согласованную с таблицей атомов U отображение verbmap по списку
// строк, оканчивающемуся NULL. В полученной verbmap на i-том месте будет стоять
// номер атома, который описывается (hint (strlen (atoms i)) (atoms i))

extern Ref keymap(
	Array *const U, const unsigned hint, const char *const atoms[]);

extern List *forkdag(const List *const dag);

extern void freedag(List *const dag);

// Сборка мусорных не корневых узлов. Не корневые узлы определяются
// verbmap-ой.

extern List *gcnodes(List **const dag, const Ref go, const Ref nonroots);

typedef void (*WalkOne)(List *const, void *const);

extern void walkdag(
	const List *const dag, const Ref go, const WalkOne, void *const);

extern List *evallists(
	Array *const U,
	List **const dag, const Ref go, const List *const arguments);

// Форма. У неё есть сигнатура, определяющая способ встраивания формы в текущий
// выводимый граф и dag с описанием тела формы.  Счётчик необходим для
// отслеживания процесса активации формы. Форма активируется, когда в контексте
// вывода появляется необходимое количество выходов, соответствующих её
// сигнатуре. Метка goal отмечает целевые формы

struct formtag
{
	union
	{
		const List *const dag;
		struct formtag *nextfree;
	} u;

	const List *const signature;

	unsigned count;
};

// Из-за сложной жизни форм (cf. txt/worklog.txt:2690 2013-09-04 11:55:50) имеет
// смысл во freeform передавать Ref и, соответственно, возвращать Ref из
// newform. Потому что они всегда связаны, получается: формы бывают либо в
// окружениях (там Ref-ы), либо в списках (тоже Ref-ы)

extern void freeform(const Ref);

extern Ref newform(const List *const dag, const List *const signature);

// Структура контекста вывода

// В соответствии с txt/worklog.txt:2331 2013-09-01 22:31:36

typedef struct
{
	List *outs;
	List *ins;
	List *forms;
} Reactor;

struct contexttag
{
	// Выращенная в этом контексте часть графа программы. Сюда дописывается
	// содержимое активированных форм.

	List *dag;

	// Это та часть, которая называется в worklog реакторами. Но всерьёз
	// писать ->reactor в коде? Настолько ли мы безумные программисты?

	Reactor R[2];

	// marker пригодится для проверки соответствий. Нужен ли state - пока не
	// понятно

	unsigned state;
	const unsigned marker;
};

// Варианты состояния контекста

enum { EMPTY, RIPENING, RIPE };

// Контексты собираются в стеки

// При размещении нового контекста имеет смысл указать его состояние и маркер
// (последний для проверки корректности). Маркер - это номер атома

extern List *pushcontext(
	List *const ctx, const unsigned state, const unsigned marker);

// При очищении контекста нужно знать структуру выращенного в нём графа, не
// понятно наперёд, какая именно это карта должна быть (взятая из форм или что?
// любые гипотезы приветствуются). Поэтому просто параметр

extern List *popcontext(List **const pctx);

// Слияние двух контекстов на вершине стека. Тот, что сверху дописывается к
// тому, что снизу - это описание формирования порядка dag-ов

extern List *mergecontext(const Array *const U, List *const ctx);

extern unsigned isforwardempty(const List *const ctx);

extern void dumpcontext(FILE *const, const Array *const, const List *const ctx);

// Выбирает из текущего графа формы и размещает их в соответствии с указаниями
// публикации: .FPut, .FGPut, .FEPut. Публикация осуществляется на вершинах
// двух указанных стеков: областей видимости и контекстов вывода.

// WARN: У формы есть ссылка на карту графа, эта ссылка будет взята из аргумента
// evalforms

extern void evalforms(
	Array *const U, const List *const dag, const Ref go,
	const List *const env, const List *const ctx);

// Синтаксические команды. Тут и дальше получается некий свободный поток
// примитивов, не сгруппированный и не упорядоченный

#define FOP 0
#define AOP 1
#define UOP 2
#define LOP 3
#define BOP 4
#define EOP 5

// О Position заметка txt/worklog.txt Position 2013-08-27 18:24:10

typedef struct
{
	// номер атома с именем файла
	unsigned file;
	unsigned line;
	unsigned column;
} Position;

typedef struct
{
	const unsigned op;
	const unsigned atom;
	const Position pos;
} SyntaxNode;

// Самая главная функция. Новые атомы могут появится в ходе вывода графа
// программы (например, атомы меток), поэтому universe не константа. Стеки
// окружений или контекстов тоже могут быть расширены или удалены, поэтому
// передаются ссылки на значения их описывающие (а такими значениями являются
// списки, которые указываются указателем).

extern void progress(
	Array *const universe,
	const List **const penv, const List **const ctx,
	const SyntaxNode cmd);

// Процедура вбрасывания в контекст новой формы. Параметр level задаёт тот R
// (реактор - гы, я это всерьёз написал), в который следует забрать форму. Форма
// задаётся тройкой параметров: (dag; map), signature и external. U необходима
// для распечатки ошибок внутри intakeform

enum { ITLOCAL = 0, ITEXTERNAL };

extern void intakeform(
	const Array *const U,
	Context *const, const unsigned level,
	const List *const dag,
	const List *const signature, const unsigned external);

extern void intakeout(
	const Array *const U,
	Context *const, const unsigned level, const List *const outs);

#endif
