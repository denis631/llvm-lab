#include <stdio.h>

int a(int in) {
  if (in == 0) {
      return in;
  }
  int y = a(in - 1);
  return y;
}


int main(int argc, char const *argv[]) {
    int x = a(3);
    return x;
}
