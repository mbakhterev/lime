#ifndef BUFFERHINCLUDED
#define BUFFERHINCLUDED

typedef struct {
	void * buffer;
	unsigned capacity;
	unsigned itemlength;
} Array;

// Изменение размера буфера с началом в b. Новый размер - ближайшая сверху
// степень 2 к запрашиваемому размеру: (*plen * len). Новая длина в блоках
// размера len записывается в *plen
extern void * exporesize(Array *, unsigned len);

#endif
