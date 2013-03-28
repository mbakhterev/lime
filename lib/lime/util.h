#ifndef UTILHINCLUDED
#define UTILHINCLUDED

#include <stdio.h>

// Координаты при чтении, для формирования сообщения об ошибке. Устанавливаются
// извне
extern unsigned item;
extern unsigned field;
extern const char * unitname;

#define ERROR(fmt, ...) \
	error(EXIT_FAILURE, errno, "%s(%u:%u) error: " fmt, \
		unitname, item, field, __VA_ARGS__)

#define DBG(f, fmt, ...) \
	(void)((f & DBGFLAGS) \
		&& fprintf(stderr, \
			__FILE__ ":%u\t%s\t" fmt "\n", \
			__LINE__, __func__, __VA_ARGS__))

extern unsigned middle(const unsigned l, const unsigned r);

#endif
