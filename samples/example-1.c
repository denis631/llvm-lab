volatile int x1,x2,x3;
volatile int condition;


void P() {
    if (condition) {
        x1 = x1 + x2 + 1;
        x3 = x3 + 1;
        P();
        x1 = x1 - x2;   
    }
}


int main() {
    x2 = x1;
    x3 = 0;
    P();
    x1 = 1 - x2 - x3;
}