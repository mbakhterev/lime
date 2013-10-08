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
	// отмечают и соответствующие Array-и. Из этих значений состоят
	// выражения.
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

	// Метка для обозначения окружений
	MAP,

	// Метки, которые в будущем не понадобятся
	CTX,

	// Свободная ссылка, в которой ничего нет
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

// Копирование отдельной Ref-ы. Ориентируясь по значению в Ref.code вызвать
// соответствующий forkxxx

extern Ref forkref(const Ref);

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

// Для упрощения синтаксиса DL - Define List (не особо удачное название). Чаще
// всего DL требуется для конструирования выражений, поэтому сразу возвращает
// Ref-у

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

// Различные варианты копирования списков

// Самый простой вариант. Все ссылки будут повторены, под-списки будут
// скопированы, если для них не установлен external-бит

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

// Базовый элемент конструкции окружений - локальное отображение выражений
// (ключей) в некоторые значения. Создаются пустыми. Привязывать к другим
// окружениям и формировать общий атлас (набор из map-ов) нужно явно

extern Array *newkeymap(void);

// Освобождение работает рекурсивно. Если в окружении есть другие окружения,
// в Ref-ах на которые не установлен external-бит, то они будут так же
// освобождены. Это произойдёт через freeref

extern void freekeymap(Array *const);

// Печать окружения keymap. Рекурсивная по тому же принципу, что и freekeymap

extern void dumpkeymap(
	FILE *const, const unsigned tabs, const Array *const U,
	const Array *const keymap);

// Запись в отображение новой информации (процедура tunekeymap) и её поиск
// (keymap). Информация хранится в виде связок

typedef struct
{
	const Ref key;
	Ref ref;
} Binding;

// Процедуры возвращают указатели на созданную Binding или на искомую.
// tunekeymap ожидает (через assert), что повторений ключей в отображении быть
// не должно. keymap вернёт NULL, если ничего не найдёт. Параметр decoration
// задаёт формирование дополнительной структуры для ключа (чтобы различать
// окружения, символы, формы, типы). Если он будет равен FREE, никакой
// трансформации с ключом не случится. Процедура tunekeymap просто скопирует
// Ref-ы key и ref в структуру Binding. За правильной установкой external-бита и
// копированием структур данных должна следить вызывающая процедура

extern Binding *tunekeymap(
	Array *const map,
	const Ref key, const unsigned decoration, const Ref ref);

extern Binding *keymap(
	Array *const map,
	const Ref key, const unsigned decoration);

// Окружения привязываются друг к другу через именованные атомами ссылки,
// которые в самих окружениях и хранятся. Например, ссылка отмеченная атомом
// "parent" может указывать на окружение, содержащее данное. Особенность в том,
// что таких parent-ов может быть несколько по разным поводам. Поэтому ссылки
// разбиты на несколько видов, которые можно назвать путями. Пути различаются
// своими именами-атомами. Поэтому способ порождения структуры из окружений
// вдоль определённого пути таков, какой он есть

// makepath создаст цепочку окружений связанных по именам из списка names,
// действуя по правилам команды (mkdir -p) из мира POSIX: (1) если в текущем E
// окружении есть ссылка на окружение с текущем именем N на пути path, то
// перейти в окружение E.(path:N) и к имени N.next; (2) если такого окружения
// нет, то создать его, и перейти в него и к следующему имени.
//
// С именем N в списке makepath поступает особо. (1) Если в текущем
// (предпоследнем) окружении под именем зарегистрировано окружение,
// то должно быть map.code == FREE, и makepath ничего не сделает. (2) Если в
// текущем окружении ничего под таким именем нет, то должно быт map.code == MAP,
// и в текущее окружение будет записана ссылка на окружение, задаваемое map. Для
// этой ссылки будет принудительно установлен флаг external.
// 
// Вернёт makepath ссылку на последние из заданных списком окружений

extern Array *makepath(
	const Array *const map,
	const Ref path, const List *const names, const Ref newmap);

// В некотором смысле обратная операция: которая строит стек окружений,
// связанных по имени name на пути path, начиная с того, которое в окружении map
// задаётся именем 0.4."this". Это окружение и будет на вершине стека, в глубине
// будет то, в котором не найдётся следующего окружения по ключу (path name)

extern List *tracepath(
	const Array *const map, const Ref path, const Ref name);

// Вдоль таких списков мы тоже умеем искать. В по адресу depth, если он не NULL
// запишется глубина (вершина стека на глубине 0), на которой найдена
// соответствующая Binding

extern Binding *pathkeymap(
	const List *const stack, const Ref key, unsigned *const depth);

// В некоторых случаях необходима уверенность в том, что ключ состоит только из
// { NUMBER, ATOM, TYPE } элементов. Это позволяет проверить процедура

extern unsigned isbasickey(const Ref);

// Для упрощения синтаксиса процедуры для работы с задаваемыми keymap-ами
// отображениями. Они создаются при помощи newkeymap. В каждое отображение можно
// добавить информацию вызовом соответствующей tune-процедуры. Соответствующая
// map-процедура работает как применение отображение к аргументу. Добавлять в
// отображение можно только ту информацию, которой в нём ещё не было, иначе
// сработает assert. NULL-евое значение для параметра map процедуры трактуют как
// пустое отображение

// Отображение Ref -> Ref. Без выставления деталей о декорациях и Binding-ах
// наружу. Если отображение не знает про соответствующий аргумент, оно вернёт
// reffree

extern void tunerefmap(Array *const map, const Ref key, const Ref val);
extern Ref refmap(const Array *const map, const Ref key);

// Отображение Ref -> (set 0 1). То есть, характеристическое для множества. Если
// отображение не знает о соответствующем аргументе, оно возвращает 0

extern void tunesetmap(Array *const map, const Ref key);
extern unsigned setmap(const Array *const map, const Ref key);

// Отображение Ref -> void.ptr. Если отображение не знает об аргументе, оно
// возвращает NULL

extern void tuneptrmap(Array *const map, const Ref key, void *const ptr);
extern void *ptrmap(const Array *const map, const Ref key);

// Отображения unsigned -> unsigned. Основное предназначение: осмысливание
// разных verb-ов выражений в разных контекстах. Чаще всего оно наполняется
// информацией по списку строчек, которые переводятся в атомы. Поэтому для него
// свой конструктор.
// 
// Если отображение не знает о своём аргументе, оно возвращает -1

extern Array *newverbmap(
	Array *const U, const unsigned hint, const char *const atoms[]);

extern unsigned verbmap(const Array *const, const unsigned verb);

// Процедура прохода по окружениям. Идёт в ширину от this-окружения в пути path
// по (Ref.external != 1) ссылкам на другие окружения. Для каждой Binding
// вызывает соответствующую процедуру

typedef void WalkBinding(
	const Array *const map, const Binding *const bnd, void *const ptr);

typedef void walkbindings(
	const Array *const map, const Ref path, WalkBinding, void *const);

// Узлы.

// Узлы представлены выражениями вида (hh.l."some verb atom" attribute ...).
// Необходимо уметь их распаковывать. Параметр exp - само выражение для узла.
// Его удобнее сделать Ref-ой, потому что узлы в одиночку почти не ходят.
// Параметр vm (verbmap), если не NULL, задаёт отображение оригинального verb-а
// узла в некоторое числовое значение. Почти всегда на verb-ы нужно смотреть
// через такое отображение. Если vm == NULL, то verb возвращается так, как есть:
// (vm != NULL -> vm verb : verb)

extern unsigned nodeverb(const Ref exp, const List *const vm);
extern Ref nodeattribute(const Ref exp);
extern unsigned nodeline(const Ref exp);

// Конструирование узла. Процедура вернёт Ref-у со сброшенным external-битом,
// что будет трактоваться как ссылка на определение узла, а не просто ссылка на
// узел (случай (Ref.code == NODE && Ref.external))

extern Ref newnode(
	const unsigned verb, const Ref attribute, const unsigned line);

// Проверка структуры списка на то, что она действительно задаёт выражение
extern unsigned isnode(const Ref);
extern unsigned isnodelist(const List *const);

// Процедура для копирования выражений (узлов)
extern Ref forknode(const Ref);

// DAG-и. Устроены как списки узлов в атрибутах которых бывают ссылки на другие
// узлы

// Некоторые нижеследующие процедуры содержат параметр map, который является
// verbmap-ом. Отображение описывает особые выражения. Особым выражением
// считается список, описывающий узел N: (verbmap map (nodeverb N) != -1).

// Загрузка dag-а. Атомы загружаются в U. Особые выражения в данном случае - это
// узлы, в атрибутах которых должен быть записан замкнутый граф

extern Ref loaddag(FILE *const, Array *const U, const List *const map);

// Выгрузка dag-а. tabs - для красивой печати с отступами. dbg - выдавать ли
// указатели на узлы (для закрепления: на списки особого формата) в выводе
// графов (это нужно для отладки). Особые узлы трактуются так же, как в loaddag

extern void dumpdag(
	const unsigned dbg, FILE *const, const unsigned tabs,
	const Array *const U, const Ref dag, const List *const map);

extern Ref forkdag(const Ref dag);

extern void freedag(const Ref dag);

// Сборка мусорных не корневых узлов. Не корневые узлы определяются
// verbmap-ой. После обработки структура графа поменяется, но это затронет
// только атрибуты выражения dag. Поэтому Ref-а на узел с графом может быть
// константой

extern void gcnodes(
	const Ref dag, const List *const map,
	const List *const nonroots, const List *const marks);

// Узел передаётся в WalkOne в разобранном виде. На атрибут передаётся ссылка,
// потому что в некоторых случаях walkone будет его переписывать

typedef int WalkOne(
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
	const Ref dag, WalkOne, void *const, const List *const verbmap);

// Оценка узлов L, LNth и FIn. Параметр map описывает те выражения, в которых
// оценку проводить не следует

extern List *evallists(
	Array *const U,
	const Ref dag, const Array *const map, const List *const arguments);

// Форма. У неё есть сигнатура, определяющая способ встраивания формы в текущий
// выводимый граф и dag с описанием тела формы. Счётчик необходим для
// отслеживания процесса активации формы. Форма активируется, когда в контексте
// вывода появляется необходимое количество выходов, соответствующих её
// сигнатуре

struct Form 
{
	union
	{
		const Ref dag;
		struct Form *nextfree;
	} u;

	const List *const signature;

	unsigned count;
};

// Из-за сложной жизни форм (cf. txt/worklog.txt:2690 2013-09-04 11:55:50) имеет
// смысл во freeform передавать Ref и, соответственно, возвращать Ref из
// newform. Потому что они всегда связаны: формы бывают либо в окружениях (там
// Ref-ы), либо в списках (тоже Ref-ы)

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
