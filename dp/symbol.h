#ifndef symbolhincluded
#define symbolhincluded

typedef struct
{
	const char * str;
	unsigned char len;
	unsigned char priority;
} operatorsymbol;

#define OPSYM(str) {.str = str, .strlen = sizeof(str), .priority = PRIO}

extern const operatorsymbol opsymtable[] =
{
#define PRIO 0
OPSYM(";")
#undef PRIO

#define PRIO 1
OPSYM("=")
OPSYM("*=")
OPSYM("/=")
OPSYM("%=")
OPSYM(">>=")
OPSYM("<<=")
OPSYM("&=")
OPSYM("+=")
OPSYM("-=")
OPSYM("|=")
OPSYM("^=")
OPSYM("||=")
OPSYM("&&=")
#undef PRIO


OPSYM("->")

OPSYM(":")

OPSYM("||")

OPSYM("&&")

OPSYM("==")
OPSYM("!=")
OPSYM("<")
OPSYM("<=")
OPSYM(">")
OPSYM(">=")

OPSYM("+")
OPSYM("-")
OPSYM("|")
OPSYM("^")

OPSYM("*")
OPSYM("/")
OPSYM("%")
OPSYM("<<")
OPSYM(">>")
OPSYM("&")

OPSYM("@")

OPSYM("!")
OPSYM("-")
OPSYM("^")

OPSYM(".")
OPSYM("@p")

OPSYM("(")
OPSYM(")")

}

#endif
