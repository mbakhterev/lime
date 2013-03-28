#ifndef HEAPSORTHINCLUDED
#define HEAPSORTHINCLUDED

typedef int(*Cmp)(const void *const data, const unsigned i, const unsigned j);

void heapsort(const void *const data, unsigned index[], const unsigned, Cmp);

#endif
