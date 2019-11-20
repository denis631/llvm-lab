#include <stdio.h>

int a(int in) {
  printf("In a()\n");
  int a_val = in - 1;
  if (in == 0) {
    a_val = 4;
    return a_val;
  }
  a_val = a_val - 3;
  if (a_val > 2) {
    return 2;
  } else {
    if ( a_val == in) {
      return 0;
    }
    return a_val;
  }
}

int b(int in) {
  int b_val = in +3;
  return b_val;
}

int main(int argc, char const *argv[]) {
  int x = a(4);
  int y = b(x);
  return y+1;
}
