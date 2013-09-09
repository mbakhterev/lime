#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "heap.h"

#include <stdio.h>
#include <limits.h>

enum { MAXHINT = 255, MAXLEN = (unsigned)-1 >> 1, CHUNKLEN = 32 };

// Имена для основных типов

typedef struct nodetag Node;
typedef struct listtag List;
typedef struct arraytag Array;
typedef struct formtag Form;
typedef struct contexttag Context;

// Автоматически индексируемые массивы

struct arraytag
{
	const KeyCmp keycmp;
	const ItemCmp itemcmp;

	void *data;
	unsigned *index;
	unsigned capacity;
	unsigned itemlength;
	unsigned count;

	const int code;
};

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

struct nodetag
{
	union
	{
		Node *nextfree;
		List *attributes;
	} u;

	const unsigned verb;

	// В какой строке начинается описание узла. Важно для отладки
	const unsigned line;
};

extern Node *newnode(
	const unsigned line, const unsigned verb, const List *const attributes);

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

typedef struct
{
	union
	{
		void *pointer;
		List *list;
		Node *node;
		Array *environment;
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

enum
{
	NUMBER, ATOM, TYPE, LIST, NODE,
	ENV, MAP, PTR, CTX,
	FORM,
	FREE = -1
};

struct listtag
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

extern List *append(List *const, List *const);
extern List *tipoff(List **const);
extern List *tip(const List *const);

// Копирование списка с заменой ссылок на узлы (NODE). Если nodemap - это NULL,
// то должно быть: (nodes == NULL && bound == 0). И в этом случае получается
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
extern char *listtostr(const Array *const universe, const List *const list);
extern void unidumplist(
	FILE *const, const Array *const universe, const List *const list);

// Функция forlist применяет другую функцию типа Oneach к каждому элементу
// списка, пока последняя возвращает значение, равное key. foreach устроена так,
// что позволяет менять ->next в обрабатываемом элементе списка.

typedef int (*Oneach)(List *const, void *const);
extern int forlist(List *const, Oneach, void *const, const int key);

extern unsigned listlen(const List *const);

// Плавный переход к окружениям. Вписывается ли список в линейный порядок

extern unsigned iscomparable(const List *const);

// Для создания нового стека окружений можно выполнить pushenvironment(NULL)

extern List *pushenvironment(List *const);
extern List *popenvironment(List *const);

extern void freeenvironment(List *const);

extern void dumpenvironment(
	FILE *const, const Array *const U, const List *const env);

typedef struct
{
	const List *const key;
	Ref ref;
} Binding;

extern Ref *keytoref(
	const List *const env, const List *const key, const unsigned depth);

extern Binding *keytobinding(
	const List *const env, const List *const key, const unsigned depth);

// Набор специальных функций, которые дополнительно декорируют ключи
// определёнными атомами, чтобы имена различных по назначению структур не
// перемешивались. Декорация осуществляется атомами, поэтому появляется
// дополнительный параметр - U

extern Ref *formkeytoref(
	Array *const U,
	const List *const env, const List *const key, const unsigned depth);

extern const Binding *topbindings(const List *const, unsigned *const length);

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

// Во многих нижеследующих процедурах используется два особых параметра,
// описывающих структуру обрабатываемого графа: map и go. Параметр map - это
// uimap, которая задаёт verb-ы узлов, в атрибутах которых записаны графы.
// Параметр go - это указание на verb-ы тех узлов с графами, в которые алгоритм
// должен рекурсивно спускаться. Если параметр go не указан, значит, алгоритм
// рекурсивно спускается в графы во всех узлах с ними.
// 
// Например, пройтись по всем графам в узлах, имена которых в массиве verbs:
// 	const Array map = keymap(U, 0, verbs); 
// 	walkdag(dag, &map, &map, walkone, ptr);
//	freemap((Array *)map);

// Загрузка dag-а. Атомы загружаются в universe. dagmap отмечает узлы, которые
// должны содержать другие dag-и в своих атрибутах. У каждого dag-а своя область
// видимости по ссылкам, они не могут явно ссылаться на узлы друг друга.

extern List *loaddag(
	FILE *const, Array *const universe, const Array *const map);

// Выгрузка dag-а. tabs - для красивой печати с отступами. 

extern void dumpdag(
	FILE *const, const unsigned tabs, const Array *const universe,
	const List *const dag, const Array *const map);

// Создать согласованную с таблицей атомов keymap по списку строк. Список строк,
// оканчивающийся NULL. В полученной uimap на i-том месте будет стоять номер
// атома, который описывается (hint; strlen(atoms(i)); atoms(i))

extern Array keymap(Array *const universe,
	const unsigned hint, const char *const atoms[]);

extern List *forkdag(const List *const dag, const Array *const map);
extern void freedag(List *const dag, const Array *const map);

// Сборка мусорных не корневых узлов. Не корневые узлы определяются
// uimap-отображением nonroots.

extern List *gcnodes(
	List **const dag, const Array *const map, const Array *const go,
	const Array *const nonroots);

typedef void (*WalkOne)(List *const, void *const);

extern void walkdag(
	const List *const dag, const Array *const map, const Array *const go,
	const WalkOne, void *const);

extern List *evallists(
	Array *const U,
	List **const dag, const Array *const map, const Array *const go,
	const List *const arguments);

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
	const Array *const map;
	unsigned count;
//	const unsigned goal;
};

// extern void freeform(Form *const f);
// 
// extern Form *newform(
// 	const List *const dag, const Array *const map,
// 	const List *const signature);

// Из-за сложной жизни форм (cf. txt/worklog.txt:2690 2013-09-04 11:55:50) имеет
// смысл во freeform передавать Ref и, соответственно, возвращать Ref из
// newform. Потому что они всегда связаны, получается: формы бывают либо в
// окружениях (там Ref-ы), либо в списках (тоже Ref-ы)

extern void freeform(const Ref);

extern Ref newform(
	const List *const dag, const Array *const map,
	const List *const signature);

extern void freeformlist(List *const forms);

// Структура контекста вывода

// struct contexttag
// {
// 	// Выведенная в этом контексте часть графа программы. Сюда дописывается
// 	// содержимое активированных форм
// 
// 	List *dag;
// 
// 	// Ссылки на уже выведенные в контексте узлы графа программы.
// 	// Environment из пар ключ (список из чисел, атомов, типов) : значение
// 	// (список с указателями на узлы)
// 
// 	List *outs;
// 
// 	// Размещённые в контексте формы, которые можно (потенциально)
// 	// активировать
// 
// 	List *forms;
// 	
// 	// Части сигнатур входов потенциально активных форм. Environment из пар
// 	// ключ : значение (ссылка на Form)
// 
// 	List *ins;
// 
// 	// Состояние контекста
// 	unsigned state;
// 
// 	// Атом - метка контекста для сопоставления с закрывающими E-узлами
// 	// синтаксиса. Не должно меняться
// 
// 	const unsigned marker;
// };

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

// 	struct reactor
// 	{
// 		List *outs;
// 		List *ins;
// 		List *forms;
// 	} R[2];

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

extern List *popcontext(List *const ctx, const Array *const map);

// Слияние двух контекстов на вершине стека. Тот, что сверху дописывается к
// тому, что снизу - это описание формирования порядка dag-ов

extern List *mergecontext(const Array *const U, List *const ctx);

extern unsigned isforwardempty(const List *const ctx);

// Выбирает из текущего графа формы и размещает их в соответствии с указаниями
// публикации: .FPut, .FGPut, .FEPut. Публикация осуществляется на вершинах
// двух указанных стеков: областей видимости и контекстов вывода.

// WARN: У формы есть ссылка на карту графа, эта ссылка будет взята из аргумента
// evalforms

extern void evalforms(
	Array *const universe,
	const List *const dag, const Array *const map, const Array *const go,
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
	const List *const dag, const Array *const map,
	const List *const signature, const unsigned external);

#endif
