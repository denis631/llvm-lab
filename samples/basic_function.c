int incr(int a) {
	a++;
	return a;
}

int main() {
	int a = 1;
	int b = 0;
	a++;
	b++;
	b = incr(a);
	return a + b;
}