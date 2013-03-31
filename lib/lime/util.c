#include "util.h"

unsigned middle(const unsigned a, const unsigned b) {
	return a + (b - a) / 2;
}

int cmpui(const unsigned a, const unsigned b) {
	return 1 - (a == b) - ((a < b) << 1);
}
