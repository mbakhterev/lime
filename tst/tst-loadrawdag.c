#include <stdio.h>

static void somefn(int have, const int expecting[]) {
}

int main(const int ac, const char *const av[]) {
	somefn(EOF, (int[]) { 'a', 'b', 'c' });
	return 0;
}
