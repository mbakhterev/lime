#include <construct.h>
#include <util.h>

#define FIN 0U
#define NTH 1U
#define SNODE 2U
#define TNODE 3U
#define TENV 4U

static const char *const verbs[] =
{
	[FIN] = "FIn",
	[NTH] = "Nth",
	[SNODE] = "S",
	[TNODE] = "T",
	[TENV] = "TEnv"
};
