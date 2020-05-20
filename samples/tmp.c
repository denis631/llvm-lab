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

  return x;
}
