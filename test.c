#include "stdio.h"

struct A {
	int x;
};


int main(int argc, char const *argv[])
{
	struct A A;
	A.x = 1;
	printf(A.x);
	return 0;
}
