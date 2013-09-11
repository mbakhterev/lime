#include <lime/construct.h>
#include <lime/util.h>

#include <assert.h>

#include <stdio.h>
#include <ctype.h>
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

// 			evalforms(U, loaddag(f, U, map), map, NULL, env, ctx);
			List *const g = loaddag(f, U, map);
			evalforms(U, g, map, NULL, env, ctx);
			freedag(g, map);
			
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

const char *const syntaxops[] =
{
	[FOP] = "F",
	[AOP] = "A",
	[UOP] = "U",
	[LOP] = "L",
	[BOP] = "B",
	[EOP] = "E",
	NULL
};

// FIXME: пока ничего не делается с F

static unsigned opdecode(int c)
{
	switch(c)
	{
	case 'A': return AOP;
	case 'U': return UOP;
	case 'L': return LOP;
	case 'B': return BOP;
	case 'E': return EOP;
	}

	return -1;
}

static Position notapos(void)
{
	return (Position) { .file = -1, .line = -1, .column = -1 };
}

static SyntaxNode syntaxend(void)
{
	return (SyntaxNode)
	{
		.op = EOF,
		.atom = -1,
		.pos = notapos()
	};
}

static unsigned isgoodpos(Position p)
{
	if(p.file == -1 || p.line == -1 || p.column == -1)
	{
		assert(p.file == -1 && p.line == -1 && p.column == -1);
		return 0;
	}

	return 1;
}

static Position readposition(FILE *const f)
{
	unsigned line;
	unsigned col;
	
	if(fscanf(f, "%u.%u", &line, &col) == 2
		&& line < MAXNUM && col < MAXNUM)
	{
		return (Position)
		{
			.line = line,
			.column = col,
			.file = 0 // FIXME
		};
	}

	return notapos();
}

static SyntaxNode readone(FILE *const f, Array *const U)
{
	int c = -1;
	
	if((c = skipspaces(f)) == EOF)
	{
		return syntaxend();
	}

	// Декодируем op-код синтаксиса

	const unsigned op = opdecode(c);
	if(op == -1)
	{
		ERR("wrong syntax node op: %c", c);
	}

	// Зачитываем координаты. skipspaces нужна, чтобы учесть переходы на
	// новую строку.
	
	// FIXME: Вообще, красивым решением было бы написать свои процедуры
	// чтения текстовых файлов, где бы автоматически считалась позиция во
	// вводе, но пока на это нет времени. Поэтому действуем топорно

	if(!isdigit(c = skipspaces(f)))
	{
		ERR("%s", "can't get syntax node position");
	}

	assert(ungetc(c, f) == c);
	const Position pos = readposition(f);
	if(!isgoodpos(pos))
	{
		ERR("%s", "can't get syntax node position");
	}

	// Сам атом. Должен начинаться с шестнадцатеричного hint

	if(!isxdigit(c = skipspaces(f)))
	{
		ERR("%s", "can't get syntax node atom");
	}

	assert(ungetc(c, f) == c);
	const unsigned atom = loadatom(U, f);

	return (SyntaxNode)
	{
		.op = op,
		.atom = atom,
		.pos = pos
	};
}

// Тут read в форме past perfect

static void progressread(
	FILE *const f,
	Array *const U, const List **const env, const List **const ctx)
{
// Workaround странного (?) поведения GCC
// 
// 	SyntaxNode sntx;
// 
// 
// 	while((sntx = readone(f, U)).op != EOF)
// 	{
// 		progress(U, env, ctx, sntx);
// 	}

	while(1)
	{
		const SyntaxNode sntx = readone(f, U);
		if(sntx.op != EOF)
		{
			progress(U, env, ctx, sntx);
		}
		else
		{
			break;
		}
	}
}

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

	const List *env = pushenvironment(NULL);
	const List *ctx = NULL;

// 	const List *ctx = pushcontext(NULL);

	initforms(argc, argv, &U, &map, env, ctx);

	DBG(DBGMAIN, "%s", "forms loaded");

	if(DBGFLAGS & DBGMAINEX)
	{
		dumpenvironment(stderr, 0, &U, env);
	}

	// Основной цикл вывода графа программы. Чтение с stdin. Если вывод не
	// удался, то, всё равно, выдаём накопленный в "придонном" контексте
	// граф. Потому что, пока cfe не отлажен корректный вывод не получится

	if(CKPT() == 0)
	{
		item = 1;
		unitname = "stdin";
		progressread(stdin, &U, &env, &ctx);

		checkout(0);
	}
	else
	{
		DBG(DBGMAIN, "%s", "progression error; dumping result anyway");
	}

	assert(!therearepoints());
	assert(ctx && ctx->ref.code == CTX && ctx->ref.u.context);

	dumpcontext(stderr, &U, ctx);

	dumpdag(0, stdout, 0, &U, ctx->ref.u.context->dag, &map);
	fprintf(stdout, "\n");

	return 0;
}
