#include <stdio.h>
#include <string.h>

#include "hkdcc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "arguments count mismatch\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  Vector *tokens = tokenize(argv[1]);

  // for debug
  // show_tokens(tokens);

  Node *main = parse(tokens);

  // for debug
  // show_node(main, 0);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  generate(main, NULL);
  return 0;
}
