#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "heap.h"

#include <stdio.h>
#include <limits.h>

enum { MAXHINT = 255, MAXLEN = (unsigned)-1 >> 1, CHUNKLEN = 32 };

// Имена для основных типов

typedef struct List List;
typedef struct Form Form;
typedef struct Array Array;
typedef struct Context Context;
typedef struct Environment Environment;

// Типы различных значений, используемых в алгоритмах. Метки записываются в поля
// с именем code в структурах Ref и Array

enum
{
	// Основные значения, используемые в процессе вывода. ATOM и TYPE
	// отмечают и соответствующие Array-и.
	// 
	// NUMBER, ATOM, TYPE - это числа и номера значений в таблицах атомов и
	// типов соответственно.
	// 	
	// EXP отмечает выражения, которые представлены списками. Списки
	// выражений должны иметь формат ("some-verb" (attributes)). Они
	// моделируют узлы в DAG-е программы.
	// 
	// LIST - это списки из разнообразных Ref-ов. Могут содержать Ref-ы на
	// другие списки. Поэтому используются для представления S-выражений.
	// 
	// FORM говорит об указателе на форму - кусочек DAG-а программы,
	// открытый для подстановки в него некоторых значений

	NUMBER, ATOM, TYPE, PTR, NODE, LIST, FORM,

	// Метка для обозначения окружений. Отмечает и Ref-ы и Array-и
	KEYTAB, ENV,

	// Метки, которые в будущем не понадобятся
	CTX,

	FREE = -1
};

typedef struct
{
	union
	{
		unsigned number;
		void *pointer;
		List *list;
		Form *form;
		Array *array;
		Context *context;
		Environment *environment;
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
extern Ref reftype(const unsigned);

extern Ref refptr(void *const);

// Особенность refnode в том, что она УСТАНАВЛИВАЕТ external-бит
extern Ref refnode(List *const);

extern Ref reflist(List *const);
extern Ref refform(Form *const);

extern Ref reftab(Array *const);
extern Ref refenv(Environment *const);

extern Ref refctx(Context *const);

extern Ref markext(const Ref);
extern Ref cleanext(const Ref);

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

	const unsigned code;
	const unsigned itemlength;

	unsigned *index;
	unsigned capacity;
	unsigned count;
};

extern Array *newarray(
	const unsigned code,
	const unsigned itemlen, const ItemCmp, const KeyCmp);

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
extern unsigned lookpack(Array *const, const AtomPack);

extern unsigned loadatom(Array *const, FILE *const);

extern unsigned loadtoken(
	Array *const, FILE *const,
	const unsigned char hint, const char *const format);

extern Atom atomat(const Array *const, const unsigned id);

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
	const Ref DLNAME = \
	{ \
		.code = LIST, \
		.u.list = (List[sizeof(REFS)/sizeof(Ref) - 1]) { 0 } \
	}; \
	formlist(DLNAME.u.list, REFS, sizeof(REFS)/sizeof(Ref) - 1)

// Записывает ссылки из списка (не рекурсивно, и не спускаясь в подсписки) в
// массив. Записывает не более N элементов, с учётом последнего с кодом FREE.
// Возвращает количество записанных элементов. При этом, если
//
//	R[writerefs(L, R, N) - 1].code == FREE
//
// то список выдан полностью.

extern unsigned writerefs(const List *const, Ref refs[], const unsigned N);

extern void freelist(const List *const);

// Получить первый элемент списка или N-ный (счёт от 0)

extern List *tip(const List *const);
extern List *listnth(const List *const, const unsigned N);

extern List *append(List *const, List *const);
extern List *tipoff(List **const);
extern unsigned listlen(const List *const);

// Различные варианты копирования списков. Самый простой вариант. Все ссылки
// будут повторены, под-списки будут скопированы, если для них не установлен
// external-бит

extern List *forklist(const List *const);

// Копирование списка с заменой узлов по отображению map. В скопированном списке
// вместо узла n будет узел (map n != -1 -> map n : n). Отображение
// конструируется как список из keytab-ов (см. ниже). Под-списки рекурсивно
// копируются, если для них не установлен external-бит

extern List *transforklist(const List *const, const List *const map);

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

// Выщипать из списка все звенья, в которых записана ref
extern void trimlist(List *const, const Ref);

extern char *strlist(const Array *const universe, const List *const);
extern void dumplist(
	FILE *const, const Array *const, const List *const);

// Базовая конструкция для окружений - это ассоциативная по ключам таблица. Эти
// таблицы, как и пингвины из Мадагаскара, в одиночку ходить не любят, а любят
// группировки в списки, чаще в LIFO, поэтому конструируются сразу в таком виде

extern List *pushkeytab(List *const tab);
extern List *popkeytab(List *const tab);

// Для массовой зачистки окружений в списках имеет смысл открыть доступ к
// процедуре. freekeytab реагирует на external-бит

extern void freekeytab(const Ref);

extern void dumpkeytab(
	FILE *const, const unsigned tabs, const Array *const U,
	const List *const keytab);

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

extern unsigned isbasickey(const Ref);

// Получить массив Binding-ов из таблицы на вершине стеков. Это порой может быть
// полезно для прохода по всем

extern Binding *tipbindings(const List *const, unsigned *const length);

// Специальные процедуры, которые декорирую ключи определёнными атомами, перед
// осуществлением поиска. Декорация происходит в виде ("some atom" key). Для
// этого нужен параметр U

extern Binding *formkeytobind(
	Array *const U,
	const List *const stack, unsigned *const depth, const Ref);

extern Binding *envkeytobind(
	Array *const U,
	const List *const stack, unsigned *const depth, const Ref);

// Для упрощения выражений процедуры для работы с задаваемыми keytab-ами
// отображениями. Они создаются в виде 1-элементных стеков таблиц с ключами. В
// каждое отображение можно добавить информации вызовом соответствующей
// tune-процедуры. Соответствующая map-процедура работает как применение
// отображение к аргументу. Добавлять в отображение можно только ту информацию,
// которой в нём ещё не было, иначе сработает assert. NULL-евое значение для
// параметра map процедуры трактуют как пустое отображение

// Базовое (в некотором смысле) отображение Ref -> Ref. Если отображение не
// знает про соответствующий аргумент, оно вернёт reffree

extern void tunerefmap(const List *const map, const Ref key, const Ref val);
extern Ref refmap(const List *const map, const Ref key);

// Отображение Ref -> (set 0 1). То есть, характеристическое для множества. Если
// отображение не знает о соответствующем аргументе, оно возвращает 0

extern void tunesetmap(const List *const map, const Ref key);
extern unsigned setmap(const List *const map, const Ref key);

// Отображение Ref -> void.ptr. Если отображение не знает об аргументе, оно
// возвращает NULL

extern void tuneptrmap(const List *const map, const Ref key, void *const ptr);
extern void *ptrmap(const List *const map, const Ref key);

// Отображения unsigned -> unsigned. Основное предназначение: осмысливание
// разных verb-ов выражений в разных контекстах. Чаще всего оно наполняется
// информацией по списку строчек, которые переводятся в атомы. Поэтому для него
// свой конструктор.
// 
// Если отображение не знает о своём аргументе, оно возвращает -1

extern List *newverbmap(
	Array *const U, const unsigned hint, const char *const atoms[]);

extern unsigned verbmap(const List *const, const unsigned verb);

// Ресурсы всех отображений освобождаются одинаково (это вообще простой
// (assert (popkeytab map == NULL))

extern void freemap(List *const map);

// Узлы.

// Узлы представлены выражениями вида (ll.h."some verb atom" attribute).
// Необходимо уметь их распаковывать. Параметр exp - само выражение для узла.
// Его удобнее сделать Ref-ой, потому что узлы в одиночку почти не ходят.
// Параметр vm (verbmap), если не NULL, задаёт отображение оригинального verb-а
// узла в некоторое числовое значение. Почти всегда на verb-ы нужно смотреть
// через такое отображение. Если vm == NULL, то verb возвращается так, как есть:
// (vm != NULL -> vm verb : verb)

extern unsigned nodeverb(const Ref exp, const List *const vm);
extern Ref nodeattribute(const Ref exp);

// Конструирование узла. Процедура вернёт Ref-у со сброшенным external-битом,
// что будет трактоваться как ссылка на определение узла, а не просто ссылка на
// узел (случай (Ref.code == NODE && Ref.external))

extern Ref newnode(const unsigned verb, const Ref attribute);

// Проверка структуры списка на то, что она действительно задаёт выражение
unsigned isnode(const List *const);

// Окружения из "деревьев" ассоциативных по ключам таблиц. Нечто вроде
// cactus stack-ов. Всё просто: есть собственная таблица, есть окружение ниже
// (ближе к корню дерева или дну стека), если окружения выше

struct Environment
{
	const List *const self;
	Environment *const down;
	List *up;
};

// Окружения конструируются вверх по дереву (стеку)

extern Environment *envup(Environment *const);
extern void freeenv(Environment *const);

// Поиск в окружениях сводится к поиску в стеке, состоящем из keytab-ов на пути
// от узла дерева к его вершине. Отвечает за keytab-ы само окружение, поэтому
// все Ref-ы в полученном списке-стеке будут отмечены external-битом. Список
// можно будет очистить freelist-ой

extern List *stackenv(Environment *const);

// Для сбора данных о том, что есть в окружении по нему надо уметь ходить сверху
// вниз. Разумно иметь два варианта прохода. (1) просто по самим окружениям

typedef void (*WalkEnvironment)(
	Environment *const, const List *const envid, void *const ptr);

extern void walkenv(Environment *const, const WalkEnvironment, void *const ptr);

// (2) по каждому Binding-у в каждом окружении

typedef void (*WalkBinding)(
	Binding *const, const List *const envid, void *const ptr);

extern void walkbind(
	Environment *const env, const WalkBinding, void *const ptr);

// Вариант (1) нужен, по крайней мере, для распечатки окружений

extern void dumpenv(
	FILE *const, const unsigned tabs, const Array *const U,
	const Environment *const env);

// DAG-и. Устроены как списки узлов в атрибутах которых бывают ссылки на другие
// узлы

// Некоторые нижеследующие процедуры содержат параметр map, который является
// verbmap-ом. Отображение описывает особые выражения. Особым выражением
// считается список, описывающий узел N: (verbmap map (nodeverb N) != -1).

// Загрузка dag-а. Атомы загружаются в U. Особые выражения в данном случае - это
// узлы, в атрибутах которых должен быть записан замкнутый граф.
// 
// Для управления памятью и для обхода 

extern Ref loaddag(FILE *const, Array *const U, const List *const map);

// Выгрузка dag-а. tabs - для красивой печати с отступами. dbg - выдавать ли
// указатели на узлы (для закрепления: на особые списки длиной 2) в выводе
// графов (это нужно для отладки)

extern void dumpdag(
	const unsigned dbg, FILE *const, const unsigned tabs,
	const Array *const U, const Ref dag, const List *const map);

extern Ref forkdag(const Ref dag);

extern void freedag(const Ref dag);

// Сборка мусорных не корневых узлов. Не корневые узлы определяются
// verbmap-ой.

extern void gcnodes(
	const Ref dag, const List *const map,
	const List *const nonroots, const List *const marks);

// Узел передаётся в WalkOne в разобранном виде. На атрибут передаётся ссылка,
// потому что в некоторых случаях walkone будет его переписывать

typedef int (*WalkOne)(
	const unsigned verb, Ref *const attribute, void *const);

// Процедура walkdag проходит по объявлениям выражений
// 
// 	(Ref.code == NULL && !Ref.external)
// 
// в графе (ну, граф условный у нас теперь). Для каждого такого объявления
// вызывается процедура walkone, в которую выражение передаётся в разобранном
// виде. Если walkone возвращает истину, то walkdag рекурсивно повторяется для
// атрибутов этого узла.

extern void walkdag(
	const Ref dag, const WalkOne, void *const, const List *const verbmap);

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
