int b(int x, int y) {
    if (y == 0) {
        return x;
    }
    return y - x;
}

int a(int x, int y, int z) {
    int l = x + y;
    int m = b(3, z);
    return l - m;
}

int main(int argc, char const* argv[]) {
    int l = 4;
    int n = 2;
    int x = a(n, l, 1); // a(2,4,1) = 2+4-b(3,1) = 6-(1-3) = 8
    int y = b(x, l);    // b(8, 4) = 4-8 = -4
    return x + y;       // 8-4 = 4
}
