#include <stdio.h>
int foo() {
  printf("foo is called\n");
  return 1;
}
int bar(int a, int b) {
  printf("bar is called %d + %d\n", a, b);
  return a + b;
}
