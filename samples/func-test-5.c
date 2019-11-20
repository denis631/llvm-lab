int a(int in) {
    in++;
    return in;
}

int main() {
    int x_1 = a(0); // 1
    int x_2 = a(1); // 2
    int x_3 = a(2); // 3
    int x_4 = a(3); // 4
    return x_1 + x_2 + x_3 + x_4; // 10
}
