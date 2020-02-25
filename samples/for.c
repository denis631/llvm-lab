// test program:
// simple loop
int xex(int b) {
	return b + 9;
}


int main() {
    int x = -7;
	x++;
	for (int i = 0; i < 500; i++)
		x = xex(i);
    return x;
}
