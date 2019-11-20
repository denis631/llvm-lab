int main() {
    float a = 1; a++; // a={2}
    float b = 2; b++; // b={3}
    float c;
    c += 1;         // c=T
    
    if(c == a){     // c=a={2}
        c += 1;     // c={3}
        a = c;      // a={3}
    }
    
                    // a={2,3}
    return 0;   // ret={5,6}
}
