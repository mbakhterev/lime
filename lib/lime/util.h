#ifndef UTILHINCLUDED
#define UTILHINCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
// #include <err.h>
#include <string.h>
#include <setjmp.h>

// Координаты при чтении, для формирования сообщения об ошибке. Устанавливаются
// извне

extern unsigned item;
extern unsigned field;
extern const char *unitname;

// Процедуры названы вслед за терминологией из книги Хоара «CSP».
// 
// Процедура checkpoint создаёт контрольную точку, которая потом устанавливается
// при помощи setjmp. Эти контрольные точки записываются в стек, для возможности
// возвратов из глубоких рекурсий.
// 
// Процедура checkout - это парное событие для checkpoint. Она принимает
// аргумент, и если он равен 0, то всё интерпретируется, как успешное завершение
// процесса, начатого с контрольной точки. В этом случае из стека извлекается
// элемент на вершине (если стек не пуст) и работа продолжается. Если аргумент
// checkout не нулевой (а именно, пока для надёжности и вслед за книгой -
// STRIKE), то это интерпретируется как ошибка. В такой ситуации checkout
// извлекает с вершины стека контрольных точек элемент и передаёт на него
// управление при помощи longjmp. Если стек пуст, то checkout завершает процесс
// с кодом EXIT_FAILURE.
// 
// Таким образом вот пример использования:
// 
// if(setjmp(*checkpoint()) == 0) {
// 	// Здесь процедура с потенциальным ERR (или checkout(STRIKE)), например:
// 	progressread(f, U, env, ctx);
// 	// Всё хорошо, ставим галочку:
// 	checkout(0);
// }
// else
// {
// 	// Выходим на уровень выше
// 	ERR("%s", "something wrong with progressread")
// }
 
enum { STRIKE = 2 };

extern jmp_buf *checkpoint(void);
extern void checkout(int code);

extern unsigned therearepoints(void);

#if 0
#define ERR(fmt, ...) \
	err(EXIT_FAILURE, __FILE__ "(%u) %s: error:\n\t%s(%u): " fmt, \
		__LINE__, __func__, unitname, item, __VA_ARGS__)
#endif

// Имеет смысл упростить синтаксис

#define CKPT() (setjmp(*checkpoint()))

#define ERR(fmt, ...) \
do { \
	fprintf(stderr, \
		__FILE__ "(%u) %s: error(%s):\n\t%s(%u): " fmt "\n", \
		__LINE__, __func__, strerror(errno), \
		unitname, item, __VA_ARGS__); \
	checkout(STRIKE); \
} while(0)

#define DBG(f, fmt, ...) \
	(void)((f & DBGFLAGS) \
		&& fprintf(stderr, \
			__FILE__ "(%u) %s:\t" fmt "\n", \
			__LINE__, __func__, __VA_ARGS__))

#define MAXNUM ((unsigned)-1 >> 1)

extern unsigned middle(const unsigned l, const unsigned r);

extern int cmpui(const unsigned, const unsigned);
extern int cmpptr(const void *const, const void *const);

extern unsigned min(const unsigned, const unsigned);

// expogrow работает в предположении, что в буффер, на который указывает buf,
// входит count элементов длины ilen. Предполагается и то, что размер этого
// буфера равен ближайшей сверху к (cnt*ilen) степени двойки. Если для хранения
// cnt+1 элементов размером ilen этого окажется недостаточно, то expogrow
// увеличит буфер в два раза.

extern void *expogrow(void *const buf, const unsigned cnt, const unsigned ilen);

extern int skipspaces(FILE *const);

#define ES(...) ((const char *[]) { __VA_ARGS__, NULL })
extern void errexpect(const int have, const char *const expecting[]);

extern FILE *newmemstream(char **const ptr, size_t *const size);

#endif
