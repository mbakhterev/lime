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
	// NUMBER, ATOM, TYPE: числа и номера значений в таблицах атомов и типов
	// соответственно.
	// 
	// NODE: выражения, которые представлены списками. Списки выражений
	// должны иметь формат ("some-verb" (attributes)). Они моделируют узлы в
	// DAG-е программы.
	// 
	// MAP: метка для обозначения окружений. MAP перечислена перед LIST,
	// значит, ссылки на MAP можно использовать в качестве элементов ключей
	// поиска по окружениям. Необходимо для определения типов в стиле Си
	// 
	// PTR: void-указатели, для различных служебных нужд
	// 
	// LIST: списки из разнообразных Ref-ов. Могут содержать Ref-ы на другие
	// списки. Поэтому используются для представления S-выражений.

	NUMBER, ATOM, TYPE, MAP, NODE, PTR, LIST,
	
	// FORM говорит об указателе на форму - кусочек DAG-а программы,
	// открытый для подстановки в него некоторых значений

	FORM,

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

extern Ref refkeymap(Array *const);

extern Ref refctx(Context *const);

extern Ref markext(const Ref);
extern Ref cleanext(const Ref);

// Иногда нужно поставить бит external, в зависимости от типа Ref-ы. Процедура
// для этого

extern Ref dynamark(const Ref);

// Ориентируясь на Ref.code вызвать соответствующее freexxx из доступных, если
// необходимо

extern void freeref(const Ref);

// Копирование отдельной Ref-ы. Ориентируясь по значению в Ref.code вызвать
// соответствующий forkxxx. Дополнительные параметры могут понадобится в одном
// из вариантов

extern Ref forkref(const Ref, Array *const map);

// Распечатка разных Ref-ов

extern void dumpref(
	FILE *const, const Array *const U, Array *const nodemap, const Ref);

extern char *strref(const Array *const U, Array *const nodemap, const Ref);

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

extern Array *newmap(
	const unsigned code,
	const unsigned itemlen, const ItemCmp, const KeyCmp);

extern void freemap(Array *const);

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

extern Ref readpack(Array *const, const AtomPack);
extern Ref loadatom(Array *const, FILE *const);

extern Ref loadtoken(
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

#define RS(...) ((const Ref[]) { __VA_ARGS__ })

// Конструктор списка из массива ссылок. Чтобы сконструировать список из одной
// ссылки нужно написать: list = readrefs(RS(refnum(NUMBER, 42)))

extern List *readrefs(const Ref refs[], const unsigned N);

// Для упрощения синтаксиса RL - Ref List

#define RL(...) (readrefs(RS(__VA_ARGS__), sizeof(RS(__VA_ARGS__))/sizeof(Ref)))

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
		.u.list = (List[sizeof(REFS)/sizeof(Ref)]) { { NULL } } \
	}; \
	formlist(DLNAME.u.list, REFS, sizeof(REFS)/sizeof(Ref))

// Записывает ссылки из списка (не рекурсивно, и не спускаясь в подсписки) в
// массив. Записывает не более N элементов.

extern unsigned writerefs(const List *const, Ref refs[], const unsigned N);

extern void freelist(List *const);

// Получить первый элемент списка или N-ный (счёт от 0)

extern List *tip(const List *const);
extern Ref listnth(const List *const, const unsigned N);

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

extern List *transforklist(const List *const, Array *const map);

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

extern char *strlist(const Array *const universe, const List *const);

extern void dumplist(
	FILE *const, const Array *const universe, const List *const);

// Базовый элемент конструкции окружений - локальное отображение выражений
// (ключей) в некоторые значения. Создаются пустыми. Привязывать к другим
// окружениям и формировать общий атлас (набор из map-ов) нужно явно

extern Array *newkeymap(void);

extern unsigned iskeymap(const Ref);

// Освобождение работает рекурсивно. Если в окружении есть другие окружения,
// в Ref-ах на которые не установлен external-бит, то они будут так же
// освобождены. Это произойдёт через freeref

extern void freekeymap(Array *const);

// Печать окружения keymap. Рекурсивная по тому же принципу, что и freekeymap

extern void dumpkeymap(
	FILE *const, const unsigned tabs, const Array *const U,
	const Array *const keymap);

// Информация хранится в keymap-ах в виде связок

typedef struct
{
	const Ref key;
	Ref ref;
} Binding;

// Из-за некоторых технических особенностей в управлении памятью нам надо
// различать добавление новой ячейки в отображение и поиск по некоторому ключу.
// Потому что ключи поиска могут быть собраны из разных кусочков и их нельзя
// просто брать и записывать в отображение. Для записи нужно эти ключи аккуратно
// формировать. Поэтому нужно разделить расширение отображение и поиск в нём. В
// следующей версии это не понадобится. Но пока две процедуры вместо одной
// (keymap): mapreadin - зачитывание нового ключа с выделением Binding под его
// связку; maplookup - поиск Binding по ключу.
// 
// Функции возвращают NULL в случае логического неуспеха: maplookup(map, key) ==
// NULL, если ничего не найдено; mapreadin(map, key) == NULL, если конфликт
// ключей.
// 
// maplookup(NULL, key) всегда равно NULL. mapreadin(NULL, key) - это assert.
// 
// mapreadin сохраняя ключ в отображении ничего с ним не делает, соответствующая
// Ref просто копируется. Например, чтобы передать дополнительно украшенный
// (decorate) ключ на хранение в соответствующую map (чтобы он был удалён вместе
// с ней), можно сделать так:
//
//	mapreadin(map, decorate(forkref(sourcekey), U, TYPE))
// 
// Или можно оставить на хранении прямо оригинал ключа:
// 
//	mapreadin(map, decorate(sourcekey), U, TYPE)

extern Binding *mapreadin(Array *const map, const Ref key);
extern const Binding *maplookup(const Array *const map, const Ref key);

// Достаточно часто требуется отыскать Binding по ключу, если соответствующая
// Binding в отображении есть. А если нет, то создать её и скопировать ключ в
// структуру отображения. Почему-то эта процедура называется bindkey

extern Binding *bindkey(Array *const map, const Ref key);

// Процедура декорирует ключи специальными атомами. Это нужно для удобства
// отладки (в основном). Дополнительный плюс, что различно декорированные ключи
// не будут смешиваться. Процедура decorate вернёт список из двух элементов,
// если code ей поддерживается: (deco-atom key). Бит external в Ref-е установлен
// не будет. В противном случае вылетит с assert-ом

extern Ref decorate(const Ref key, Array *const U, const unsigned code);

// Окружения

// Окружения привязываются друг к другу через именованные атомами ссылки,
// которые в самих окружениях и хранятся. Например, ссылка отмеченная атомом
// "parent" может указывать на окружение, содержащее данное. Особенность в том,
// что таких parent-ов может быть несколько по разным поводам. Поэтому ссылки
// разбиты на несколько видов, которые можно назвать путями. Пути различаются
// своими именами-атомами.

// makepath создаст цепочку окружений связанных по именам из списка names,
// действуя по правилам команды (mkdir -p) из мира POSIX: (1) если в текущем E
// окружении есть ссылка на окружение с текущем именем N на пути path, то
// перейти в окружение E.(path:N) и к имени N.next; (2) если такого окружения
// нет, то создать его, и перейти в него и к следующему имени.
//
// С последним именем N в списке makepath поступает особо. (1) Если в текущем
// (предпоследнем) окружении под именем зарегистрировано окружение, то должно
// быть (map.code == FREE), и makepath ничего не сделает. (2) Если в текущем
// окружении ничего под таким именем нет, то должно быт map.code == MAP, и в
// текущее окружение будет записана ссылка на окружение, задаваемое map.
// Параметр map - это Ref-а, чтобы можно было управлять external-битом.
//
// Вернёт makepath ссылку на последние из заданных списком имён окружений, если
// map и последнее имя согласованы. В случае несогласованности NULL

extern Array *makepath(
	Array *const env,
	Array *const U, const Ref path, const List *const names, const Ref map);

// В некотором смысле обратная операция: которая строит стек окружений,
// связанных по имени name на пути path, начиная с того, которое в окружении map
// задаётся именем 0.4."this". Это окружение и будет на вершине стека, в глубине
// будет то, в котором не найдётся следующего окружения по ключу (path name)

extern List *tracepath(
	const Array *const map, Array *const U, const Ref path, const Ref name);

// Вдоль таких списков мы тоже умеем искать Binding-и. По адресу
// depth, если он не NULL запишется глубина (вершина стека на глубине 0), на
// которой найдена соответствующая Binding. Если Binding с нужным ключём не
// будет найдена, то pathlookup вернёт NULL

extern const Binding *pathlookup(
	const List *const stack, const Ref key, unsigned *const depth);

// В некоторых случаях необходима уверенность в том, что ключ состоит только из
// элементов определённого типа элементов:
// 
// 	basic - { NUMBER, ATOM };
// 	signature - { NUMBER, ATOM, TYPE };
//	type - { NUMBER, ATOM, TYPE, TYPEVAR }.

extern unsigned isbasickey(const Ref);
extern unsigned issignaturekey(const Ref);
extern unsigned istypekey(const Ref);

// Процедура для сопоставления ключей. Рекурсивно идёт по Ref-ам в ключах
// pattern и k, требуя их полного равенства (external-бит не учитывается) за
// исключением случая, когда в очередной Ref ключа pattern (code == FREE).
// (Ref.code == FREE) считается несвязанным местом в выражении, и в него может
// быть подставлено выражение произвольное. Такой вот примитивный алгоритм
// унификации. Указатели на не более чем N первых Ref из l, которые
// соответствуют первым позициям с (Ref.code == FREE) в pattern будут записаны с
// массив unifiers. Если (pmatched != NULL), то по этому адресу будет записано
// реальное число совпадений

extern unsigned keymatch(
	const Ref pattern, const Ref *const l,
	const Ref *unifiers[], const unsigned N, unsigned *const pmatched);

// Для упрощения синтаксиса уместно иметь процедуры для работы с задаваемыми
// keymap-ами отображениями. Они создаются при помощи newkeymap. В каждое
// отображение можно добавить информацию вызовом соответствующей tune-процедуры.
// Соответствующая map-процедура работает как применение отображение к
// аргументу. Добавлять в отображение можно только ту информацию, которой в нём
// ещё не было, иначе сработает assert. NULL-евое значение для параметра map
// процедуры трактуют как пустое отображение

// Отображение Ref -> Ref. Если отображение не знает про соответствующий
// аргумент, оно вернёт reffree

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

// Аналогичное ptrmap отображение Ref -> Array.ptr

extern void tuneenvmap(Array *const map, const Ref key, Array *const ptr);
extern Array *envmap(const Array *const map, const Ref key);

// Отображения unsigned -> unsigned. Основное предназначение: осмысливание
// разных verb-ов выражений в разных контекстах. Чаще всего оно наполняется
// информацией по списку строчек, которые переводятся в атомы. Поэтому для него
// свой конструктор.
// 
// Если отображение не знает о своём аргументе, оно возвращает -1

extern Array *newverbmap(
	Array *const U, const unsigned hint, const char *const atoms[]);

extern unsigned verbmap(const Array *const, const unsigned verb);

// Процедура, которая строит отображение с информацией о порядковых номерах
// Ref-ов в этом отображении. Специально его настраивать не нужно, оно всегда
// возвращает номер Ref-ы в указанном отображении, которое не должно быть
// NULL-ём.

extern unsigned enummap(Array *const map, const Ref key);

// Аналогичная по семантике enummap процедура, но работающая только для типов.
// Ключ должен быть таким, что istypekey(key) истина. При необходимости, ключ
// копируется в отображение

extern unsigned typeenummap(Array *const map, const Ref key);

// Процедура прохода по окружениям. Начинает с map и идёт по Binding-ам (на
// всякий случай в порядке индекса). Когда walkbindings видит такую связку B,
// что (B.ref.code == MAP && !B.ref.external), и когда WalkBinding для B вернёт
// !0, процедура будет вызвана рекурсивно для B.ref.u.array

typedef int WalkBinding(Binding *const, void *const ptr);
extern void walkbindings(Array *const map, WalkBinding, void *const);

// Узлы.

// Узлы представлены выражениями вида (hh.l."some verb atom" attribute ...).
// Необходимо уметь их распаковывать. Параметр exp - само выражение для узла.
// Его удобнее сделать Ref-ой, потому что узлы в одиночку почти не ходят.
// Параметр vm (verbmap), если не NULL, задаёт отображение оригинального verb-а
// узла в некоторое числовое значение. Почти всегда на verb-ы нужно смотреть
// через такое отображение. Если vm == NULL, то verb возвращается так, как есть:
// (vm != NULL -> vm verb : verb)

extern unsigned nodeverb(const Ref exp, const Array *const vm);
extern unsigned nodeline(const Ref exp);
extern Ref nodeattribute(const Ref exp);
extern const Ref *nodeattributecell(const Ref exp);

extern unsigned knownverb(const Ref exp, const Array *const verbs);

// Конструирование узла. Процедура вернёт Ref-у со сброшенным external-битом,
// что будет трактоваться как ссылка на определение узла, а не просто ссылка на
// узел (случай (Ref.code == NODE && Ref.external))

extern Ref newnode(
	const unsigned verb, const Ref attribute, const unsigned line);

// Проверка структуры списка на то, что она действительно задаёт выражение

extern unsigned isnode(const Ref);
extern unsigned isnodelist(const List *const);

// Процедура для копирования выражений (узлов). Для воспроизведения ссылок на
// узлы (подвыражения), ей нужно отображение прежних узлов на новые

extern Ref forknode(const Ref, Array *const nodemap);

// DAG-и. Устроены как списки узлов в атрибутах которых бывают ссылки на другие
// узлы

// Некоторые нижеследующие процедуры содержат параметр map, который является
// verbmap-ом. Отображение описывает особые выражения. Особым выражением
// считается список, описывающий узел N: (verbmap map (nodeverb N) != -1).

// Загрузка dag-а. Атомы загружаются в U. Особые выражения в данном случае - это
// узлы, в атрибутах которых должен быть записан замкнутый граф

extern Ref loaddag(
	FILE *const, Array *const U, Array *const map);

// Выгрузка dag-а. tabs - для красивой печати с отступами. dbg - выдавать ли
// указатели на узлы (для закрепления: на списки особого формата) в выводе
// графов (это нужно для отладки). Особые узлы трактуются так же, как в loaddag

extern void dumpdag(
	const unsigned dbg, FILE *const, const unsigned tabs,
	const Array *const U, const Ref dag, Array *const map);

extern Ref forkdag(const Ref dag);

// Сборка мусорных не корневых узлов. Не корневые узлы определяются
// verbmap-ой nonroots. После обработки структура графа поменяется,

extern void gcnodes(
	Ref *const dag, Array *const map,
	Array *const nonroots, Array *const marks);

// Процедура для записи меток в setmap, которую можно потом передать в качестве
// marks в gcnodes

extern void collectmarks(Array *const marks, const Ref, Array *const nonroots);

// Узел передаётся в WalkOne и в разобранном виде тоже (разбор почти всегда
// необходим, поэтому разумно для упрощения работы). На атрибут передаётся
// ссылка, потому что в некоторых случаях WalkOne будет его переписывать

typedef int WalkOne(
	const Ref node,
	const unsigned verb, const Ref *const attribute, void *const);

// Процедура walkdag проходит по объявлениям выражений
// 
// 	(Ref.code == NODE && !Ref.external)
// 
// в графе (ну, граф условный у нас теперь). Для каждого такого объявления
// вызывается процедура wlk, в которую выражение передаётся в разобранном виде.
// Если wlk возвращает истину, то walkdag рекурсивно повторяется для атрибутов
// этого узла. Параметр vm - это verbmap, через которое будут транслироваться
// verb-узлов пере вызовом wlk

extern void walkdag(const Ref dag, WalkOne wlk, void *const, Array *const vm);

// Процедура для конструирования по графу dag. Обращает внимание на .E и .Env
// узлы. Результатами работы будет (1) дерево окружений, задаваемое .E-узлами,
// построенное из корня в env; (2) семантика, записанное в ptrmap-отображение
// envmarks, в котором узлам с verb-ами из markit будет сопоставлено то
// окружение, где они объявлены (!Ref.external). enveval можно попросить не
// трогать подвыражения в атрибутах узлов с verb-ами из escape (нужно для форм) 

extern void enveval(
	Array *const U,
	Array *const env,
	Array *const envmarks,
	const Ref dag, const Array *const escape, const Array *const markit);

// Процедура переписи выражения в другое с учётом накопленной информации о
// значениях узлов. Ссылки на узлы подменяются на значения для них в map.
// Подменяются только (Ref.code == NODE && Ref.external), verb-ы которых
// определены в verbs. Детали работы для ссылки n:
// 
// 1.
// 	Если (map n).code == FREE, то ничего не происходит и ссылка копируется в
// 	целевое выражение.
// 
// 2.
// 	В других случаях ссылка пропускается через forkref. Процедура forkref
// 	работает с таблицей соответствий новых и старых узлов. Эта таблица будет
// 	создана в начале работы expeval.
// 
// 3.
// 	Если (map n).code == LIST, то список, полученный при помощи forkref
// 	будет дописан прямо в формируемое expeval выражение. Это не особо
// 	приятная, но необходимая в текущей версии деталь. В следующей версии
// 	необходимости делать это не будет

extern Ref exprewrite(const Ref exp, const Array *const map, const Array *const verbs);

extern void typeeval(
	Array *const U,
	Array *const types,
	Array *const typemarks,
	const Ref dag, const Array *const escape, const Array *const envmarks);

extern const Binding *typeat(const Array *const, const unsigned);

extern void symeval(
	Array *const U,
	Array *const symmarks,
	const Ref dag, const Array *const escape,
	const Array *const envmarks, const Array *const typemarks);

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
