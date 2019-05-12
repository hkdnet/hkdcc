#include <stdio.h>
#include <stdlib.h>

int foo() {
  printf("foo is called\n");
  return 1;
}
int bar(int a, int b) {
  printf("bar is called %d + %d\n", a, b);
  return a + b;
}
int baz(int a) {
  printf("arg is %d\n", a);
  return a - 1;
}
void alloc4(int *p, int e0, int e1, int e2, int e3) {
  p = malloc(sizeof(int) * 4);
  p[0] = e0;
  p[1] = e1;
  p[2] = e2;
  p[3] = e3;
}
