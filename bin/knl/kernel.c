#include <lime/construct.h>
#include <lime/util.h>

#include <assert.h>

#include <stdio.h>
#include <unistd.h>

#define DBGMAIN 1
#define DBGMAINEX 2
#define DBGFLAGS (DBGMAIN | DBGMAINEX)

unsigned item = 1;
const char *unitname = "stdin";

// Загрузка исходных форм в окружение и контекст (?)

void *initforms(
	int argc, char *const argv[],
	Array *const U, const Array *const map,
	const List *const env, const List *const ctx)
{
	optind = 1;
	int opt = 0;
	while((opt = getopt(argc, argv, "f:")) != -1)
	{
		switch(opt)
		{
		case 'f':
		{
			unitname = optarg;
			item = 1;
			FILE *const f = fopen(unitname, "r");
			if(!f)
			{
				ERR("can't open form-source: %s", unitname);
			}

			// Параметр go = NULL, в под-графы не ходим, собираем
			// формы только из верхнего уровня

			evalforms(U, loaddag(f, U, map), map, NULL, env, ctx);
			
			break;
		}

		default:
			ERR("%s", "usage: [-f form-source]+");
		}
	}

	return NULL;
}

const char *const stdmap[] =
{
	"F", "LB", NULL
};

// Команды от синтаксического фасада. Их тоже необходимо загружать в виде
// атомов, чтобы был возможен поиск формы по её имени (ключу)

#define AOP	0
#define UOP	1
#define BOP	2
#define LOP	3
#define EOP	4
#define FOP	5

const char *const syntaxops[] =
{
	[AOP] = "A",
	[UOP] = "U",
	[BOP] = "B",
	[LOP] = "L",
	[EOP] = "E",
	[FOP] = "F",
	NULL
};

// Чтение синтаксиса и выстраива

// static void readandcode(FILE *const f)
// {
// 	
// 	while(skipspaces(f) != EOF)
// 	{
// 		
// 	}
// }

int main(int argc, char *const argv[])
{
	Array U = makeatomtab();
	
	// Прогружаем таблицу атомов начальными значениями. Они системные,
	// поэтому hint = 0

	for(unsigned i = 0; syntaxops[i]; i += 1)
	{
		assert(readpack(&U, strpack(0, syntaxops[i])) == i);
	}

	// Загрузка начальных форм

	const Array map = keymap(&U, 0, stdmap);

	List *env = pushenvironment(NULL);
	List *ctx = pushcontext(NULL);

	initforms(argc, argv, &U, &map, env, ctx);

	DBG(DBGMAIN, "%s", "forms loaded");

	if(DBGFLAGS & DBGMAINEX)
	{
		dumpenvironment(stderr, &U, env);
	}

	// Основной цикл вывода графа программы. Чтение с stdin

	FILE *const f = stdin;

	while(skipspaces(f) != EOF)
	{
		
	}

	return 0;
}
