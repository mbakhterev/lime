#ifndef ARRAYHINCLUDED
#define ARRAYHINCLUDED

typedef struct {
	void * buffer;
	unsigned capacity;
	unsigned itemlength;
} Array;

// Создание пустого массива
extern Array mkarray(const unsigned itemlength);

// Массив будет расширен для вмещения count элементов размером itemlength. Общая
// доступная длина будет capacity * itemlength
extern void * exporesize(Array *, unsigned count);

#endif
