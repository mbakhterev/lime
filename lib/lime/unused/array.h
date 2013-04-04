#ifndef ARRAYHINCLUDED
#define ARRAYHINCLUDED

typedef struct {
	void * buffer;
	unsigned capacity;
	unsigned itemlength;
	unsigned count;
} Array;

// Создание пустого массива
extern Array mkarray(const unsigned itemlength);
extern void freearray(Array *const);

// Массив будет расширен для вмещения count элементов размером itemlength. Общая
// доступная длина будет capacity * itemlength
extern void * exporesize(Array *, unsigned count);

extern void * append(Array *const, const void *const itemptr);

#endif
