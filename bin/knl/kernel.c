#include <lime/construct.h>
#include <lime/util.h>

#include <assert.h>

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#define DBGMAIN 1
#define DBGMAINEX 2
#define DBGINIT 4
#define DBGTYPES 8

#define DBGFLAGS (DBGMAIN | DBGMAINEX | DBGINIT)

unsigned item = 1;
const char *unitname = "stdin";

// Загрузка исходных форм в окружение и контекст (?)

const char *const stdmap[] =
{
	"F", "LB", NULL
};

static void initforms(
	int argc, char *const argv[],
	Array *const U, Array *const env, Array *const types)
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

			Array *const subdags = newverbmap(U, 0, stdmap);
			const Ref dag = loaddag(f, U, subdags);
			
			if(DBGFLAGS & DBGINIT)
			{
				dumpdag(1, stderr, 0, U, dag, subdags);
				assert(fputc('\n', stderr) == '\n');
			}

			freekeymap(subdags);
			fclose(f);

			Array *const envmarks = newkeymap();
			Array *const typemarks = newkeymap();

			Array *const escape = newverbmap(U, 0, ES("F"));

			Array *const tomark
				= newverbmap(U, 0, ES("FEnv", "TEnv"));
			enveval(U, env, envmarks, dag, escape, tomark);
			freekeymap(tomark);

			if(DBGFLAGS & DBGINIT)
			{
				dumpkeymap(1, stderr, 0, U, envmarks);
				assert(fputc('\n', stderr) == '\n');
			}

			typeeval(U, types, typemarks, dag, escape, envmarks);
			formeval(U, NULL, dag, escape, envmarks, typemarks);

			freekeymap(escape);
			freekeymap(typemarks);
			freekeymap(envmarks);

			freeref(dag);
			
			break;
		}

		default:
			ERR("%s", "usage: [-f form-source]+");
		}
	}
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

static void progressread(
	FILE *const f,
	Array *const U, const List **const env, const List **const ctx)
{
	while(1)
	{
		const SyntaxNode sntx = readone(f, U);

		if(sntx.op != EOF)
		{
//			progress(U, env, ctx, sntx);
		}
		else
		{
			break;
		}
	}
}

int main(int argc, char *const argv[])
{
	Array *const U = newatomtab();
	
	// Прогружаем таблицу атомов начальными значениями. Они системные,
	// поэтому hint = 0

	for(unsigned i = 0; syntaxops[i]; i += 1)
	{
		assert(readpack(U, strpack(0, syntaxops[i])).u.number == i);
	}

	Array *const env = newkeymap();
	Array *const types = newkeymap();

	// Инициализация примитивных типов

	for(unsigned i = 0; i < 0x10; i += 1)
	{
// 		DL(key, RS(readpack(U, strpack(i << 4, ""))));
		const Ref key = reflist(RL(readpack(U, strpack(i << 4, ""))));

		if(DBGFLAGS & DBGTYPES)
		{
			DBG(DBGTYPES, "%u", i);
			dumpkeymap(1, stderr, 0, U, types);

			char *const kstr = strref(U, NULL, key);
			DBG(DBGTYPES, "%s -> %p",
				kstr, (void *)maplookup(types, key));
			free(kstr);
		}

// 		const Ref key = readpack(U, strpack(i << 4, ""));
		assert(mapreadin(types, key));
	}

	// Основной цикл вывода графа программы. Чтение с stdin. Если вывод не
	// удался, то, всё равно, выдаём накопленный в "придонном" контексте
	// граф. Потому что, пока cfe не отлажен корректный вывод не получится

	if(CKPT() == 0)
	{
		item = 1;
		unitname = "stdin";

		initforms(argc, argv, U, env, types);
		DBG(DBGMAIN, "%s", "forms loaded");

		progressread(stdin, U, NULL, NULL);
		checkout(0);
	}
	else
	{
		DBG(DBGMAIN, "%s", "progression error; dumping result anyway");
	}

	if(DBGFLAGS & DBGMAINEX)
	{
		DBG(DBGMAINEX, "env: count = %u", env->count);
		dumpkeymap(0, stderr, 0, U, env);

		DBG(DBGMAINEX, "%s", "types:");
		dumptable(stderr, 0, U, types);
	}

	// FIXME:
	dumpdag(0, stdout, 0, U,
		reflist(NULL), newverbmap(U, 0, ES("F", "LB")));
	assert(fputc('\n', stdout) == '\n');

	return 0;
}
