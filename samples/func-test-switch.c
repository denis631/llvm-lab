int a(int in) {
    in++;
    return in;
}

int main() {
    int x = 0;
    x++;
    for (int i = 0; i < 4; i++) {
        switch(i) {
            case 0:
                x += a(i);
                break;
            case 1:
                x += a(i);
                break;
            case 2:
                x += a(i);
                break;
            case 3:
                x += a(i);
                break;
        }
    }
    return x;
}
