int a(int in) {
    return a(in+1);
}

int main() {
    int x = a(5);
    return x;
}
