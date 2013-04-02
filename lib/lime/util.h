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

#define MAXNUM ((1 << (8*sizeof(unsigned) - 1)) - 1)

extern unsigned middle(const unsigned l, const unsigned r);

extern int cmpui(const unsigned, const unsigned);

// expogrow работает в предположении, что в буффер, на который указывает buf,
// входит count элементов длины ilen. Предполагается и то, что размер этого
// буфера равен ближайшей сверху к (cnt*ilen) степени двойки. Если для хранения
// cnt+1 элементов размером ilen этого окажется недостаточно, то expogrow
// увеличит буфер в два раза.
extern void *expogrow(void *const buf, const unsigned cnt, const unsigned ilen);

#endif
