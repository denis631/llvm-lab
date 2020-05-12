int a(int b, int c) {
    if (b > c) {
        return c;
    }
    return b;
}

int main(int argc, char const* argv[]) {
    int x = a(3, 4);
    return 0;
}
