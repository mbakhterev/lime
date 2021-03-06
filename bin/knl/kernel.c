#include <lime/construct.h>
#include <lime/util.h>
#include <lime/fs.h>

#include <assert.h>

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

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
const char *unitname = "<stdin>";

static const char *const dagverbs[] =
{
	"F", "LB", NULL
};

static Ref initone(Core *const C, const Ref D, const char *const optarg)
{
	assert(isdag(D));
	Array *const U = C->U;

	unitname = optarg;
	item = 1;
	FILE *const f = fopen(unitname, "r");
	if(!f)
	{
		ERR("can't open initial forms source: %s", unitname);
		return reffree();
	}

	Array *const subdags = newverbmap(U, 0, dagverbs);
	const Ref src = loaddag(f, unitname, U, subdags);
	freekeymap(subdags);
	fclose(f);

	const Ref dag = eval(C, NULL, src, 0, NULL, EMINIT);
	freeref(src);

	return refdag(append(D.u.list, dag.u.list));
}

static Ref initmany(Core *const C, const Ref D, const char *const optarg)
{
	assert(isdag(D));

	FILE *const f = fopen(optarg, "r");
	if(!f)
	{
		ERR("can't open initial forms sources list: %s", optarg);
		return reffree();
	}

	// Буфер для очередного имени файла
	char buffer[MAXPATH + 2];

	// Граф в котором будем копить формы
	Ref d = D;

	while(fgets(buffer, MAXPATH + 1, f) == buffer)
	{
		// unitname будет меняться после каждого обращения к initone
		unitname = optarg;

		const unsigned len = strlen(buffer);

		// Разбор крайних случаев

		if(len == 1 && buffer[len - 1] == '\n')
		{
			// Прочитана пустая строка, пропускаем
			continue;
		}

		if(len == MAXPATH && !feof(f))
		{
			// В этом случае буфер заполнен под завязку, а конец
			// файла или перевод строки не обнаружены. Это ошибка,
			// слишком длинное имя файла

			ERR("file name is too long: %s", buffer);
			continue;
		}

		// В оставшихся случаях всё должно быть хорошо

		if(buffer[len - 1] == '\n')
		{
			buffer[len - 1] = '\0';
		}
		else
		{
			assert(buffer[len - 1] == '\0' && feof(f));
		}

		// Загружаем

		char *const fullpath = joinpath(optarg, buffer);
		d = initone(C, d, fullpath);
		free(fullpath);
	}

	return d;
}

static Ref initforms(Core *const C, const int argc, char *const argv[])
{
	Ref D = refdag(NULL);

	optind = 1;
	int opt = -1;
	while((opt = getopt(argc, argv, "df:F:")) != -1)
	{
		switch(opt)
		{
		case 'f':
			D = initone(C, D, optarg);
			break;

		case 'F':
			D = initmany(C, D, optarg);
			break;

		case 'd':
			break;

		default:
			ERR("%s",
				"usage: [-d] "
				"[-f form-source]+ "
				"[-F form-source-list]+");
			break;
		}
	}

	return D;
}

static unsigned initinfodump(const int argc, char *const argv[])
{
	optind = 1;
	int opt = 0;
	unsigned dump = 0;
	while((opt = getopt(argc, argv, "df:F:")) != -1)
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
				ERR("%s",
					"usage: [-d] "
					"[-f form-source]+ "
					"[-F form-source-list]+");
			}

			break;
		
		case 'f':
		case 'F':
			break;

		default:
			ERR("%s",
				"usage: [-d]"
				"[-f form-source]+ "
				"[-F form-source-list]+");
		}
	}

	return dump;
}

static char *const syntaxops[] =
{
	[AOP] = "A",
	[UOP] = "U",
	[LOP] = "L",
	[EOP] = "E",
	[FOP] = "F",
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
	case 'E': return EOP;
	case 'F': return FOP;
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

typedef struct
{
	// Входной поток синтаксических инструкций
	FILE *const f;

	// Координаты очередного токена в исходном тексты программы
	unsigned line;
	unsigned column;
	unsigned file;
} ReadContext;

// static Position readposition(FILE *const f)

static Position readposition(ReadContext *const rc, const unsigned op)
{
	int lineoffset;
	int coloffset;
	
	if(fscanf(rc->f, "%d.%d", &lineoffset, &coloffset) != 2)
	{
		return notapos();
	}
	
	const unsigned line = op == FOP ? rc->line : rc->line + lineoffset;
	const unsigned column = op == FOP ? rc->column : rc->column + coloffset;

	if(line < MAXNUM && column < MAXNUM)
	{
		rc->line = line;
		rc->column = column;

		return (Position)
		{
			.line = line,
			.column = column,

			// FIXME: При обработке узла F заносим сюда информацию о
			// предыдущем обрабатываемом файле. Может быть будет
			// полезно делать именно так
			.file = rc->file
		};
	}

	return notapos();
}

// static SyntaxNode readone(FILE *const f, Array *const U)

static SyntaxNode readone(ReadContext *const rc, Array *const U)
{
	int c = -1;
	
	if((c = skipspaces(rc->f)) == EOF)
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

	if(!isdigit(c = skipspaces(rc->f)))
	{
		ERR("%s", "can't get syntax node position");
	}

	assert(ungetc(c, rc->f) == c);
	const Position pos = readposition(rc, op);
	if(!isgoodpos(pos))
	{
		ERR("%s", "can't get syntax node position");
	}

	// Сам атом. Должен начинаться с шестнадцатеричного hint

	if(!isxdigit(c = skipspaces(rc->f)))
	{
		ERR("%s", "can't get syntax node atom");
	}

	assert(ungetc(c, rc->f) == c);
	const unsigned atom = loadatom(U, rc->f).u.number;

	return (SyntaxNode)
	{
		.op = op,
		.atom = atom,
		.pos = pos
	};
}

// Тут read в форме past perfect

// static void progressread(FILE *const f, Core *const C)

static void progressread(ReadContext *const rc, Core *const C)
{
	while(1)
	{
		const SyntaxNode sntx = readone(rc, C->U);

		if(sntx.op == FOP)
		{
			// FIXME: учесть смену имени файла

			rc->file = sntx.atom;
			rc->line = sntx.pos.line;
			rc->column = sntx.pos.column;
		}
		else if(sntx.op != EOF)
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

int main(int argc, char *const argv[])
{
	const unsigned dip = initinfodump(argc, argv);
	Array *const U = inituniverse();

	Core *const C = newcore(U, NULL, NULL, dip);

	const Ref D = initforms(C, argc, argv);

	DBG(DBGMAIN, "%s", "forms loaded");

	if(DBGFLAGS & DBGMAIN)
	{
		const Array *const R = envkeymap(C->E, refenv(0));
		const Array *const esc = stdupstreams(U);
		DBG(DBGMAIN, "env: count = %u", R->count);

		dumpkeymap(1, stderr, 0, U, R, esc);
		freekeymap((Array *)esc);

		DBG(DBGMAIN, "%s", "types:");
		dumptable(stderr, 0, U, C->T);
	}

	ReadContext rc =
	{
		.f = stdin,
		.line = 1,
		.column = 1,
		.file = readpack(U, strpack(3, "<stdin>")).u.number
	};


	// Основной цикл вывода графа программы. Чтение с stdin. Если вывод не
	// удался, то, всё равно, выдаём накопленный в "придонном" контексте
	// граф. Потому что, пока cfe не отлажен корректный вывод не получится

	unsigned ok = 0;
	if(CKPT() == 0)
	{
		item = 1;
		unitname = "<stdin>";


// 		progressread(stdin, C);
		
		progressread(&rc, C);
		ok = 1;
		checkout(0);
	}
	else
	{
	}

	if(ok)
	{
		const Ref A = C->areastack->ref;
		assert(isarea(A));

		// Прицепим накопленный в инициализации граф к накопленному во
		// время вывода

		Ref *const AD = areadag(U, A.u.array);
		AD->u.list = append(D.u.list, AD->u.list);

		const Ref dag
			= reconstruct(U,
				*AD, C->verbs.system, C->E, C->T, C->S);

		dumpdag(0, stdout, 0, U, dag);
		fflush(stdout);
		freeref(dag);
		assert(fputc('\n', stdout) == '\n');
	}
	else if(dip)
	{
		assert(fprintf(stderr, "\nENVIRONMENT\n\n") > 0);

		const Array *const rootescape = stdupstreams(U);
		dumpkeymap(0, stderr, 0, U,
			envkeymap(C->E, refenv(0)), rootescape);
		freekeymap((Array *)rootescape);

		assert(fputc('\n', stderr) == '\n');

		assert(fprintf(stderr, "STACK\n") > 0);

		const Array *const stackescape = stdstackupstreams(U);
		dumpareastack(0, stderr, 0, U, C->areastack, stackescape);
		freekeymap((Array *)stackescape);

		assert(fputc('\n', stderr) == '\n');

		assert(fprintf(stderr,
			"progression error. State dumped to stderr\n") > 0);
	}

	freecore(C);

	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
