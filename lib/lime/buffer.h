#ifndef BUFFERHINCLUDED
#define BUFFERHINCLUDED

// Изменение размера буфера с началом в b. Новый размер - ближайшая сверху
// степень 2 к запрашиваемому размеру: (*plen * len). Новая длина в блоках
// размера len записывается в *plen
extern void * exporesize(void * b, unsigned * plen, unsigned len);

#endif
