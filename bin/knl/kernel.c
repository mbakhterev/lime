#include <lime/construct.h>
#include <lime/util.h>

#include <assert.h>

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

#define DBGMAIN		1
#define DBGMAINEX	2
#define DBGINIT		4
#define DBGTYPES	8
#define DBGPRD		16

// #define DBGFLAGS (DBGMAIN | DBGMAINEX | DBGINIT)
// #define DBGFLAGS (DBGMAIN | DBGMAINEX)
// #define DBGFLAGS (DBGMAIN | DBGINIT)
// #define DBGFLAGS (DBGMAIN | DBGPRD)

#define DBGFLAGS 0

unsigned item = 1;
const char *unitname = "stdin";

// Загрузка исходных форм в окружение и контекст (?)

const char *const stdmap[] =
{
	"F", "LB", NULL
};

static void initone(
	const char *const optarg,
	Array *const U, Array *const env, Array *const types)
{
	unitname = optarg;
	item = 1;
	FILE *const f = fopen(unitname, "r");
	if(!f)
	{
		ERR("can't open form-source: %s", unitname);
	}

	Array *const subdags = newverbmap(U, 0, stdmap);
	const Ref rawdag = loaddag(f, U, subdags);
	freekeymap(subdags);
	fclose(f);
	
	if(DBGFLAGS & DBGINIT)
	{
		dumpdag(1, stderr, 0, U, rawdag);
		assert(fputc('\n', stderr) == '\n');
	}

	Array *const escape = newverbmap(U, 0, ES("F"));

	const Ref dag = leval(U, rawdag, escape);
	freeref(rawdag);

	Array *const envmarks = newkeymap();
	Array *const typemarks = newkeymap();

	Array *const tomark = newverbmap(U, 0, ES("FEnv", "TEnv"));
	enveval(U, env, envmarks, dag, escape, tomark);
	freekeymap(tomark);

	if(DBGFLAGS & DBGINIT)
	{
		dumpkeymap(1, stderr, 0, U, envmarks, NULL);
		assert(fputc('\n', stderr) == '\n');
	}

	typeeval(U, types, typemarks, dag, escape, envmarks);
	formeval(U, NULL, dag, escape, envmarks, NULL, typemarks);

	if(DBGFLAGS & DBGINIT)
	{
		dumptable(stderr, 0, U, types);
		assert(fputc('\n', stderr) == '\n');
	}

	freekeymap(escape);
	freekeymap(typemarks);
	freekeymap(envmarks);

	freeref(dag);
}

static void initforms(
	int argc, char *const argv[],
	Array *const U, Array *const env, Array *const types)
{
	optind = 1;
	int opt = 0;
	while((opt = getopt(argc, argv, "df:")) != -1)
	{
		switch(opt)
		{
		case 'f':
			initone(optarg, U, env, types);
			break;

		case 'd':
			break;
		
		default:
			ERR("%s", "usage: [-d] [-f form-source]+");
		}
	}
}

static unsigned initinfodump(const int argc, char *const argv[])
{
	optind = 1;
	int opt = 0;
	unsigned dump = 0;
	while((opt = getopt(argc, argv, "df:")) != -1)
	{
		switch(opt)
		{
		case 'd':
			if(!dump)
			{
				dump = 1;
			}
			else
			{
				ERR("%s", "usage: [-d] [-f form-source]+");
			}

			break;
		
		case 'f':
			break;

		default:
			ERR("%s", "usage: [-d] [-f form-source]+");
		}
	}

	return dump;
}

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
	const unsigned atom = loadatom(U, f).u.number;

	return (SyntaxNode)
	{
		.op = op,
		.atom = atom,
		.pos = pos
	};
}

// Тут read в форме past perfect

static void progressread(FILE *const f, Core *const C)
{
	while(1)
	{
		const SyntaxNode sntx = readone(f, C->U);

		if(sntx.op != EOF)
		{
			ignite(C, sntx);
			DBG(DBGPRD, "%s", "ignited");

			if(C->dumpinfopath)
			{
				const Array *const escape
					= stdstackupstreams(C->U);
				fprintf(stderr,
					"\nNEXT STACK PROGRESSION");
				dumpareastack(0, stderr, 0,
					C->U, C->areastack, escape);
				freekeymap((Array *)escape);
// 				assert(fputc('\n', stderr) == '\n');
			}

			progress(C);
			DBG(DBGPRD, "%s", "progressed");
		}
		else
		{
			break;
		}
	}
}

static Array *const inituniverse(void)
{
	Array *const U = newatomtab();

	// Прогружаем таблицу атомов начальными значениями. Они системные,
	// поэтому hint = 0

	for(unsigned i = 0; syntaxops[i]; i += 1)
	{
		assert(readpack(U, strpack(0, syntaxops[i])).u.number == i);
	}

	return U;
}

static Array *const inittypes(Array *const U)
{
	Array *const T = newkeymap();
	return T;
}

static Array *const initroot(Array *const U)
{
	Array *const R = newkeymap();

	assert(linkmap(U, R,
		readtoken(U, "ENV"), readtoken(U, "this"), refkeymap(R)) == R);

	return R;
}

int main(int argc, char *const argv[])
{
	const unsigned dip = initinfodump(argc, argv);
	Array *const U = inituniverse();
	Array *const R = initroot(U);
	Array *const T = inittypes(U);

	initforms(argc, argv, U, R, T);
	DBG(DBGMAIN, "%s", "forms loaded");

	if(DBGFLAGS & DBGMAIN)
	{
		const Array *const esc = stdupstreams(U);
		DBG(DBGMAIN, "env: count = %u", R->count);
		dumpkeymap(1, stderr, 0, U, R, esc);
		freekeymap((Array *)esc);

		DBG(DBGMAIN, "%s", "types:");
		dumptable(stderr, 0, U, T);
	}

	Core C =
	{
		.U = U,
		.types = T,
		.typemarks = newkeymap(),
		.symbols = newkeymap(),
		.symmarks = newkeymap(),
		.root = R,
		.envtogo = R,
		.areastack = NULL,
		.activity = newkeymap(),
		.dumpinfopath = dip
	};

	// Основной цикл вывода графа программы. Чтение с stdin. Если вывод не
	// удался, то, всё равно, выдаём накопленный в "придонном" контексте
	// граф. Потому что, пока cfe не отлажен корректный вывод не получится

	unsigned ok = 0;
	if(CKPT() == 0)
	{
		item = 1;
		unitname = "stdin";

		progressread(stdin, &C);
		ok = 1;
		checkout(0);
	}
	else
	{
		fprintf(stderr, "progression error. Dumping DAG on stderr\n");
	}

	const Ref A = C.areastack->ref;
	assert(isarea(A));
	dumpdag(0, ok ? stdout : stderr, 0, U, *areadag(U, A.u.array));
	assert(fputc('\n', ok ? stdout : stderr) == '\n');

	freekeymap(C.activity);
	freekeymap(C.symmarks);
	freekeymap(C.symbols);
	freekeymap(C.typemarks);
	freekeymap(T);
	freekeymap(R);
	freeatomtab(U);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
