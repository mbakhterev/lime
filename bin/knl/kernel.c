#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>
#include <unistd.h>

unsigned item = 1;
const char *unitname = "stdin";

// static List *initforms(int argc, char *const argv[])
// {
// 	optind = 1;
// 	int opt = 0;
// 	while((opt = getopt(argc, argv, "f:")) != -1)
// 	{
// 		switch(opt)
// 		{
// 		case 'f':
// 		
// 
// 		default:
// 			ERR("%s", "usage: [-f form-file]");
// 		}
// 	}
// 
// 	return NULL;
// }

const char *const stdmap[] =
{
	"F", "LB", NULL
};

int main(int ac, char *const av[])
{
	Array U = makeatomtab();
	const Array map = keymap(&U, 0, stdmap);
	loaddag(stdin, &U, &map);

	return 0;
}
