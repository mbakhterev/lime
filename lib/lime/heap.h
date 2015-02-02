#ifndef HEAPHINCLUDED
#define HEAPHINCLUDED

typedef int (*ItemCmp)(const void *const data,
	const unsigned i, const unsigned j);

typedef int (*KeyCmp)(const void *const data,
	const unsigned i, const void *const key);

extern void heapsort(const void *const data,
	unsigned index[], const unsigned, ItemCmp);

extern unsigned heapsearch(const void *const D,
	const unsigned I[], const unsigned N, const void *const key, KeyCmp);

#endif
