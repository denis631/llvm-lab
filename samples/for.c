#include <stdio.h>
#include <stdlib.h>
// test program:
// simple loop
int xex(int b) {
	return b + 9;
}


int main(int argc, char const *argv[]) {
    int x = -7;
	x++;
	for (int i = 0; i < 500; i++)
		x = xex(i);
    return x;
}
