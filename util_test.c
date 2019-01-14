#include <stdio.h>
#include <stdlib.h>

#include "hkdcc.h"

void assert(int line, int expected, int actual) {
  if (expected == actual)
    return;
  fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
  exit(1);
}

void test_vector() {
  Vector *vec = new_vector();
  assert(__LINE__, 0, vec->len);

  for (long i = 0; i < 100; i++)
    vec_push(vec, (void *)i);

  assert(__LINE__, 100, vec->len);
  assert(__LINE__, 0, (int)vec->data[0]);
  assert(__LINE__, 50, (int)vec->data[50]);
  assert(__LINE__, 99, (int)vec->data[99]);
}

void test_map() {
  Map *map = new_map();
  assert(__LINE__, 0, (int)map_get(map, "foo"));

  map_put(map, "foo", (void *)2);
  assert(__LINE__, 2, (int)map_get(map, "foo"));

  map_put(map, "bar", (void *)4);
  assert(__LINE__, 4, (int)map_get(map, "bar"));

  map_put(map, "foo", (void *)6);
  assert(__LINE__, 6, (int)map_get(map, "foo"));
}

void runtest() {
  test_vector();
  test_map();
  printf("OK\n");
}
