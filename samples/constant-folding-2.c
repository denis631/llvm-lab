int fun(int x, int y) {
  int z = 58;
  int k = 0;

  if (x == y) {
    z = 31;
    z--;
  } else {
    z = 37;
    z = z - (2 * 4) + 1;
  }

  for (int i = 0; i < 42; i++) {
    k = z * 3;
  }

  return k + 317;
}

int main() {
  int x = 5;
  int y = 10;
  int z = x + y;
  z = 3 * z;
  z = 5 * z;

  if (x + y == 15) {
    x = 7;
    x++;
  } else {
    x = 9;
    x++;
  }

  int r = fun(4, 5);
  r = fun(4, 5);

  return x;
}
